// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/utils/cancellation.h"

#include "nau/async/async_timer.h"
#include "nau/threading/lock_guard.h"
#include "nau/threading/spin_lock.h"

using namespace std::chrono;

namespace nau::rt_detail
{
    /**
     */
    class CancellationState
    {
    public:
        using Ptr = eastl::shared_ptr<CancellationState>;
        using CallbackFunc = void (*)(void*);

        ~CancellationState()
        {
            m_subscriptions.clear();
        }

        bool isCancelled() const
        {
            lock_(m_mutex);
            return m_isCancelled;
        }

        void cancel()
        {
            {
                lock_(m_mutex);
                if(const bool alreadyCancelled = std::exchange(m_isCancelled, true))
                {
                    return;
                }
            }
            // After state going to be cancelled, m_subscriptions will not be ever modified.
            // So m_mutex must not be used (also this can lead to deadlock).
            for(auto& subscription : m_subscriptions)
            {
                subscription();
            }
        }

        uintptr_t subscribe(CallbackFunc callback, void* data)
        {
            NAU_ASSERT(callback, "Callback expected to be not null");
            if(!callback)
            {
                return 0;
            }

            lock_(m_mutex);

            if(m_isCancelled)
            {
                callback(data);
                return 0;
            }

            // TODO: refactor to using unsigned as ID instead of address.
            m_subscriptions.emplace_back(callback, data);
            auto& subscription = m_subscriptions.back();
            return reinterpret_cast<uintptr_t>(&subscription);
        }

        void unsubscribe(uintptr_t handle)
        {
            if(handle == 0)
            {
                return;
            }

            SubscriptionEntry* const subscription = reinterpret_cast<SubscriptionEntry*>(handle);
            subscription->setUnsubscribed();
        }

        void setTimeout(std::chrono::milliseconds)
        {
            NAU_FAILURE_ALWAYS("CancellationState::SetTimeout is not implemented");
            // Halt("CancellationState::SetTimeout is not implemented");
        }

    private:
        struct SubscriptionEntry
        {
            CallbackFunc callback;
            void* callbackData;
            threading::RecursiveSpinLock mutex;

            SubscriptionEntry(CallbackFunc cb, void* cbData) :
                callback(cb),
                callbackData(cbData)
            {
            }

            void setUnsubscribed()
            {
                lock_(mutex);
                callback = nullptr;
                callbackData = nullptr;
            }

            void operator()()
            {
                lock_(mutex);
                if(callback)
                {
                    callback(callbackData);
                }
            }
        };

        static_assert(sizeof(SubscriptionEntry*) == sizeof(uintptr_t));

        mutable std::mutex m_mutex;
        bool m_isCancelled = false;
        eastl::list<SubscriptionEntry> m_subscriptions;
    };

    /**
     */
    class ExpirationState
    {
    public:
        using Ptr = eastl::shared_ptr<ExpirationState>;
        using Clock = std::chrono::system_clock;
        using CallbackFunc = void (*)(void*);

        ExpirationState() = default;

        ExpirationState(CancellationState::Ptr cancellation, std::chrono::milliseconds timeout) :
            m_cancellation(std::move(cancellation)),
            m_timeout(timeout)

        {
        }

        ExpirationState(CancellationState::Ptr cancellation) :
            m_cancellation(std::move(cancellation))
        {
        }

        ExpirationState(std::chrono::milliseconds timeout) :
            m_timeout(timeout)
        {
        }

        ~ExpirationState()
        {
            resetSubscriptions();
        }

        bool isExpired() const
        {
            return (m_cancellation && m_cancellation->isCancelled()) || timeIsOver();
        }

        bool isEternal() const
        {
            return !(m_cancellation || m_timeout);
        }

        uintptr_t Subscribe(CallbackFunc callback, void* data)
        {
            NAU_ASSERT(callback, "Callback expected to be not null");

            if(!callback)
            {
                return 0;
            }

            lock_(m_mutex);

            if(isExpired())
            {
                callback(data);
                return 0;
            }

            // TODO: refactor to using unsigned as ID instead of address.
            m_subscriptions.emplace_back(callback, data);
            auto& entry = m_subscriptions.back();
            const uintptr_t handle = reinterpret_cast<uintptr_t>(&entry);

            installSubscriptions();

            return handle;
        }

        void unsubscribe(uintptr_t handle)
        {
            if(handle == 0)
            {
                return;
            }

            SubscriptionEntry* const subscription = reinterpret_cast<SubscriptionEntry*>(handle);
            subscription->setUnsubscribed();
        }

        std::optional<std::chrono::milliseconds> getTimeout() const
        {
            return m_timeout;
        }

    private:
        struct SubscriptionEntry
        {
            CallbackFunc callback;
            void* callbackData;
            threading::SpinLock mutex;

