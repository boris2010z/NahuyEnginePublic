// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "./async_message_source_impl.h"

#include "./async_message_stream_impl.h"

namespace nau
{
    AsyncMessageSourceImpl::AsyncMessageSourceImpl() :
        m_disposeRegistration(*this)
    {
    }

    AsyncMessageSourceImpl::~AsyncMessageSourceImpl()
    {
        cancelSubscriptions();
    }

    void AsyncMessageSourceImpl::dispose()
    {
        cancelSubscriptions();
        m_disposeRegistration = nullptr;
    }

    void AsyncMessageSourceImpl::setCancellation(Cancellation cancellation)
    {
        NAU_ASSERT(!m_cancellationSubscription, "Cancellation can be set only once");

        m_cancellationSubscription = cancellation.subscribe([](void* selfPtr)
                                                            {
                                                                auto& self = *reinterpret_cast<AsyncMessageSourceImpl*>(selfPtr);
                                                                self.cancelSubscriptions();
                                                            },
                                                            this);
    }

    bool AsyncMessageSourceImpl::hasSubscribers(eastl::string_view streamName) const
    {
        const std::shared_lock lock{m_mutex};

        auto entry = m_subscribers.find(eastl::string{streamName});
        return entry != m_subscribers.end() && entry->second.hasAnySubscription();
    }

    AsyncMessageStream AsyncMessageSourceImpl::getStream(eastl::string_view streamName)
    {
        lock_(m_mutex);

        auto stream = rtti::createInstance<AsyncMessageStreamImpl>(*this, streamName);
        if(m_isCancelled)
        {
            // LOG_WARN(Core::Format::format("GetStream({}) for cancelled message source", name));
            stream->cancelFromSource(NauMakeError("Subscription is cancelled"));
        }
        else
        {
            m_subscribers[eastl::string{streamName}].asyncStreams.push_back(stream);
        }

        return AsyncMessageStream{std::move(stream)};
    }

    void AsyncMessageSourceImpl::post(eastl::string_view streamName, RuntimeValue::Ptr message)
    {
        eastl::vector<nau::Ptr<AsyncMessageStreamImpl>> receivers;

        {
            const std::shared_lock lock{m_mutex};

            if(m_isCancelled)
            {
                NAU_ASSERT(m_subscribers.empty());
                // NAU_FAILURE_ALWAYS("Post message through closed stream:({})", name);
                return;
            }

            if(auto subscribers = m_subscribers.find(eastl::string{streamName}); subscribers != m_subscribers.end())
            {
                receivers.reserve(subscribers->second.asyncStreams.size());
                for(nau::Ptr<AsyncMessageStreamImpl>& stream : subscribers->second.asyncStreams)
                {
                    receivers.push_back(stream);
                }
            }
        }

        for(auto&& stream : receivers)
        {
            stream->push(message);
        }
    }

    void AsyncMessageSourceImpl::unregisterStream(AsyncMessageStreamImpl& stream)
    {
        lock_(m_mutex);

        if(auto entry = m_subscribers.find(stream.getStreamName()); entry != m_subscribers.end())
        {
            auto& streams = entry->second.asyncStreams;

            auto iter = std::find_if(streams.begin(), streams.end(), [&stream](const nau::Ptr<AsyncMessageStreamImpl>& streamPtr)
                                     {
                                         return streamPtr.get() == &stream;
                                     });

            if(iter != streams.end())
            {
                // streams.erase(iter);
                if(&(*iter) != &streams.back())
                {
                    *iter = std::move(streams.back());
                }

                streams.resize(streams.size() - 1);
            }
        }
    }

    void AsyncMessageSourceImpl::cancelSubscriptions()
    {
        const auto tempSubscribers = EXPR_Block->decltype(m_subscribers)
        {
            lock_(m_mutex);

            if(const bool alreadyCancelled = std::exchange(m_isCancelled, true))
            {
                return {};
            }

            m_cancellationSubscription = nullptr;
            return std::move(m_subscribers);
        };

        auto error = NauMakeError("Subscription is cancelled");
        for(auto& [key, data] : tempSubscribers)
        {
            for(const nau::Ptr<AsyncMessageStreamImpl>& stream : data.asyncStreams)
            {
                stream->cancelFromSource(error);
            }
        }
        // lock_(m_mutex);
        // if(const bool alreadyCancelled = std::exchange(m_isCancelled, true))
        //{
        //     return;
        // }

        // auto error = NauMakeError("Subscription is cancelled");
        // for(auto& [key, data] : m_subscribers)
        //{
        //     for(const nau::Ptr<AsyncMessageStreamImpl>& stream : data.asyncStreams)
        //     {
        //         stream->cancelFromSource(error);
        //     }
        // }
    }

    AsyncMessageSource::Ptr AsyncMessageSource::create()
    {
        return rtti::createInstance<AsyncMessageSourceImpl>();
    }
}  // namespace nau
