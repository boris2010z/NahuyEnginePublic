// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/network/asio/networking_asio.h"

#include "nau/diag/logging.h"
#include "nau/network/asio/networking_connection_asio.h"
#include "nau/network/asio/networking_identity_asio.h"

namespace nau
{
    NetworkingASIO::NetworkingASIO()
    {
    }
    NetworkingASIO::~NetworkingASIO() = default;

    bool NetworkingASIO::applyConfig(const eastl::string& data)
    {
        return true;
    }

    bool NetworkingASIO::init()
    {
        NAU_LOG_DEBUG("NetworkingASIO::init Ok");
        return true;
    }
    bool NetworkingASIO::shutdown()
    {
        NAU_LOG_DEBUG("NetworkingASIO::shutdown");
        return true;
    }
    bool NetworkingASIO::update()
    {
        m_io_context.poll();
        return true;
    }
    const INetworkingIdentity& NetworkingASIO::identity() const
    {
        static NetworkingIdentityASIO id("ASIO");
        return id;
    }
    eastl::shared_ptr<INetworkingListener> NetworkingASIO::createListener()
    {
        auto listener = eastl::make_shared<NetworkingListenerASIO>(m_io_context);
        m_listeners.push_back(listener);
        return listener;
    }
    eastl::shared_ptr<INetworkingConnector> NetworkingASIO::createConnector()
    {
        auto connector = eastl::make_shared<NetworkingConnectorASIO>(m_io_context);
        m_connectors.push_back(connector);
        return connector;
    }
}  // namespace nau
