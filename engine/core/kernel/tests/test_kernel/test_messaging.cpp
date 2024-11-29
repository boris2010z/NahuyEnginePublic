// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.



#include "nau/messaging/messaging.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/utils/functor.h"

namespace nau::test
{
    namespace
    {
        struct MessageData
        {
            NAU_TYPEID(nau::test::MessageData)

            NAU_CLASS_FIELDS(
                CLASS_FIELD(id)
            )

            unsigned id;
        };

        NAU_DECLARE_SIGNAL_MESSAGE(TestMessage, "test.stream_1");
        NAU_DECLARE_MESSAGE(TestTypesMessage, "test.stream_2", MessageData);

        struct Destructible
        {
            Functor<void()> destructorCallback;

            Destructible() = default;

            Destructible(Destructible&&) = default;

            template <typename Callable>
            Destructible(Callable callback) :
                destructorCallback(std::move(callback))
            {
            }

            ~Destructible()
            {
                if(destructorCallback)
                {
                    destructorCallback();
                }
            }
        };
    }  // namespace

    class Test_Messaging : public ::testing::Test
    {
    protected:
        static inline const eastl::string TestStream1Name = "test.stream_1";

        ~Test_Messaging()
        {
            shutdownRuntime();
            m_runtime->completeShutdown();
        }

        void shutdownRuntime()
        {
            auto shutdown = m_runtime->shutdown(false);

            while(shutdown())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        AsyncMessageSource& broadcaster() const
        {
            return *m_broadcaster;
        }

        const RuntimeState::Ptr m_runtime = RuntimeState::create();
        const AsyncMessageSource::Ptr m_broadcaster = AsyncMessageSource::create();
    };

    TEST_F(Test_Messaging, AsyncMessageSubscriptionTraits)
    {
        static_assert(std::is_move_constructible_v<AsyncMessageSubscription>);
        static_assert(!std::is_copy_constructible_v<AsyncMessageSubscription>);

        static_assert(std::is_move_assignable_v<AsyncMessageSubscription>);
        static_assert(!std::is_copy_assignable_v<AsyncMessageSubscription>);
    }

    TEST_F(Test_Messaging, SubscribeNoPost)
    {
        auto subscription = TestMessage.subscribe(
            broadcaster(), []
            {
            },
            async::Executor::getDefault());
    }

    TEST_F(Test_Messaging, HandlerDestruction)
    {
        std::atomic_bool destructed = false;
        std::atomic_bool handlerCalled = false;

        Destructible destructible([&destructed]
                                  {
                                      destructed = true;
                                  });

        auto subscription = TestMessage.subscribe(
            broadcaster(), [&handlerCalled, destructible = std::move(destructible)]
            {
                handlerCalled = true;
            },
            async::Executor::getDefault());

        shutdownRuntime();

        ASSERT_TRUE(destructed);
        ASSERT_FALSE(handlerCalled);
    }

    TEST_F(Test_Messaging, PosTypedMessage)
    {
        const unsigned ExpectedValue = 77;

        threading::Event signal;
        unsigned receivedValue = 0;

        auto subscription = TestTypesMessage.subscribe(broadcaster(), [&signal, &receivedValue](const MessageData& msg)
        {
            receivedValue = msg.id;
            signal.set();
        }, async::Executor::getDefault());

        TestTypesMessage.post(broadcaster(), {ExpectedValue});

        signal.wait();

        ASSERT_EQ(receivedValue, ExpectedValue);
    }

}  // namespace nau::test