            SubscriptionEntry(CallbackFunc cb, void* cbData) :
                callback(cb),
                callbackData(cbData)
            {
            }

            void setUnsubscribed()
            {
                lock_(mutex);
                callback = nullptr;
                callbackData = nullptr;
            }

            void operator()()
            {
                lock_(mutex);
                if(callback)
                {
                    callback(callbackData);
                }
            }
        };

        inline bool timeIsOver() const
        {
            return m_timeout && (m_timeout->count() == 0 || *m_timeout <= (Clock::now() - m_creationTime));
        }

        void installSubscriptions()
        {
            using namespace std::chrono;

            if(isExpired())
            {
                return;
            }

            if(m_timeout && !m_timerSubscription)
            {
                const auto timePass = duration_cast<milliseconds>(Clock::now() - m_creationTime);

                if(timePass < *m_timeout)
                {
                    const int64_t timeLeft = static_cast<int64_t>(m_timeout->count()) - static_cast<int64_t>(timePass.count());
                    NAU_ASSERT(timeLeft > 0);

                    m_timerSubscription = async::invokeAfter(milliseconds{timeLeft}, [](void* ptr) noexcept
                                                             {
                                                                 auto& self = *reinterpret_cast<ExpirationState*>(ptr);
                                                                 self.invokeCallbacks();
                                                             },
                                                             this);
                }
            }

            if(m_cancellation && !m_cancellationSubscription)
            {
                m_cancellationSubscription = m_cancellation->subscribe([](void* ptr)
                                                                       {
                                                                           auto& self = *reinterpret_cast<ExpirationState*>(ptr);
                                                                           self.invokeCallbacks();
                                                                       },
                                                                       this);
            }
        }

        void resetSubscriptions()
        {
            if(m_timerSubscription)
            {
                async::cancelInvokeAfter(*m_timerSubscription);
                m_timerSubscription.reset();
            }

            if(m_cancellationSubscription)
            {
                NAU_ASSERT(m_cancellation);
                m_cancellation->unsubscribe(*m_cancellationSubscription);
                m_cancellationSubscription.reset();
            }

            m_subscriptions.clear();
        }

        void invokeCallbacks()
        {
            {
                lock_(m_mutex);
                if(std::exchange(m_callbacksAreInvoked, true) == true)
                {
                    return;
                }
            }

            // From here m_subscriptions will never be change:
            // - Unsubscribe does not modify container
            // - Subscribe will be completed at started, because of IsExpired() == true.

            for(auto& subscription : m_subscriptions)
            {
                subscription();
            }
        }

        const Clock::time_point m_creationTime = Clock::now();
        const CancellationState::Ptr m_cancellation;
        const std::optional<std::chrono::milliseconds> m_timeout;

        mutable threading::SpinLock m_mutex;
        bool m_callbacksAreInvoked = false;
        eastl::list<SubscriptionEntry> m_subscriptions;

        std::optional<uintptr_t> m_timerSubscription;
        std::optional<uintptr_t> m_cancellationSubscription;
    };

}  // namespace nau::rt_detail

namespace nau
{

    CancellationSubscription::~CancellationSubscription()
    {
        reset();
    }

    CancellationSubscription::CancellationSubscription() = default;

    CancellationSubscription::CancellationSubscription(CancellationSubscription&& other) :
        m_cancellation{std::move(other.m_cancellation)},
        m_subscriptionHandle{other.m_subscriptionHandle}
    {
        other.m_subscriptionHandle = 0;
    }

    CancellationSubscription& CancellationSubscription::operator=(CancellationSubscription&& other)
    {
        if(m_cancellation && m_subscriptionHandle != 0)
        {
            m_cancellation->unsubscribe(m_subscriptionHandle);
        }

        m_cancellation = std::move(other.m_cancellation);
        m_subscriptionHandle = std::exchange(other.m_subscriptionHandle, 0);

        return *this;
    }

    CancellationSubscription& CancellationSubscription::operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    CancellationSubscription::operator bool() const
    {
        return m_cancellation && m_subscriptionHandle != 0;
    }

    CancellationSubscription::CancellationSubscription(rt_detail::CancellationState::Ptr cancellation, uintptr_t handle) :
        m_cancellation{std::move(cancellation)},
        m_subscriptionHandle{handle}
    {
    }

    void CancellationSubscription::reset()
    {
        if(m_cancellation && m_subscriptionHandle != 0)
        {
            auto cancellation = std::exchange(m_cancellation, nullptr);
            const auto handle = std::exchange(m_subscriptionHandle, 0);
            cancellation->unsubscribe(handle);
        }
    }

    Cancellation::~Cancellation() = default;

    Cancellation::Cancellation() = default;

    Cancellation::Cancellation(Cancellation&&) = default;

    Cancellation::Cancellation(const Cancellation&) = default;

