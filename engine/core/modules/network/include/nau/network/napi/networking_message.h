// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <EASTL/vector.h>

#include "nau/memory/bytes_buffer.h"

namespace nau
{
    /**
     * @brief Encapsulates a networking message which is a byte sequence of known size.
     */
    struct NetworkingMessage
    {
        /**
         * @brief Containes message byte buffer.
         */
        BytesBuffer buffer;

        /**
         * @brief Default constructor.
         */
        NetworkingMessage() = default;

        /** 
         * @brief Destructor.
         */
        ~NetworkingMessage() = default;

        /**
         * @brief Copy constructor.
         * 
         * @param [in] other Intance to copy from.
         */
        NetworkingMessage(const NetworkingMessage& other)
        {
            size_t size = other.buffer.size();
            buffer = BytesBuffer(size);
            std::memcpy(buffer.data(), other.buffer.data(), size);
        }

        /**
         * @brief Copy-assigment operator.
         * 
         * @param [in] other    Instance to assign.
         * @return              Modified object.
         */
        NetworkingMessage& operator=(const NetworkingMessage& other)
        {
            return *this = NetworkingMessage(other);
        }

        /**
         * @brief Move constructor.
         * 
         * @param [in] other Instance to move from.
         */
        NetworkingMessage(NetworkingMessage&& other) noexcept = default;

        /**
         * @brief Move-assigment operator
         * 
         * @param [in] other    Instance to assign.
         * @return              Modified object.
         */
        NetworkingMessage& operator=(NetworkingMessage&& other) noexcept = default;

        /**
         * @brief Constructs message from string.
         * 
         * @note This is a naive constructor for debug purposes.
         * 
         * @param [in] str String to convert in to a message
        */
        NetworkingMessage(const eastl::string_view& str)
        {
            size_t length = str.length();
            buffer.resize(length);
            std::byte* data = buffer.data();
            std::memcpy(data, str.begin(), length);
        }

        /**
         * @brief Constructs message from u8string.
         * 
         * @note This is a naive constructor for debug purposes.
         * 
         * @param [in] str String to convert in to a message.
         */
        NetworkingMessage(const eastl::u8string_view& str)
        {
            size_t length = str.length();
            buffer.resize(length);
            std::byte* data = buffer.data();
            std::memcpy(data, str.begin(), length);
        }
    };
}  // namespace nau
