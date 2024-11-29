// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include <json/writer.h>

#include "nau/serialization/json.h"

namespace nau::serialization
{
    namespace
    {

        Json::StreamWriter& getJsonWriter(JsonSettings settings)
        {
            using namespace ::Json;

            static thread_local eastl::unique_ptr<StreamWriter> writer;
            static thread_local eastl::unique_ptr<StreamWriter> prettyWriter;

            const auto prepareWriter = [](eastl::unique_ptr<StreamWriter>& w, JsonSettings settings) -> StreamWriter&
            {
                if(!w)
                {
                    Json::StreamWriterBuilder wbuilder;
                    if(settings.pretty)
                    {
                        wbuilder["indentation"] = "\t";
                    }
                    else
                    {
                        wbuilder["indentation"] = "";
                    }

                    w = eastl::unique_ptr<Json::StreamWriter>(wbuilder.newStreamWriter());
                }

                return *w;
            };

            return settings.pretty ? prepareWriter(prettyWriter, settings) : prepareWriter(writer, settings);
        }

        class WriterStreambuf final : public std::streambuf
        {
        public:
            WriterStreambuf(io::IStreamWriter& writer) :
                m_writer(writer)
            {
            }

            std::streamsize xsputn(const char_type* s, std::streamsize n) override
            {
                static_assert(sizeof(char_type) == sizeof(std::byte));
                m_writer.write(reinterpret_cast<const std::byte*>(s), n).ignore();
                return n;
            };

            int_type overflow(int_type ch) override
            {
                m_writer.write(reinterpret_cast<const std::byte*>(&ch), 1).ignore();
                return 1;
            }

        private:
            io::IStreamWriter& m_writer;
        };

        void makeJsonPrimitiveValue(Json::Value& jValue, const RuntimePrimitiveValue& value)
        {
            if(auto integer = value.as<const RuntimeIntegerValue*>(); integer)
            {
                if(integer->isSigned())
                {
                    jValue = Json::Value(integer->getInt64());
                }
                else
                {
                    jValue = Json::Value(integer->getUint64());
                }
            }
            else if(auto floatPoint = value.as<const RuntimeFloatValue*>(); floatPoint)
            {
                if(floatPoint->getBitsCount() == sizeof(double))
                {
                    jValue = Json::Value(floatPoint->getDouble());
                }
                else
                {
                    jValue = Json::Value(floatPoint->getSingle());
                }
            }
            else if(auto str = value.as<const RuntimeStringValue*>(); str)
            {
                auto text = str->getString();
                jValue = Json::Value(text.data(), text.data() + text.size());
            }
            else if(auto boolValue = value.as<const RuntimeBooleanValue*>(); boolValue)
            {
                jValue = Json::Value(boolValue->getBool());
            }
            else
            {
                // Halt("Unknown primitive type for json serialization");
            }
        }

        Result<> makeJsonValue(Json::Value& jValue, const RuntimeValue::Ptr& value, const JsonSettings& settings)
        {
            if(RuntimeOptionalValue* const optionalValue = value->as<RuntimeOptionalValue*>())
            {
                if(optionalValue->hasValue())
                {
                    return makeJsonValue(jValue, optionalValue->getValue(), settings);
                }

                jValue = Json::Value(Json::nullValue);
                return {};
            }

            if(RuntimeValueRef* const refValue = value->as<RuntimeValueRef*>())
            {
                const auto referencedValue = refValue->get();
                if(referencedValue)
                {
                    return makeJsonValue(jValue, referencedValue, settings);
                }

                jValue = Json::Value(Json::nullValue);
                return {};
            }

            if(const RuntimePrimitiveValue* const primitiveValue = value->as<const RuntimePrimitiveValue*>(); primitiveValue)
            {
                makeJsonPrimitiveValue(jValue, *primitiveValue);
            }
            else if(RuntimeReadonlyCollection* const collection = value->as<RuntimeReadonlyCollection*>())
            {
                jValue = Json::Value(Json::arrayValue);

                for(size_t i = 0, sz = collection->getSize(); i < sz; ++i)
                {
                    if(auto result = makeJsonValue(jValue[jValue.size()], collection->getAt(i), settings); !result)
                    {
                        return result;
                    }
                }
            }
            else if(RuntimeReadonlyDictionary* const obj = value->as<RuntimeReadonlyDictionary*>())
            {
                jValue = Json::Value(Json::objectValue);

                for(size_t i = 0, sz = obj->getSize(); i < sz; ++i)
                {
                    auto key = obj->getKey(i);
                    auto member = obj->getValue(key);

                    if(!settings.writeNulls)
                    {
                        if(RuntimeOptionalValue* const optionalValue = member->as<RuntimeOptionalValue*>())
                        {
                            if(!optionalValue->hasValue())
                            {
                                continue;
                            }
                        }
                        else if(const RuntimeValueRef* refValue = member->as<const RuntimeValueRef*>())
                        {
                            if(!static_cast<bool>(refValue->get()))
                            {
                                continue;
                            }
                        }
                    }

                    if(auto result = makeJsonValue(jValue[key.data()], member, settings); !result)
                    {
                        return result;
                    }
                }
            }

            return {};
        }

    }  // namespace

    Result<> jsonWrite(io::IStreamWriter& writer, const Json::Value& value, JsonSettings settings)
    {
        WriterStreambuf buf{writer};
        std::ostream stream(&buf);
        getJsonWriter(settings).write(value, &stream);

        return ResultSuccess;
    }

    Result<> jsonWrite(io::IStreamWriter& writer, const RuntimeValue::Ptr& value, JsonSettings settings)
    {
        Json::Value root;
        if(auto result = makeJsonValue(root, value, settings); !result)
        {
            return result;
        }

        return jsonWrite(writer, root, settings);
    }

    Result<> runtimeApplyToJsonValue(Json::Value& jsonValue, const RuntimeValue::Ptr& runtimeValue, JsonSettings settings)
    {
        return makeJsonValue(jsonValue, runtimeValue, settings);
    }


    Json::Value runtimeToJsonValue(const RuntimeValue::Ptr& value, JsonSettings settings)
    {
        Json::Value root;
        makeJsonValue(root, value, settings).ignore();
        return root;
    }

}  // namespace nau::serialization