    Cancellation::Cancellation(rt_detail::CancellationState::Ptr state) :
        m_cancellation{std::move(state)}
    {
    }

    Cancellation& Cancellation::operator=(Cancellation&&) = default;

    Cancellation& Cancellation::operator=(const Cancellation&) = default;

    bool Cancellation::isCancelled() const
    {
        return m_cancellation && m_cancellation->isCancelled();
    }

    bool Cancellation::isEternal() const
    {
        return !m_cancellation;
    }

    CancellationSubscription Cancellation::subscribe(void (*callback)(void*), void* callbackData)
    {
        if(m_cancellation)
        {
            auto handle = m_cancellation->subscribe(callback, callbackData);
            return {m_cancellation, handle};
        }

        return {};
    }

    Cancellation Cancellation::none()
    {
        return Cancellation{nullptr};
    }

    CancellationSource::~CancellationSource() = default;

    CancellationSource::CancellationSource(std::nullptr_t)
    {
    }

    CancellationSource::CancellationSource() :
        m_cancellation{eastl::make_shared<rt_detail::CancellationState>()}
    {
    }

    CancellationSource::CancellationSource(CancellationSource&&) = default;

    CancellationSource& CancellationSource::operator=(CancellationSource&&) = default;

    CancellationSource::operator bool() const
    {
        return static_cast<bool>(m_cancellation);
    }

    Cancellation CancellationSource::getCancellation()
    {
        NAU_ASSERT(m_cancellation);
        return m_cancellation ? Cancellation{m_cancellation} : Cancellation{};
    }

    bool CancellationSource::isCancelled() const
    {
        NAU_ASSERT(m_cancellation);
        return m_cancellation && m_cancellation->isCancelled();
    }

    void CancellationSource::cancel()
    {
        NAU_ASSERT(m_cancellation);
        if(m_cancellation)
        {
            m_cancellation->cancel();
        }
    }

    void CancellationSource::SetTimeoutInternal(std::chrono::milliseconds timeout)
    {
        NAU_ASSERT(m_cancellation);
        if(m_cancellation)
        {
            m_cancellation->setTimeout(timeout);
        }
    }

    ExpirationSubscription::~ExpirationSubscription()
    {
        if(m_expiration)
        {
            m_expiration->unsubscribe(m_subscriptionHandle);
        }
    }

    ExpirationSubscription::ExpirationSubscription() = default;

    ExpirationSubscription::ExpirationSubscription(ExpirationSubscription&& other) :
        m_expiration{std::move(other.m_expiration)},
        m_subscriptionHandle{other.m_subscriptionHandle}
    {
        other.m_subscriptionHandle = 0;
    }

    ExpirationSubscription& ExpirationSubscription::operator=(ExpirationSubscription&& other)
    {
        if(m_expiration)
        {
            m_expiration->unsubscribe(m_subscriptionHandle);
        }

        m_expiration = std::move(other.m_expiration);
        m_subscriptionHandle = other.m_subscriptionHandle;
        other.m_subscriptionHandle = 0;

        NAU_ASSERT(!other.m_expiration);

        return *this;
    }

    ExpirationSubscription::operator bool() const
    {
        return m_expiration && m_subscriptionHandle != 0;
    }

    ExpirationSubscription::ExpirationSubscription(rt_detail::ExpirationState::Ptr expiration, uintptr_t handle) :
        m_expiration{std::move(expiration)},
        m_subscriptionHandle{handle}
    {
    }

    Expiration::Expiration() = default;

    Expiration::Expiration(Cancellation cancellation, std::chrono::milliseconds timeout) :
        m_expiration(eastl::make_shared<rt_detail::ExpirationState>(std::move(cancellation.m_cancellation), timeout))
    {
    }

    Expiration::Expiration(Cancellation cancellation) :
        m_expiration(eastl::make_shared<rt_detail::ExpirationState>(std::move(cancellation.m_cancellation)))
    {
    }

    Expiration::Expiration(std::chrono::milliseconds timeout) :
        m_expiration(eastl::make_shared<rt_detail::ExpirationState>(timeout))
    {
    }

    bool Expiration::isExpired() const
    {
        return m_expiration && m_expiration->isExpired();
    }

    bool Expiration::isEternal() const
    {
        return !m_expiration || m_expiration->isEternal();
    }

    ExpirationSubscription Expiration::subscribe(void (*callback)(void*), void* callbackData)
    {
        if(m_expiration)
        {
            auto handle = m_expiration->Subscribe(callback, callbackData);
            return {m_expiration, handle};
        }

        return {};
    }

    std::optional<std::chrono::milliseconds> Expiration::getTimeout() const
    {
        return m_expiration ? m_expiration->getTimeout() : std::nullopt;
    }

    Expiration Expiration::never()
    {
        return {};
    }

}  // namespace nau
