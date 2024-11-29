// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include <limits>
#include <string_view>

#include "lua_toolkit/lua_headers.h"
#include "nau/diag/assertion.h"


namespace nau::lua
{
    inline constexpr int InvalidLuaIndex = std::numeric_limits<int>::min();

    class ChildVariableKey
    {
    public:
        static ChildVariableKey makeFromStack(lua_State*, int);

        static inline ChildVariableKey noKey()
        {
            return nullptr;
        }

        ChildVariableKey() = default;

        ChildVariableKey(std::nullptr_t) :
            ChildVariableKey()
        {
        }

        ChildVariableKey(int indexedKey) :
            m_index(indexedKey)
        {
        }

        ChildVariableKey(std::string_view namedKey) :
            m_name(namedKey)
        {
        }

        bool isIndexed() const
        {
            return m_index != InvalidLuaIndex;
        }

        explicit operator bool() const
        {
            return isIndexed() || !m_name.empty();
        }

        operator int() const
        {
            NAU_ASSERT(isIndexed());
            return m_index;
        }

        const std::string& getName() const
        {
            NAU_ASSERT(!isIndexed());
            NAU_ASSERT(m_name.length() > 0);
            return (m_name);
        }

        std::string_view asString() const;

        bool operator==(const ChildVariableKey& other) const
        {
            NAU_ASSERT(static_cast<bool>(*this));
            NAU_ASSERT(static_cast<bool>(other));

            if(other.isIndexed() != this->isIndexed())
            {
                return false;
            }

            return isIndexed() ? this->m_index == other.m_index : this->m_name == other.m_name;
        }

        bool operator!=(const ChildVariableKey& other) const
        {
            return !this->operator==(other);
        }

        bool operator==(std::string_view) const;

        void push(lua_State*) const;

    private:
        int m_index = InvalidLuaIndex;
        mutable std::string m_name;
    };

}  // namespace nau::lua
