// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/messaging/async_message_stream.h"

#include "./async_message_stream_impl.h"

namespace nau
{
    AsyncMessageStream::AsyncMessageStream() = default;

    AsyncMessageStream::AsyncMessageStream(nau::Ptr<AsyncMessageStreamImpl>&& stream) :
        m_stream(std::move(stream))
    {
    }

    AsyncMessageStream::AsyncMessageStream(AsyncMessageStream&& other) noexcept
        :
        m_stream{std::move(other.m_stream)}
    {
        NAU_ASSERT(!other.m_stream);
    }

    AsyncMessageStream::~AsyncMessageStream()
    {
        reset();
    }

    AsyncMessageStream& AsyncMessageStream::operator=(AsyncMessageStream&& other) noexcept
    {
        reset();
        NAU_ASSERT(!m_stream);

        m_stream = std::move(other.m_stream);
        NAU_ASSERT(!other.m_stream);

        return *this;
    }

    AsyncMessageStream& AsyncMessageStream::operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    AsyncMessageStream::operator bool() const
    {
        return static_cast<bool>(m_stream);
    }

    eastl::string_view AsyncMessageStream::getStreamName() const
    {
        NAU_ASSERT(m_stream);

        return m_stream ? eastl::string_view{m_stream->getStreamName()} : eastl::string_view{};
    }

    async::Task<RuntimeValue::Ptr> AsyncMessageStream::getNextMessage()
    {
        if(!m_stream)
        {
            return async::Task<RuntimeValue::Ptr>::makeRejected(NauMakeError("Invalid message stream object"));
        }

        return m_stream->getNextMessage();
    }

    void AsyncMessageStream::reset()
    {
        if(auto stream = std::exchange(m_stream, nullptr))
        {
            stream->cancel();
        }
    }
}  // namespace nau
