// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/async/task.h"

#include "nau/threading/barrier.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/cancellation.h"

#include "helpers/runtime_guard.h"
#include "nau/test/helpers/stopwatch.h"

using namespace testing;
using namespace std::chrono_literals;

namespace nau::test
{

    TEST(TestCancellation, None)
    {
        auto cancellation = Cancellation::none();
        ASSERT_TRUE(cancellation.isEternal());
        ASSERT_FALSE(cancellation.isCancelled());
    }

    TEST(TestCancellation, CancellationSourceInit)
    {
        CancellationSource cancellationSource;
        ASSERT_TRUE(cancellationSource);
        ASSERT_FALSE(cancellationSource.isCancelled());
    }

    TEST(TestCancellation, cancel)
    {
        CancellationSource cancellationSource;
        Cancellation cancel = cancellationSource.getCancellation();

        ASSERT_FALSE(cancel.isCancelled());

        cancellationSource.cancel();

        ASSERT_TRUE(cancel.isCancelled());
    }

    TEST(TestCancellation, subscribe)
    {
        CancellationSource cancellationSource;
        Cancellation cancel = cancellationSource.getCancellation();

        bool called = false;

        auto subscription = cancel.subscribe([](void* ptr)
                                             {
                                                 *reinterpret_cast<bool*>(ptr) = true;
                                             },
                                             &called);

        cancellationSource.cancel();

        ASSERT_TRUE(called);
    }

    TEST(TestCancellation, SubscribeAfterCancel)
    {
        CancellationSource cancellationSource;
        Cancellation cancel = cancellationSource.getCancellation();
        cancellationSource.cancel();

        bool called = false;

        auto subscription = cancel.subscribe([](void* ptr)
                                             {
                                                 *reinterpret_cast<bool*>(ptr) = true;
                                             },
                                             &called);

        ASSERT_TRUE(called);
    }

    TEST(TestCancellation, Unsubscribe)
    {
        CancellationSource cancellationSource;
        Cancellation cancel = cancellationSource.getCancellation();

        bool called = false;

        auto subscription = cancel.subscribe([](void* ptr)
                                             {
                                                 *reinterpret_cast<bool*>(ptr) = true;
                                             },
                                             &called);

        subscription = nullptr;

        cancellationSource.cancel();

        ASSERT_FALSE(called);
    }

    TEST(TestCancellation, UnsubscribeAfterCancel)
    {
        CancellationSource cancellationSource;
        Cancellation cancel = cancellationSource.getCancellation();

        bool called = false;

        auto subscription = cancel.subscribe([](void* ptr)
                                             {
                                                 *reinterpret_cast<bool*>(ptr) = true;
                                             },
                                             &called);

        cancellationSource.cancel();

        subscription = nullptr;

        ASSERT_TRUE(called);
    }


    /**
        Test:
            subscribe from multiple threads
    */
    TEST(TestCancellation, ConcurrentSubscribe)
    {
        const size_t ThreadsCount = 10;
        const size_t SubscriptionPerThreads = 20;

        CancellationSource cancellationSource;

        std::atomic<size_t> counter = 0;

        std::vector<std::thread> threads;
        std::mutex mtx;
        std::vector<CancellationSubscription> subscriptions;

        threads.reserve(ThreadsCount);
        subscriptions.reserve(ThreadsCount * SubscriptionPerThreads);

        threading::Barrier barrier(ThreadsCount);

        for(size_t i = 0; i < ThreadsCount; ++i)
        {
            threads.emplace_back([&]
                                 {
                                     barrier.enter();

                                     auto cancellation = cancellationSource.getCancellation();
                                     std::vector<CancellationSubscription> threadSubscriptions;

                                     for(size_t x = 0; x < SubscriptionPerThreads; ++x)
                                     {
                                         threadSubscriptions.emplace_back(cancellation.subscribe([](void* data)
                                                                                                 {
                                                                                                     reinterpret_cast<std::atomic<size_t>*>(data)->fetch_add(1);
                                                                                                 },
                                                                                                 &counter));
                                     }

                                     lock_(mtx);
                                     for(auto& s : threadSubscriptions)
                                     {
                                         subscriptions.emplace_back(std::move(s));
                                     }
                                 });
        }

        for(auto& t : threads)
        {
            t.join();
        }

        cancellationSource.cancel();

        ASSERT_THAT(counter, Eq(ThreadsCount * SubscriptionPerThreads));
    }

