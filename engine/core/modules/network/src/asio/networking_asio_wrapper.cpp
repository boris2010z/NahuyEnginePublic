// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "networking_asio_wrapper.h"

#include <nau/diag/logging.h>

using namespace asio;
using namespace asio::ip;

namespace nau
{
    static void endpointToString(const asio::ip::tcp::endpoint& endpoint, eastl::string& str)
    {
        auto address = endpoint.address().to_string();
        auto port = endpoint.port();
        str = "tcp://";
        str += address.c_str();
        str += ":";
        str += eastl::to_string(port);
        str += "/";
    }

    ASIO_Connection::ASIO_Connection(tcp::socket s) :
        m_socket(std::move(s))
    {
    }

    ASIO_Connection::ASIO_Connection(io_context& io_context) :
        m_socket(tcp::socket(io_context))
    {
    }

    void ASIO_Connection::connect(tcp::endpoint endpoint, std::function<void(std::error_code)> onConnect)
    {
        m_socket.async_connect(endpoint, [onConnect](std::error_code ec)
        {
            if(!ec)
            {
                NAU_LOG_DEBUG("ASIO_Connection::connect Connected");
            }
            else
            {
                NAU_LOG_DEBUG(nau::utils::format("ASIO_Connection::connect Connect error {}", ec.message().c_str()));
            }
            onConnect(ec);
        });
    }

    bool ASIO_Connection::disconnect()
    {
        if (m_socket.is_open())
        {
            m_socket.close();
            return true;
        }
        return false;
    }

    bool ASIO_Connection::isConnected() const
    {
        return m_socket.is_open();
    }

    void ASIO_Connection::write(const BytesBuffer& buffer)
    {
        size_t size = buffer.size();
        asio::streambuf::mutable_buffers_type buf = m_write_buffer.prepare(size);
        std::memcpy(buf.data(), buffer.data(), size);
        m_write_buffer.commit(size);
        doWriteBuffer();
    }

    void ASIO_Connection::read(BytesBuffer& buffer)
    {
        if (m_socket.is_open())
        {
            auto toRead = m_socket.available();
            if (m_socket.available() > 0)
            {
                std::error_code error;
                m_socket.read_some(m_read_buffer.prepare(m_socket.available()), error);
                if (!error)
                {
                    m_read_buffer.commit(toRead);
                    size_t size = m_read_buffer.size();
                    buffer.resize(size);
                    std::memcpy(buffer.data(), m_read_buffer.data().data(), size);
                    m_read_buffer.consume(size);
                }
            }
        }
    }

    const eastl::string& ASIO_Connection::localEndPoint() const
    {
        if (isConnected())
        {
            endpointToString(m_socket.local_endpoint(), m_localEndPoint);
        }
        return m_localEndPoint;
    }
    const eastl::string& ASIO_Connection::remoteEndPoint() const
    {
        if (isConnected())
        {
            endpointToString(m_socket.remote_endpoint(), m_remoteEndPoint);
        }
        return m_remoteEndPoint;
    }

    void ASIO_Connection::doWriteBuffer()
    {
        if (m_write_buffer.size() > 0)
        {
            const char* str = (const char*)m_write_buffer.data().data();
            m_socket.async_write_some(asio::buffer(m_write_buffer.data(), m_write_buffer.size()),
                                      [this](const asio::error_code& error, std::size_t bytes_transferred)
            {
                this->writeHandler(error, bytes_transferred);
            });
        }
    }

    void ASIO_Connection::writeHandler(const asio::error_code& error, std::size_t bytes_transferred)
    {
        if (error)
        {
            NAU_LOG_DEBUG(nau::utils::format("ASIO_Connection::writeHandler error {}", error.message().c_str()));
        }
        else
        {
            NAU_LOG_DEBUG(nau::utils::format("ASIO_Connection::writeHandler bytes {}", bytes_transferred));
            m_write_buffer.consume(bytes_transferred);
            doWriteBuffer();
        }
    }

    ASIO_Acceptor::ASIO_Acceptor(io_context& io_context, short port, std::function<void(tcp::socket socket)> onAccept) :
        m_acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
        m_context(io_context)
    {
        m_acceptCallback = onAccept;
        do_accept();
    }

    void ASIO_Acceptor::do_accept()
    {
        m_acceptor.async_accept(
            [this](std::error_code ec, tcp::socket socket)
        {
            if (m_acceptCallback)
            {
                m_acceptCallback(std::move(socket));
            }
            do_accept();
        });
    }

    void ASIO_Acceptor::close()
    {
        m_acceptor.close();
    }
}  // namespace nau
