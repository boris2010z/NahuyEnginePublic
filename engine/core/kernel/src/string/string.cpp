// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/string/string.h"

namespace nau::string_literals
{
    _NODISCARD string operator"" _ns(const char8_t* _Str, size_t _Len)
    {
        return string(_Str);
    }
    _NODISCARD string operator"" _ns(const char16_t* _Str, size_t _Len)
    {
        return string(_Str);
    }
    _NODISCARD string operator"" _ns(const char32_t* _Str, size_t _Len)
    {
        return string(_Str);
    }
}  // namespace nau::string_literals