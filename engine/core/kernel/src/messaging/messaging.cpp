// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/messaging/messaging.h"
#include "nau/diag/assertion.h"
#include "nau/service/service_provider.h"


namespace nau
{
    AsyncMessageSubscription::AsyncMessageSubscription() = default;

    AsyncMessageSubscription::AsyncMessageSubscription(AsyncMessageSubscription&& other) :
        m_task(std::move(other.m_task)),
        m_cancellationSource(std::move(other.m_cancellationSource))
    {
        NAU_ASSERT(!other.m_task);
        NAU_ASSERT(!other.m_cancellationSource);
    }

    AsyncMessageSubscription& AsyncMessageSubscription::operator=(AsyncMessageSubscription&& other)
    {
        NAU_ASSERT(!m_task, "Can not re-assign initialized subscription");

        m_task = std::move(other.m_task);
        m_cancellationSource = std::move(other.m_cancellationSource);

        NAU_ASSERT(!other.m_task);
        NAU_ASSERT(!other.m_cancellationSource);

        return *this;
    }

    AsyncMessageSubscription::~AsyncMessageSubscription()
    {
        dispose();
    }

    AsyncMessageSubscription& AsyncMessageSubscription::operator=(std::nullptr_t)
    {
        dispose();
        return *this;
    }

    AsyncMessageSubscription::operator bool() const
    {
        return static_cast<bool>(m_task);
    }

    void AsyncMessageSubscription::dispose()
    {
        if(!m_task)
        {
            NAU_ASSERT(!m_cancellationSource);
            return;
        }

        NAU_FATAL(m_cancellationSource);
        scope_on_leave
        {
            NAU_ASSERT(!m_task);
        };

        std::exchange(m_cancellationSource, nullptr).cancel();

        if(auto task = std::move(m_task); !task.isReady())
        {
            // log warn:
            task.detach();
        }
    }

    async::Task<> AsyncMessageSubscription::disposeAsync()
    {
        if(!m_task)
        {
            NAU_ASSERT(!m_cancellationSource);
            return async::Task<>::makeResolved();
        }

        NAU_FATAL(m_cancellationSource);
        scope_on_leave
        {
            NAU_ASSERT(!m_task);
        };

        std::exchange(m_cancellationSource, nullptr).cancel();

        return std::move(m_task);
    }

    AsyncMessageSource& getBroadcaster()
    {
        return getServiceProvider().get<AsyncMessageSource>();
    }

}  // namespace nau
