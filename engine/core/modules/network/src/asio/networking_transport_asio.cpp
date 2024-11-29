// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/network/asio/networking_transport_asio.h"

#include "nau/network/asio/networking_connection_asio.h"
#include "networking_asio_wrapper.h"

namespace nau
{
    NetworkingTransportASIO::NetworkingTransportASIO(ASIO_Connection* connection) :
        m_connection(connection)
    {
    }

    size_t NetworkingTransportASIO::read(eastl::vector<nau::NetworkingMessage>& messages)
    {
        messages.clear();
        nau::NetworkingMessage message;
        m_connection->read(message.buffer);
        if (message.buffer.size() > 0)
        {
            messages.push_back(message);
        }
        return messages.size();
    }

    bool NetworkingTransportASIO::write(const nau::NetworkingMessage& message)
    {
        m_connection->write(message.buffer);
        return true;
    }

    bool NetworkingTransportASIO::disconnect()
    {
        return m_connection->disconnect();
    }

    bool NetworkingTransportASIO::isConnected()
    {
        return m_connection->isConnected();
    }

    const eastl::string& NetworkingTransportASIO::localEndPoint() const
    {
        return m_connection->localEndPoint();
    }

    const eastl::string& NetworkingTransportASIO::remoteEndPoint() const
    {
        return m_connection->remoteEndPoint();
    }

}  // namespace nau
