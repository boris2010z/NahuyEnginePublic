// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "lua_toolkit/lua_utils.h"

#include "lua_toolkit/lua_interop.h"
#include "nau/utils/scope_guard.h"

namespace nau::lua
{
    StackGuard::StackGuard(lua_State* l) :
        luaState(l),
        top(lua_gettop(luaState))
    {
    }

    StackGuard::~StackGuard()
    {
        const int currentTop = lua_gettop(luaState);
        if(currentTop == top)
        {
            return;
        }

        // it is possible to restore the stack only if it is larger than the current one.
        // Potentially, there may be situations where the logic (is the opposite) removes values from the stack;
        // in this case, consider the use of lua::StackGuard to be incorrect.
        if(top < currentTop)
        {
            lua_settop(luaState, top);
        }
        else
        {
            // LOG_WARN("The lua's stack size has decreased. Value popped from stack unexpectedly (the bug ?) or stack guard should not be used.");
            // if (IsRunningUnderDebugger()) {
            // 	BreakIntoDebugger();
            // }
        }
    }

    Result<> loadBuffer(lua_State* l, std::string_view buffer, const char* chunkName)
    {
        NAU_ASSERT(l);
        NAU_ASSERT(!buffer.empty());

        if(!l)
        {
            return NauMakeError("Invalid argument");
        }

        if(luaL_loadbuffer(l, buffer.data(), buffer.size(), chunkName) == 0)
        {
            return {};
        }

        scope_on_leave
        {
            lua_pop(l, 1);
        };

        size_t len;
        const char* const message = lua_tolstring(l, -1, &len);
        return NauMakeError(eastl::string{message, len});
    }

    int getAbsoluteStackPos(lua_State* l, int index)
    {
        NAU_ASSERT(index != 0);

        if(index > 0 || index <= LUA_REGISTRYINDEX)
        {
            return index;
        }

        const int top = lua_gettop(l);
        const int pos = top + (index + 1);
        NAU_ASSERT(pos > 0);
        return pos;
    }

    UpValuesEnumerator::iterator::iterator() = default;

    UpValuesEnumerator::iterator::iterator(lua_State* l, int index, int n) :
        m_luaState(l),
        m_index(index),
        m_n(n)
    {
        NAU_ASSERT(m_luaState);
        NAU_ASSERT(m_n > 0);

        if(m_name = lua_getupvalue(m_luaState, m_index, m_n); m_name == nullptr)
        {
            m_n = -1;
        }
    }
#if 0
    UpValuesEnumerator::iterator& UpValuesEnumerator::iterator::operator++()
    {
        NAU_ASSERT(_lua);
        NAU_ASSERT(_name);
        NAU_ASSERT(_n > 0);

        lua_pop(_lua, 1);

        if(_name = lua_getupvalue(_lua, _index, ++_n); _name == nullptr)
        {
            _n = -1;
        }

        return *this;
    }

    std::string_view UpValuesEnumerator::iterator::operator*() const
    {
        return this->Name();
    }

    std::string_view UpValuesEnumerator::iterator::Name() const
    {
        NAU_ASSERT(_n > 0 && _name);
        return std::string_view{_name};
    }

    int UpValuesEnumerator::iterator::Index() const
    {
        NAU_ASSERT(_n > 0);
        return _n;
    }

    bool operator==(const UpValuesEnumerator::iterator& iter1, const UpValuesEnumerator::iterator& iter2)
    {
        return iter1._n == iter2._n;
    }

    bool operator!=(const UpValuesEnumerator::iterator& iter1, const UpValuesEnumerator::iterator& iter2)
    {
        return iter1._n != iter2._n;
    }

    UpValuesEnumerator::UpValuesEnumerator(lua_State* l, int index) :
        _lua(l),
        _index(index)
    {
    }

    UpValuesEnumerator::iterator UpValuesEnumerator::begin() const
    {
        return iterator{_lua, _index, 1};
    }

    UpValuesEnumerator::iterator UpValuesEnumerator::end() const
    {
        return iterator{};
    }
#endif

    TableEnumerator::TableEnumerator(lua_State* l, int tableIndex) :
        m_luaState(l),
        m_tableIndex(getAbsoluteStackPos(l, tableIndex))
    {
    }

    TableEnumerator::iterator TableEnumerator::begin() const
    {
        NAU_ASSERT(lua_type(m_luaState, m_tableIndex) == LUA_TTABLE);
        lua_pushnil(m_luaState);
        return iterator{m_luaState, m_tableIndex}.takeNext();
    }

    TableEnumerator::iterator& TableEnumerator::iterator::operator++()
    {
        NAU_ASSERT(m_luaState && m_tableIndex != BadIndex);

        lua_pop(m_luaState, 1);  // pop out last value
        return takeNext();
    }

    TableEnumerator::iterator& TableEnumerator::iterator::takeNext()
    {
        if(lua_next(m_luaState, m_tableIndex) == 0)
        {
            m_tableIndex = BadIndex;
            m_luaState = nullptr;
        }

        return *this;
    }

}  // namespace nau::lua
