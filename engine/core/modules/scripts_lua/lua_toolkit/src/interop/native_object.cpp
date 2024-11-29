// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <iostream>

#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "EASTL/unique_ptr.h"

namespace nau::lua_detail
{
    const char* ClassDescriptorMetatableName = "Nau_ClassDescriptorMetatable";
    const char* ClassDescriptorFieldName = "nauClassDescriptor";

    /**
     */
    struct NAU_ABSTRACT_TYPE NativeObject
    {
        virtual ~NativeObject() = default;

        virtual IRttiObject* getObject() const = 0;
    };

    /**
     */
    class NauRefCountedNativeObject final : public NativeObject
    {
    public:
        NauRefCountedNativeObject(nau::Ptr<> obj) :
            m_object(std::move(obj))
        {
            NAU_ASSERT(m_object);
        }

        IRttiObject* getObject() const override
        {
            return m_object.get();
        }

    private:
        const nau::Ptr<> m_object;
    };

    /**
     */
    class StdUniquePtrNativeObject final : public NativeObject
    {
    public:
        StdUniquePtrNativeObject(eastl::unique_ptr<IRttiObject> obj) :
            m_object(std::move(obj))
        {
            NAU_ASSERT(m_object);
        }

        IRttiObject* getObject() const override
        {
            return m_object.get();
        }

    private:
        const eastl::unique_ptr<IRttiObject> m_object;
    };

    /**
     */

    int instanceMethodClosure(lua_State* l) noexcept
    {
        NAU_ASSERT(lua_type(l, 1) == LUA_TUSERDATA);

        auto* const nativeObjWrapper = reinterpret_cast<NativeObject*>(lua_touserdata(l, 1));
        NAU_ASSERT(nativeObjWrapper);

        IRttiObject* const object = nativeObjWrapper->getObject();
        NAU_ASSERT(object);

        DispatchArguments arguments;

        constexpr int FirstArgStackIndex = 2;
        for(int i = FirstArgStackIndex, luaTop = lua_gettop(l); i <= luaTop; ++i)
        {
            arguments.emplace_back(lua::makeValueFromLuaStack(l, i));
        }

        const auto methodUpvalueIndex = lua_upvalueindex(1);
        NAU_ASSERT(lua_type(l, methodUpvalueIndex) == LUA_TLIGHTUSERDATA);
        const IMethodInfo* const method = reinterpret_cast<IMethodInfo*>(lua_touserdata(l, methodUpvalueIndex));

        auto result = method->invokeToPtr(object, std::move(arguments));

        if(nau::Ptr<> resultValue = result)
        {
            if (!lua::pushRuntimeValue(l, resultValue))
            { // TODO: make an error
                lua_pushnil(l);
            }

            return 1;
        }

        return 0;
    }

    int classMethodClosure(lua_State* l) noexcept
    {
        return 0;
    }

    int classCtorClosure(lua_State* l) noexcept
    {
        const auto upvalueIndex = lua_upvalueindex(1);
        NAU_ASSERT(lua_type(l, upvalueIndex) == LUA_TLIGHTUSERDATA);

        IClassDescriptor* const classDescriptor = reinterpret_cast<IClassDescriptor*>(lua_touserdata(l, upvalueIndex));
        NAU_ASSERT(classDescriptor);

        const IMethodInfo* const ctor = classDescriptor->getConstructor();
        NAU_ASSERT(ctor);

        DispatchArguments args;

        nau::Ptr<> object = ctor->invokeToPtr(nullptr, std::move(args));
        NAU_ASSERT(object);
        if(object)
        {
            [[maybe_unused]]
            auto pushRes = lua::pushObject(l, std::move(object), IClassDescriptor::Ptr{classDescriptor});
            NAU_ASSERT(pushRes);

            return 1;
        }

        return 0;
    }

    /**
     */
    void pushClassDescriptorMetatable(lua_State* l)
    {
        if(luaL_newmetatable(l, ClassDescriptorMetatableName) == 0)
        {
            return;
        }

        const auto metatableGC = [](lua_State* l) noexcept -> int
        {
            NAU_ASSERT(lua_type(l, 1) == LUA_TTABLE);

            lua_getfield(l, 1, ClassDescriptorFieldName);
            NAU_ASSERT(lua_type(l, -1) == LUA_TLIGHTUSERDATA);

            auto* const classDescriptor = reinterpret_cast<IClassDescriptor*>(lua_touserdata(l, -1));
            NAU_ASSERT(classDescriptor);
            lua_pop(l, 1);

            classDescriptor->releaseRef();
            return 0;
        };

        lua_pushstring(l, "__gc");
        lua_pushcclosure(l, metatableGC, 0);
        lua_rawset(l, -3);
    }

}  // namespace nau::lua_detail