    /**
        Test:
            Simultaneous subscription from several threads, when cancel is called at the same time
    */
    TEST(TestCancellation, ConcurrentSubscribeWhileCancelled)
    {
        const size_t ThreadsCount = 10;
        const size_t SubscriptionPerThreads = 50;

        CancellationSource cancellationSource;

        std::atomic<size_t> counter = 0;

        std::vector<std::thread> threads;
        std::mutex mtx;
        std::vector<CancellationSubscription> subscriptions;

        threads.reserve(ThreadsCount);
        subscriptions.reserve(ThreadsCount * SubscriptionPerThreads);

        threading::Barrier barrier(ThreadsCount + 1);

        for(size_t i = 0; i < ThreadsCount; ++i)
        {
            threads.emplace_back([&]
                                 {
                                     barrier.enter();

                                     auto cancellation = cancellationSource.getCancellation();
                                     std::vector<CancellationSubscription> threadSubscriptions;

                                     for(size_t x = 0; x < SubscriptionPerThreads; ++x)
                                     {
                                         threadSubscriptions.emplace_back(cancellation.subscribe([](void* data)
                                                                                                 {
                                                                                                     reinterpret_cast<std::atomic<size_t>*>(data)->fetch_add(1);
                                                                                                 },
                                                                                                 &counter));
                                     }

                                     lock_(mtx);
                                     for(auto& s : threadSubscriptions)
                                     {
                                         subscriptions.emplace_back(std::move(s));
                                     }
                                 });
        }

        barrier.enter();
        std::this_thread::sleep_for(1ms);
        cancellationSource.cancel();

        for(auto& t : threads)
        {
            t.join();
        }

        ASSERT_THAT(counter, Eq(ThreadsCount * SubscriptionPerThreads));
    }

    /**
        Test: 
            checking the situation when the subscription descriptor is destroyed inside a call to the cancel handler.
            Those callback is already called and at this moment the descriptor of the same subscription is reset/destroyed.
            Such situations should be handled normally: without deadlocks, crashes, etc.
    */
    TEST(TestCancellation, UnsubscribeWhileCancel)
    {
#if 0
        using namespace nau::async;
        using namespace std::chrono;

        constexpr size_t TasksCount = 10;
        const auto runtime = RuntimeGuard::create();

        const auto TaskFactory = [&]() -> Task<bool>
        {
            co_await Executor::getDefault();

            Task<> outterTask;

            {  // запускается Task<> который ждёт cancellation, который будет автоматически отменён внутри SCOPE_Leave.
                CancellationSource cancellationSource;
                scope_on_leave
                {
                    cancellationSource.cancel();
                };

                outterTask = [](Cancellation cancellation) -> Task<>
                {
                    CancellationSubscription subscription;
                    subscription = cancellation.subscribe([](void* ptr)
                                                          {
                                                              // т.к. вызов происходит в том же потоке, то здесь выполнение окажется на том же стеке, что и cancellationSource.cancel() выше.
                                                              reinterpret_cast<CancellationSubscription*>(ptr)->reset();
                                                          },
                                                          &subscription);

                    co_await Expiration{cancellation};

                    subscription = nullptr;
                }(cancellationSource.getCancellation());
            }

            co_await outterTask;

            co_return true;
        };

        constexpr auto WorkTime = 10ms;

        const auto tp = system_clock::now();

        do
        {
            std::vector<Task<bool>> tasks;
            tasks.reserve(TasksCount);

            for(size_t i = 0; i < TasksCount; ++i)
            {
                tasks.emplace_back(TaskFactory());
            }

            async::waitResult(async::whenAll(tasks)).ignore();

        } while((system_clock::now() - tp) <= WorkTime);

        // здесь не проверяется какое-либо состояние, но только то, что тест проходит без зависаний и крашей.
#endif
    }

