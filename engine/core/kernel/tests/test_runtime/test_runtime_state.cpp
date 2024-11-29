// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/async/task.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/threading/event.h"

namespace nau::test
{
    class TestRuntimeState : public testing::Test
    {
    protected:
        TestRuntimeState()
        {
        }

        ~TestRuntimeState()
        {
            shutdownAndWait();
        }

        size_t shutdownAndWait(Functor<void()> callback = {})
        {
            using namespace std::chrono_literals;

            if(!m_runtimeState)
            {
                return 0;
            }

            size_t stepsCount = 0;

            auto runtimeState = std::move(m_runtimeState);
            auto shutdown = runtimeState->shutdown();
            if(callback)
            {
                callback();
            }
            while(shutdown())
            {
                std::this_thread::sleep_for(50ms);
                ++stepsCount;
            }

            return stepsCount;
        }

    private:
        RuntimeState::Ptr m_runtimeState = RuntimeState::create();
    };

    TEST_F(TestRuntimeState, TestCreateReset)
    {
    }

    TEST_F(TestRuntimeState, CompleteTimerWhileShutdown)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        bool coroDestructed = false;
        bool coroCompleted = false;

        auto timer = [](bool& destructed, bool& completed) -> Task<>
        {
            scope_on_leave
            {
                destructed = true;
            };

            co_await 1h;
            completed = true;
        }(coroDestructed, coroCompleted);

        shutdownAndWait();

        ASSERT_TRUE(timer.isReady());
        ASSERT_TRUE(coroDestructed);
        ASSERT_FALSE(coroCompleted);
    }

    TEST_F(TestRuntimeState, CompleteAsyncTasksWhileShutdown)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t TasksCount = 1000;
        constexpr size_t IterationsPerTask = 300;

        std::atomic<size_t> counter = 0;
        threading::Event signal(threading::Event::ResetMode::Manual);

        auto taskFactory = [](threading::Event& signal, std::atomic<size_t>& counter) -> Task<>
        {
            co_await Executor::getDefault();

            signal.wait();

            for(size_t i = 0; i < IterationsPerTask; ++i)
            {
                if (++counter % 3 == 0)
                {
                    co_await Executor::getDefault();
                }
            }
        };

        eastl::vector<Task<>> tasks;

        for(size_t i = 0; i < TasksCount; ++i)
        {
            tasks.push_back(taskFactory(signal, std::ref(counter)));
        }

        [[maybe_unused]]
        const size_t shutdownCounter = shutdownAndWait([&signal]
                        {
                            signal.set();
                        });

        const bool allTaskCompleted = eastl::all_of(tasks.begin(), tasks.end(), [](const Task<>& t)
                                                    {
                                                        return t.isReady();
                                                    });

        ASSERT_TRUE(allTaskCompleted);
        //ASSERT_GT(shutdownCounter , 0);
        ASSERT_EQ(counter, IterationsPerTask * TasksCount);
    }

}  // namespace nau::test
