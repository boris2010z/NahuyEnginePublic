// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "asio.hpp"
#include "nau/network/napi/networking.h"

namespace nau
{
    class NetworkingListenerASIO;
    class NetworkingConnectorASIO;

    /**
     *  @brief Provides interface for ASIO network context instance.
     */
    class NetworkingASIO : public INetworking
    {
    public:

        /**
         * @brief Default constructor.
         */
        NetworkingASIO();

        /**
         * @brief Destructor.
         */
        ~NetworkingASIO();

        /**
         * @brief Apply config string
         *
         * @param [in] data String data in implementation dependent format.
         * @return          `true`, if the string data has been successfully parsed and applied, `false` otherwise.
         */
        bool applyConfig(const eastl::string& data) override;

        /**
         * @brief Initializes networking context. Call this function once before the first update.
         *
         * @return `true` on success, `false` otherwise.
         */
        bool init() override;

        /**
         * @brief Shuts down the context and frees all resources.
         *
         * @return `true` on success, `false` otherwise.
        */
        bool shutdown() override;

        /**
         * @brief Updates state (polling). This function must be called in a loop.
         *
         * @return `true` on success, `false` otherwise.
         */
        bool update() override;

        /**
         * @brief Retrieves the context instance id. It has to be unique for each instance in each process.
         *
         * @return A reference to a INetworkingIdentity object.
         */
        const INetworkingIdentity& identity() const override;

        /**
         * @brief Creates a listener object.
         *
         * @return A pointer to the INetworkingListener instance or `NULL` in case of failure.
         */
        eastl::shared_ptr<INetworkingListener> createListener() override;

        /**
         * @brief Creates a connector object.
         *
         * @return A pointer to the INetworkingConnector instance or `NULL` in case of failure.
         */
        eastl::shared_ptr<INetworkingConnector> createConnector() override;

        /**
         * @brief Creates a NetworkingASIO instance.
         * 
         * @return A pointer to the created NetworkingASIO instance.
         */
        static eastl::unique_ptr<nau::INetworking> create()
        {
            return eastl::make_unique<NetworkingASIO>();
        }

    private:
        eastl::string m_identity;
        asio::io_context m_io_context;
        eastl::vector<eastl::shared_ptr<NetworkingListenerASIO>> m_listeners;
        eastl::vector<eastl::shared_ptr<NetworkingConnectorASIO>> m_connectors;
    };
}  // namespace nau
