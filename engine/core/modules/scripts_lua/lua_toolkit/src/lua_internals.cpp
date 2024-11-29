// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "lua_internals.h"

#include "nau/string/string_utils.h"


namespace nau::lua
{
    ChildVariableKey ChildVariableKey::makeFromStack(lua_State* l, int index)
    {
        const int keyType = lua_type(l, index);
        NAU_ASSERT(keyType == LUA_TNUMBER || keyType == LUA_TSTRING);

        if(keyType == LUA_TNUMBER)
        {
            return static_cast<int>(lua_tointeger(l, index));
        }

        size_t len;
        const char* const value = lua_tolstring(l, index, &len);
        return std::string_view{value, len};
    }

    std::string_view ChildVariableKey::asString() const
    {
        if(!*this)
        {
            return {};
        }

        if(m_index != InvalidLuaIndex)
        {
            if(m_name.empty())
            {
                m_name = strings::lexicalCast(m_index);
            }
        }

        return m_name;
    }

    bool ChildVariableKey::operator==(std::string_view str) const
    {
        NAU_ASSERT(static_cast<bool>(*this));

        if(isIndexed())
        {
            return std::to_string(m_index) == str;
        }

        return m_name == str;
    }

    void ChildVariableKey::push(lua_State* l) const
    {
        NAU_ASSERT(static_cast<bool>(*this));

        if(m_index != InvalidLuaIndex)
        {
            lua_pushinteger(l, static_cast<lua_Integer>(m_index));
        }
        else
        {
            lua_pushlstring(l, m_name.c_str(), m_name.size());
        }
    }

}  // namespace nau::lua
