// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "nau/network/napi/networking_transport.h"

namespace nau
{
    class ASIO_Connection;

    /**
     * @brief Provides ASIO-based data transfer mechanism.
     */
    class NetworkingTransportASIO : public INetworkingTransport
    {
    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] connection A pointer to the connection object.
         */
        NetworkingTransportASIO(ASIO_Connection* connection);

        /**
         * @brief Attempts to read received messages.
         *
         * @param [out] messages    A collection of received message. Empty if no messages.
         * @return                  Number of received messages, i.e. size of **messages**.
         */
        size_t read(eastl::vector<nau::NetworkingMessage>& messages) override;

        /**
         * @brief Attempts to send the specified message.
         *
         * @param [in] message  Message to send.
         * @return              `true` if the message can be sent, `false` otherwise.
         */
        bool write(const nau::NetworkingMessage& message) override;

        /**
         * @brief Checks if the connection is alive.
         *
         * @return `true` if the connection is still alive, `false` otherwise.
         */
        bool isConnected() override;

        /**
         * @brief Drops the connection.
         *
         * @return `true` if the connections has been dropped successfully, `false` otherwise.
         */
        bool disconnect() override;

        /**
         * @brief Retrieves the local endpoint as URI.
         *
         * @return Local endpoint in URI form.
         */
        const eastl::string& localEndPoint() const override;

        /**
         * @brief Retrieves the remote endpoint as URI.
         *
         * @return Remote endpoint in URI form.
         */
        const eastl::string& remoteEndPoint() const override;

    private:
        ASIO_Connection* m_connection;
    };
}  // namespace nau