namespace nau::lua
{
    Result<> initializeClass(lua_State* l, IClassDescriptor::Ptr classDescriptor, bool keepMetatableOnStack)
    {
        NAU_ASSERT(l);
        NAU_ASSERT(classDescriptor);

        std::optional<lua::StackGuard> stackGuard;
        if(!keepMetatableOnStack)
        {
            stackGuard.emplace(l);
        }

        const std::string className = classDescriptor->getClassName();
        NAU_ASSERT(!className.empty());

        if(luaL_newmetatable(l, className.c_str()) == 0)
        {
#ifdef _DEBUG
            lua_getfield(l, -1, lua_detail::ClassDescriptorFieldName);
            NAU_ASSERT(lua_type(l, -1) == LUA_TLIGHTUSERDATA);
            // NAU_ASSERT(lua_touserdata(l, -1) == classDescriptor.get());
            lua_pop(l, 1);
#endif
            return {};
        }

        const auto classGC = [](lua_State* l) noexcept -> int
        {
            NAU_ASSERT(lua_type(l, 1) == LUA_TUSERDATA);
            auto* const nativeObjWrapper = reinterpret_cast<lua_detail::NativeObject*>(lua_touserdata(l, 1));
            NAU_ASSERT(nativeObjWrapper);
            std::destroy_at(nativeObjWrapper);

            return 0;
        };

        lua_pushstring(l, "__gc");
        lua_pushcclosure(l, classGC, 0);
        lua_rawset(l, -3);

        lua_pushstring(l, "__index");
        lua_createtable(l, 0, 0);

        size_t classMethodsCount = 0;

        for(size_t i = 0, interfaceCount = classDescriptor->getInterfaceCount(); i < interfaceCount; ++i)
        {
            const auto& api = classDescriptor->getInterface(i);

            for(size_t x = 0, methodsCount = api.getMethodsCount(); x < methodsCount; ++x)
            {
                const auto& method = api.getMethod(x);
                const auto methodName = method.getName();

                lua_pushlstring(l, methodName.data(), methodName.size());
                if(method.getCategory() == MethodCategory::Instance)
                {
                    lua_pushlightuserdata(l, const_cast<IMethodInfo*>(&method));
                    lua_pushcclosure(l, lua_detail::instanceMethodClosure, 1);
                }
                else
                {
                    lua_pushcclosure(l, lua_detail::classMethodClosure, 0);
                    ++classMethodsCount;
                }

                // -3 index table
                // -2 field name
                // -1 closure
                lua_rawset(l, -3);
            }
        }

        // -3 metatable
        // -2 field name '__index'
        // -1 index table
        NAU_ASSERT(lua_type(l, -1) == LUA_TTABLE);
        NAU_ASSERT(lua_type(l, -2) == LUA_TSTRING);
        NAU_ASSERT(lua_type(l, -3) == LUA_TTABLE);
        lua_rawset(l, -3);

        if(const IMethodInfo* const ctor = classDescriptor->getConstructor())
        {
            lua_pushstring(l, "New");
            lua_pushlightuserdata(l, classDescriptor.get());
            lua_pushcclosure(l, lua_detail::classCtorClosure, 1);
            lua_rawset(l, -3);
            ++classMethodsCount;
        }

        lua_pushstring(l, lua_detail::ClassDescriptorFieldName);
        lua_pushlightuserdata(l, classDescriptor.giveUp());
        lua_rawset(l, -3);

        lua_detail::pushClassDescriptorMetatable(l);
        lua_setmetatable(l, -2);

        if(classMethodsCount > 0)
        {
            lua_pushvalue(l, -1);
            lua_setglobal(l, className.c_str());
        }

        return {};
    }

    Result<> pushObject(lua_State* l, nau::Ptr<> object, IClassDescriptor::Ptr classDescriptor)
    {
        NAU_ASSERT(l);
        NAU_ASSERT(object);
        NAU_ASSERT(classDescriptor);

        void* const mem = lua_newuserdatauv(l, sizeof(lua_detail::NauRefCountedNativeObject), 1);
        new (mem)(lua_detail::NauRefCountedNativeObject)(std::move(object));

        NauCheckResult(lua::initializeClass(l, std::move(classDescriptor), true));
        NAU_ASSERT(lua_type(l, -1) == LUA_TTABLE);

        lua_setmetatable(l, -2);

        return {};
    }

    Result<> pushObject(lua_State* l, eastl::unique_ptr<IRttiObject> object, IClassDescriptor::Ptr classDescriptor)
    {
        NAU_ASSERT(l);
        NAU_ASSERT(object);
        NAU_ASSERT(classDescriptor);

        void* const mem = lua_newuserdatauv(l, sizeof(lua_detail::StdUniquePtrNativeObject), 1);
        new (mem)(lua_detail::StdUniquePtrNativeObject)(std::move(object));

        NauCheckResult(lua::initializeClass(l, std::move(classDescriptor), true));
        NAU_ASSERT(lua_type(l, -1) == LUA_TTABLE);

        lua_setmetatable(l, -2);

        return {};
    }

    Result<> pushDispatch(lua_State* l, nau::Ptr<> dispatch)
    {
        NAU_ASSERT(l);
        NAU_ASSERT(dispatch && dispatch->is<IDispatch>());


        return {};
    }
}  // namespace nau::lua
