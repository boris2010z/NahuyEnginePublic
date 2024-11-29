// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/dispatch/dispatch.h"

#if 0
#include "nau/dispatch/native_dispatch.h"
#include "nau/meta/class_info.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/utils/strings.h"
#include "nau/utils/type_utility.h"

#include "EASTL/vector.h"
#include "EASTL/string.h"

#include <functional>



namespace nau::test
{
    struct IMyInterface1 : virtual IRefCounted
    {
        NAU_INTERFACE(nau::test::IMyInterface1, IRefCounted);
        //NAU_TYPEID(IMyInterface1);
        NAU_CLASS_NAME("nau::IMyInterface1");

        virtual float sub1(float x, float y) = 0;
    };

    class MyService : public IMyInterface1
    {
    public:
        NAU_CLASS_(nau::test::MyService, IMyInterface1)

        NAU_CLASS_NAME("nau::MyService");

        NAU_CLASS_METHODS(
            CLASS_METHOD(MyService, add1),
            CLASS_METHOD(MyService, getStaticData));

        NAU_CLASS_FIELDS(
            CLASS_FIELD(m_mem));

    public:
        static std::string getStaticData()
        {
            return "data";
        }

        std::string add1(float x, std::optional<float> y = std::nullopt)
        {
            const auto res = y ? (x + *y) : x;
            return std::format("Result ({})", res);
        }

        float sub1(float x, float y) override
        {
            return x - y;
        }

    private:
        float m_mem = 0;
    };

    // class RuntimeArgsHolder final : public RuntimeReadonlyCollection
    // {
    //     NAU_CLASS_(RuntimeArgsHolder, RuntimeReadonlyCollection)

    // public:
    //     RuntimeArgsHolder()
    //     {
    //     }

    //     size_t getSize() const override
    //     {
    //         return m_values.size();
    //     }

    //     RuntimeValue::Ptr getAt(size_t index) const override
    //     {
    //         return m_values[index];
    //     }

    //     bool isMutable() const override
    //     {
    //         return true;
    //     }

    //     template <typename T>
    //     void appendValue(T&& value)
    //     {
    //         m_values.emplace_back(nau::makeValueCopy(std::move(value)));
    //     }

    // private:
    //     std::vector<RuntimeValue::Ptr> m_values;
    // };
/*
    template <typename T>
    class NativeDispatchImpl : public IDispatch
    {
        NAU_RTTI_CLASS(NativeDispatchImpl<T>, IDispatch)
    private:
        template <typename R, typename Class, typename... P>
        static Result<R> invokeInstanceMethod(Class& instance, auto method, RuntimeReadonlyCollection& args, P... params)
        {
            {
                const auto applyParamValue = [&](size_t i, auto& param)
                {
                    auto paramAsRuntimeValue = nau::makeValueRef(param);
                    return RuntimeValue::assign(paramAsRuntimeValue, args[i]);
                };

                size_t argIndex = 0;
                Error::Ptr error;
                if(!(applyParamValue(argIndex++, params).isSuccess(&error) && ...))
                {
                    return error;
                }
            }

            if constexpr(std::is_same_v<R, void>)
            {
                (instance.*method)(std::move(params)...);
                return {};
            }
            else
            {
                return (instance.*method)(std::move(params)...);
            }
        }

    public:
        NativeDispatchImpl(T& instance) :
            m_instance(instance)
        {
        }

        Result<RuntimeValue::Ptr> invoke(std::string_view contract, std::string_view methodName, RuntimeReadonlyCollection::Ptr args) override
        {
            using namespace nau::meta;

            const auto methods = meta::getClassAllMethods<MyService>();
            bool invokeProcessed = false;

            Result<RuntimeValue::Ptr> result;

            nau::Tuple::forEach(methods, [&, this]<auto F, typename... A>(const MethodInfo<F, A...>& methodInfo)
                                {
                                    if(strings::icaseEqual(methodInfo.GetName(), methodName))
                                    {
                                        using CallableInfo = meta::GetCallableTypeInfo<decltype(F)>;

                                        NAU_ASSERT(!invokeProcessed);
                                        invokeProcessed = true;

                                        const auto invokeHelper = [&]<bool Const, bool NoExcept, typename C, typename R, typename... P>(CallableTypeInfo<Const, NoExcept, C, R, P...>) -> Result<R>
                                        {
                                            return NativeDispatchImpl<T>::invokeInstanceMethod<R>(m_instance, F, *args, P{}...);
                                        };

                                        auto invokeResult = invokeHelper(CallableInfo{});
                                        if(!invokeResult)
                                        {
                                            result = invokeResult.getError();
                                        }
                                        else
                                        {
                                            result = nau::makeValueCopy(*std::move(invokeResult));
                                        }
                                    }
                                    //
                                });

            return result;
        }

    private:
        T& m_instance;
    };

*/


    TEST(TestDisptach, Test1)
    {
        eastl::vector<eastl::string> strings1;

        strings1.emplace_back("text1");
        strings1.emplace_back("text2");

        using MyDispatch = nau_detail::NativeDispatchImpl<MyService>;

        auto myService = rtti::createInstance<MyService>();
        auto dispatch = eastl::make_unique<MyDispatch>(myService);

        IDispatch::ArgsCollection args;
        
        args.emplace_back(nau::makeValueCopy(10.f));
        args.emplace_back(nau::makeValueCopy(20.f));


        auto res = dispatch->invoke("", "add1", std::move(args));

        if (!res)
        {
            std::cout << std::format("Fail: ({})\n", res.getError()->getDiagMessage());
        }
        else
        {
            auto* const str = (*res)->as<RuntimeStringValue*>();
            std::cout << std::format("Result: ({})\n", str->getUtf8());
        }

        // constexpr bool a = meta::MetaDefined<MyService, meta::ClassNameAttribute>;

#if 0
        


        nau::Ptr<RuntimeArgsHolder> args = rtti::createInstance<RuntimeArgsHolder>();

        args->appendValue(10.f);
        args->appendValue(20.f);

        auto res = dispatch->invoke("", "add1", args);

        if (!res)
        {
            std::cout << std::format("Fail: ({})\n", res.getError()->getDiagMessage());
        }
        else
        {
            auto* const str = (*res)->as<RuntimeStringValue*>();
            std::cout << std::format("Result: ({})\n", str->getUtf8());
        }
#endif
        auto desc = nau::NativeDispatch::makeDispatchDescription<MyService>();
        std::cout << "Ready_\n";
    }

}  // namespace nau::test


void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::malloc(size);
}

#endif