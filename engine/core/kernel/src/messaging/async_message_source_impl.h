// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "./async_message_stream_impl.h"
#include "nau/messaging/async_message_stream.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/utils/cancellation.h"


namespace nau
{
    class AsyncMessageStreamImpl;

    class AsyncMessageSourceImpl final : public AsyncMessageSource
    {
        NAU_CLASS_(nau::AsyncMessageSourceImpl, AsyncMessageSource)

    public:
        AsyncMessageSourceImpl();

        AsyncMessageSourceImpl(const AsyncMessageSourceImpl&) = delete;

        AsyncMessageSourceImpl(AsyncMessageSourceImpl&&) = delete;

        ~AsyncMessageSourceImpl();

        void dispose() override;

        void setCancellation(Cancellation) override;

        bool hasSubscribers(eastl::string_view streamName) const override;

        AsyncMessageStream getStream(eastl::string_view streamName) override;

        // virtual subscribeInplace(Functor<void (const MessageEnvelope&)) = 0;

        void post(eastl::string_view streamName, RuntimeValue::Ptr) override;

        void unregisterStream(AsyncMessageStreamImpl& stream);

    private:
        struct StreamSubscribers
        {
            eastl::vector<nau::Ptr<AsyncMessageStreamImpl>> asyncStreams;

            bool hasAnySubscription() const
            {
                return !asyncStreams.empty();
            }
        };

        void cancelSubscriptions();

        eastl::unordered_map<eastl::string, StreamSubscribers> m_subscribers;
        mutable std::shared_mutex m_mutex;
        CancellationSubscription m_cancellationSubscription;
        RuntimeObjectRegistration m_disposeRegistration;
        bool m_isCancelled = false;
    };

}  // namespace nau