    TEST(TestCancellation, Stress)
    {
        using namespace nau::async;
        using namespace std::chrono;

        constexpr size_t IsDeadFlag = 0x1;
        constexpr size_t IsAccesedFlag = 0x1 << 1;

        // flags: [dead, access | dead]
        using State = std::tuple<std::atomic_size_t, size_t>;

        constexpr size_t TasksCount = 600;
        const auto runtimeGuard = RuntimeGuard::create();

        const auto TaskFactory = [&]() -> Task<bool>
        {
            co_await Executor::getDefault();

            State state = {0, 0};
            Task<> outterTask;

            {
                CancellationSource cancellationSource;
                TaskSource<> taskSource;
                scope_on_leave
                {
                    taskSource.resolve();
                    cancellationSource.cancel();
                };

                outterTask = [](Cancellation cancellation, Task<> t, State& s) -> Task<>
                {
                    scope_on_leave
                    {
                        std::get<0>(s).store(0x1);
                    };

                    auto subscription = cancellation.subscribe([](void* ptr)
                                                               {
                                                                   auto& [bits1, bits2] = *reinterpret_cast<State*>(ptr);
                                                                   bits2 = bits1.load() | (0x1 << 1);  // 'IsAccesedFlag' cannot be implicitly captured because no default capture mode has been specified
                                                               },
                                                               &s);

                    co_await t;
                }(cancellationSource.getCancellation(), taskSource.getTask(), state);
            }

            co_await outterTask;

            auto bits = std::get<1>(state);

            const bool invokedAfterDead = ((bits & IsDeadFlag) == IsDeadFlag) && ((bits & IsAccesedFlag) == IsAccesedFlag);
            co_return !invokedAfterDead;
        };

        constexpr auto WorkTime = 50ms;

        const auto tp = system_clock::now();

        bool failed = false;

        do
        {
            std::vector<Task<bool>> tasks;
            tasks.reserve(TasksCount);

            for(size_t i = 0; i < TasksCount; ++i)
            {
                tasks.emplace_back(TaskFactory());
            }

            async::waitResult(async::whenAll(tasks)).ignore();
            failed = failed || !std::all_of(tasks.begin(), tasks.end(), [](Task<bool>& t)
                                            {
                                                return *t;
                                            });

        } while((system_clock::now() - tp) <= WorkTime);

        ASSERT_FALSE(failed);
    }



    TEST(TestExpiration, Never)
    {
        Expiration expire = Expiration::never();
        ASSERT_TRUE(expire.isEternal());
        ASSERT_FALSE(expire.isExpired());
    }

    TEST(TestExpiration, NeverWithNoneCancellation)
    {
        Expiration expire{Cancellation::none()};
        ASSERT_TRUE(expire.isEternal());
        ASSERT_FALSE(expire.isExpired());
    }

    TEST(TestExpiration, ExpireByCancellation)
    {
        CancellationSource cancel;
        Expiration expire{cancel.getCancellation()};
        ASSERT_FALSE(expire.isEternal());

        cancel.cancel();
        ASSERT_TRUE(expire.isExpired());
    }

    TEST(TestExpiration, ExpireByTimeout)
    {
        const auto Timeout = 10ms;

        Expiration expire{Timeout};
        ASSERT_FALSE(expire.isEternal());
        ASSERT_FALSE(expire.isExpired());

        std::this_thread::sleep_for(Timeout + 2ms);

        ASSERT_TRUE(expire.isExpired());
    }

