// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>

namespace nau
{
    /**
     * @brief Enumerates known address types.
     */
    enum class NetworkingAddressType : unsigned
    {
        Unknown,
        IPv4,
        IPv6,
        Custom
    };

    /**
     * @brief Encapsulates address of some type.
     */
    struct NetworkingAddress
    {
        /**
         * @brief Address type.
         */
        NetworkingAddressType type;

        /**
         * @brief Address text representation.
         */
        eastl::string address;
    };
}  // namespace nau
