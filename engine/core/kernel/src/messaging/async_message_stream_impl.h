// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/messaging/async_message_stream.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    class AsyncMessageSourceImpl;

    /**
     */
    class AsyncMessageStreamImpl final : public IRefCounted
    {
        NAU_CLASS_(nau::AsyncMessageStreamImpl, IRefCounted)
    public:
        AsyncMessageStreamImpl(AsyncMessageSourceImpl&, eastl::string_view name);

        AsyncMessageStreamImpl(const AsyncMessageStreamImpl&) = delete;

        ~AsyncMessageStreamImpl();

//        explicit operator bool();

        async::Task<RuntimeValue::Ptr> getNextMessage();

        void push(RuntimeValue::Ptr);

        const eastl::string& getStreamName() const;

        void cancelFromSource(Error::Ptr error);

        //void cancelFromClient();
        void cancel();

    private:
        enum class CancelSource
        {
            FromSource,
            FromClient
        };

        void cancel(Error::Ptr error, bool unregisterStream);

        AsyncMessageSourceImpl* m_source;
        eastl::string m_streamName;
        std::mutex m_mutex;

        async::TaskSource<RuntimeValue::Ptr> m_awaiter = nullptr;
        eastl::list<RuntimeValue::Ptr> m_messages;
        bool m_isCancelled = false;
    };

}  // namespace nau