    TEST(TestExpiration, SubscribeOnlyCancel)
    {
        CancellationSource cancel;
        Expiration expire{cancel.getCancellation()};

        bool callbackInvoked = false;

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               *reinterpret_cast<bool*>(data) = true;
                                           },
                                           &callbackInvoked);

        cancel.cancel();

        ASSERT_TRUE(callbackInvoked);
    }

    TEST(TestExpiration, SubscribeTimeout)
    {
        const auto runtimeGuard = RuntimeGuard::create();

        const auto Timeout = 10ms;

        Expiration expire{Timeout};

        bool callbackInvoked = false;

        const Stopwatch sw;

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               *reinterpret_cast<bool*>(data) = true;
                                           },
                                           &callbackInvoked);

        while(!expire.isExpired())
        {
            std::this_thread::sleep_for(2ms);
        }

        std::this_thread::sleep_for(10ms);

        ASSERT_TRUE(expire.isExpired());
        ASSERT_TRUE(callbackInvoked);
    }

    /**
        Test:
            Timeouted Expiration and cancellation.
    */
    TEST(TestExpiration, SubscribeCalledOnce)
    {
        const auto runtimeGuard = RuntimeGuard::create();

        const auto Timeout = 10ms;

        CancellationSource cancellation;
        Expiration expire{cancellation.getCancellation(), Timeout};

        std::atomic_size_t counter = 0;

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               reinterpret_cast<std::atomic_size_t*>(data)->fetch_add(1);
                                           },
                                           &counter);

        std::this_thread::sleep_for(Timeout / 2);
        cancellation.cancel();

        std::this_thread::sleep_for(Timeout + 10ms);

        ASSERT_TRUE(expire.isExpired());
        ASSERT_THAT(counter, Eq(1));
    }

    TEST(TestExpiration, SubscribeCalledOnce2)
    {
        const auto runtimeGuard = RuntimeGuard::create();

        const auto Timeout = 10ms;

        CancellationSource cancellation;
        Expiration expire{cancellation.getCancellation(), Timeout};

        std::atomic_size_t counter = 0;

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               reinterpret_cast<std::atomic_size_t*>(data)->fetch_add(1);
                                           },
                                           &counter);

        const auto MaxAwaitTime = 1s;
        const Stopwatch stopWatch;

        while(!expire.isExpired() && stopWatch.getTimePassed() < MaxAwaitTime)
        {
            std::this_thread::sleep_for(2ms);
        }

        // ASSERT_TRUE(expire.isExpired());

        // cancellation.cancel();

        // ASSERT_THAT(counter, Eq(1));
    }

    TEST(TestExpiration, NoCallbacksOnDestruct)
    {
        const auto runtimeGuard = RuntimeGuard::create();

        const auto Timeout = 1min;

        CancellationSource cancellation;
        Expiration expire{cancellation.getCancellation(), Timeout};

        std::atomic_size_t counter = 0;

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               reinterpret_cast<std::atomic_size_t*>(data)->fetch_add(1);
                                           },
                                           &counter);
    }

    // TODO NAU-2089
    /*
    TEST(TestExpiration, RuntimeShutdown)
    {
        // not implemented
        GTEST_SKIP();

        constexpr auto Timeout = 1min;
        constexpr size_t RepeatsCount = 10;

        std::atomic_size_t counter = 0;

        for(size_t i = 0; i < RepeatsCount; ++i)
        {
            const auto runtimeGuard = RuntimeGuard::create();

            CancellationSource cancellation;
            Expiration expire{cancellation.getCancellation(), Timeout};

            const auto subs = expire.subscribe([](void* data)
                                               {
                                                   reinterpret_cast<std::atomic_size_t*>(data)->fetch_add(1);
                                               },
                                               &counter);
        }

        ASSERT_THAT(counter, Eq(RepeatsCount));
    }
    */

    TEST(TestExpiration, SubscribeAfterExpire)
    {
        CancellationSource cancel;
        Expiration expire{cancel.getCancellation()};

        bool callbackInvoked = false;
        cancel.cancel();

        const auto subs = expire.subscribe([](void* data)
                                           {
                                               *reinterpret_cast<bool*>(data) = true;
                                           },
                                           &callbackInvoked);

        ASSERT_TRUE(callbackInvoked);
    }

    TEST(TestExpiration, Unsubscribe)
    {
        const auto runtimeGuard = RuntimeGuard::create();

        const auto Timeout = 40ms;

        CancellationSource cancellation;
        Expiration expire{cancellation.getCancellation(), Timeout};

        std::atomic_size_t counter = 0;

        auto subs = expire.subscribe([](void* data)
                                     {
                                         reinterpret_cast<std::atomic_size_t*>(data)->fetch_add(1);
                                     },
                                     &counter);

        const auto MaxAwaitTime = 5s;
        const Stopwatch stopWatch;

        while(!expire.isExpired() && stopWatch.getTimePassed() < MaxAwaitTime)
        {
            std::this_thread::sleep_for(1ms);
            if(subs)
            {
                subs = {};
            }
        }

        ASSERT_TRUE(expire.isExpired());

        cancellation.cancel();

        ASSERT_THAT(counter, Eq(0));
    }

    TEST(TestExpiration, Stress)
    {
#if 0
        using namespace Runtime;
        using namespace Runtime::Async;
        using namespace std::chrono;

        constexpr size_t IsDeadFlag = 0x1;
        constexpr size_t IsAccesedFlag = 0x1 << 1;

        // flags: [dead, access | dead]
        using State = std::tuple<std::atomic_size_t, size_t>;

        constexpr size_t TasksCount = 600;
        const RuntimeGuard runtime;

        const auto TaskFactory = [&]() -> Task<bool>
        {
            co_await Scheduler::GetDefault();

            State state = {0, 0};
            Task<> outterTask;

            {
                CancellationSource cancellationSource;
                TaskSource<> taskSource;
                SCOPE_Leave
                {
                    taskSource.Resolve();
                    cancellationSource.cancel();
                };

                outterTask = [](Cancellation cancellation, Task<> t, State& s) -> Task<>
                {
                    SCOPE_Leave
                    {
                        std::get<0>(s).store(0x1);
                    };

                    auto subscription = cancellation.subscribe([](void* ptr)
                                                               {
                                                                   auto& [bits1, bits2] = *reinterpret_cast<State*>(ptr);
                                                                   bits2 = bits1.load() | (0x1 << 1);  // 'IsAccesedFlag' cannot be implicitly captured because no default capture mode has been specified
                                                               },
                                                               &s);

                    co_await Async::WhenAny(Expiration{cancellation}, t);
                }(cancellationSource.getCancellation(), taskSource.GetTask(), state);
            }

            co_await outterTask;

            auto bits = std::get<1>(state);

            const bool invokedAfterDead = ((bits & IsDeadFlag) == IsDeadFlag) && ((bits & IsAccesedFlag) == IsAccesedFlag);
            co_return !invokedAfterDead;
        };

        constexpr auto WorkTime = 50ms;

        const auto tp = system_clock::now();

        bool failed = false;

        do
        {
            std::vector<Task<bool>> tasks;
            tasks.reserve(TasksCount);

            for(size_t i = 0; i < TasksCount; ++i)
            {
                tasks.emplace_back(TaskFactory());
            }

            Async::WaitResult(Async::WhenAll(tasks)).Ignore();
            failed = failed || !std::all_of(tasks.begin(), tasks.end(), [](Task<bool>& t)
                                            {
                                                return *t;
                                            });

        } while((system_clock::now() - tp) <= WorkTime);

        ASSERT_FALSE(failed);
#endif
    }

}  // namespace nau::test