// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "async_message_stream_impl.h"

#include "async_message_source_impl.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    AsyncMessageStreamImpl::AsyncMessageStreamImpl(AsyncMessageSourceImpl& source, eastl::string_view name) :
        m_source(&source),
        m_streamName(name)
    {
    }

    AsyncMessageStreamImpl::~AsyncMessageStreamImpl()
    {
        NAU_ASSERT(m_isCancelled);
        NAU_ASSERT(!m_source);
    }

    // AsyncMessageStreamImpl::operator bool()
    // {
    //     lock_(m_mutex);
    //     return m_source != nullptr;
    // }

    async::Task<RuntimeValue::Ptr> AsyncMessageStreamImpl::getNextMessage()
    {
        lock_(m_mutex);

        if(m_isCancelled)
        {
            return async::Task<RuntimeValue::Ptr>::makeRejected(NauMakeError("Object is disposed"));
        }

        if(m_messages.empty())
        {
            NAU_ASSERT(!m_awaiter);
            m_awaiter = async::TaskSource<RuntimeValue::Ptr>{};
            return m_awaiter.getTask();
        }

        RuntimeValue::Ptr message = std::move(m_messages.front());
        m_messages.erase(m_messages.begin());

        return async::Task<RuntimeValue::Ptr>::makeResolved(std::move(message));
    }

    void AsyncMessageStreamImpl::push(RuntimeValue::Ptr message)
    {
        lock_(m_mutex);
        if(m_isCancelled)
        {
            return;
        }

        if(m_awaiter)
        {
            m_awaiter.resolve(std::move(message));
            m_awaiter = nullptr;
        }
        else
        {
            m_messages.emplace_back(std::move(message));
        }
    }

    const eastl::string& AsyncMessageStreamImpl::getStreamName() const
    {
        return m_streamName;
    }

    void AsyncMessageStreamImpl::cancelFromSource(Error::Ptr error)
    {
        cancel(std::move(error), false);
    }

    void AsyncMessageStreamImpl::cancel()
    {
        cancel(NauMakeError("Stream cancelled"), true);
    }

    void AsyncMessageStreamImpl::cancel(Error::Ptr error, bool unregisterStream)
    {
        lock_(m_mutex);
        if(const bool alreadyCancelled = std::exchange(m_isCancelled, true); !alreadyCancelled)
        {
            m_messages.clear();
            if(m_awaiter)
            {
                m_awaiter.reject(std::move(error));
                m_awaiter = nullptr;
            }
        }

        if(auto source = std::exchange(m_source, nullptr); source && unregisterStream)
        {
            source->unregisterStream(*this);
        }

        m_source = nullptr;
    }

}  // namespace nau
