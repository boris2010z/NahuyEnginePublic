// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "helpers/runtime_guard.h"
#include "nau/async/async_timer.h"
#include "nau/async/task_collection.h"
#include "nau/async/work_queue.h"
#include "nau/threading/event.h"

namespace nau::test
{
    class TestTaskCollection : public testing::Test
    {
    protected:
        void TearDown() override
        {
            m_runtimeGuard.reset();
        }

        RuntimeGuard::Ptr m_runtimeGuard = RuntimeGuard::create();
        async::TaskCollection m_taskCollection;
    };

    /**
        Test:
            Spawn multiply tasks from different threads and push all tasks into task collection.
            Each task will await signal-task that will be set right after all threads finish their works (i.e. all threads create and push task into collection).
     */

    TEST_F(TestTaskCollection, PushAndWaitAll)
    {
        using namespace nau::async;
        using namespace testing;
        using namespace std::chrono_literals;

        constexpr size_t ThreadsCount = 20;
        constexpr size_t TasksPerThread = 20;
        constexpr size_t ExpectedCounter = ThreadsCount * TasksPerThread;

        // Task sources for all tasks from all threads.
        std::vector<TaskSource<>> taskSources;
        std::atomic_size_t counterEnter = 0;
        std::atomic_size_t counterReadyAwaiter = 0;
        std::atomic_size_t counterReadyAll = 0;
        std::mutex mutex;
        std::vector<std::thread> threads;

        for (size_t i = 0; i < ThreadsCount; ++i)
        {
            threads.emplace_back([this, &taskSources, &counterEnter, &counterReadyAwaiter, &counterReadyAll, &mutex]()
            {
                std::vector<TaskSource<>> threadTasks;

                for (size_t j = 0; j < TasksPerThread; ++j)
                {
                    Task<> task = [&threadTasks](std::atomic_size_t& counterEnter, std::atomic_size_t& counterReadyAwaiter, std::atomic_size_t& counterReadyAll) -> Task<>
                    {
                        ++counterEnter;
                        auto awaiter = threadTasks.emplace_back().getTask();

                        co_await awaiter;
                        ++counterReadyAwaiter;

                        co_await 2ms;
                        ++counterReadyAll;
                    }(std::ref(counterEnter), std::ref(counterReadyAwaiter), std::ref(counterReadyAll));

                    m_taskCollection.push(std::move(task));
                }

                std::lock_guard lock(mutex);

                std::move(threadTasks.begin(), threadTasks.end(), std::back_inserter(taskSources));
            });
        }

        // wait while all threads perform their work - i.e. create task and push it into task collection.
        std::for_each(threads.begin(), threads.end(), [](std::thread& thread)
        {
            thread.join();
        });

        // at this moment all tasks from all threads are live inside collection, but none of them if finished.
        ASSERT_FALSE(m_taskCollection.isEmpty());
        ASSERT_THAT(taskSources.size(), Eq(ExpectedCounter));

        // allow tasks to finish their works. All tasks from this point are going to be finished.
        std::for_each(taskSources.begin(), taskSources.end(), [](TaskSource<>& t)
        {
            t.resolve();
        });

        // await all pushed tasks.
        async::waitResult(m_taskCollection.disposeAsync()).ignore();

        ASSERT_TRUE(m_taskCollection.isEmpty());
        ASSERT_THAT(counterReadyAll, Eq(ExpectedCounter));
    }

    /**
        Test:
            Check that we can spawn new tasks, while collection is closed.
            1. run set of tasks ;
            2. each task will be blocked by awaiter
            3. close the collection: TaskCollection::close inside calls for waitForAll;
            4. release blocker task to allow to run and finish for tasks that was running on first step;
            5. at this moment collection already closed and awaits while all spawned tasks are finished and at the same time new tasks will be added into collections
            6. ensure that all tasks are finished and no dead locks occur .
    */

    TEST_F(TestTaskCollection, RunTasksWhileClosing)
    {
        using namespace nau::async;
        using namespace testing;
        using namespace std::chrono_literals;

        constexpr size_t TasksCount = 5;
        constexpr size_t SubTasksCount = 20;
        ;
        constexpr size_t ExpectedCounterValue = TasksCount * SubTasksCount;

        std::vector<TaskSource<>> tasks;
        std::atomic_size_t counter{0};

        for (size_t i = 0; i < TasksCount; ++i)
        {
            auto task = [](Task<> signalTask, TaskCollection& collection, size_t subTaskCount, std::atomic_size_t& counter) -> Task<>
            {
                co_await signalTask;

                for (size_t i = 0; i < subTaskCount; ++i)
                {
                    co_await 1ms;
                    auto subTask = [](std::atomic_size_t& counter) -> Task<>
                    {
                        co_await 1ms;
                        ++counter;
                    }(std::ref(counter));

                    collection.push(std::move(subTask));
                }
            }(tasks.emplace_back().getTask(), std::ref(m_taskCollection), SubTasksCount, std::ref(counter));

            m_taskCollection.push(std::move(task));
        }

        auto closeTask = m_taskCollection.disposeAsync();

        for (auto& taskSource : tasks)
        {
            taskSource.resolve();
        }

        waitResult(std::ref(closeTask)).ignore();
        ASSERT_THAT(counter, Eq(ExpectedCounterValue));
    }

    /**
     */
    TEST_F(TestTaskCollection, PushReadyTask)
    {
        using namespace nau::async;

        m_taskCollection.push(Task<>::makeResolved());
        ASSERT_TRUE(m_taskCollection.isEmpty());
    }

    /**
        Test:
            Check for adding tasks that can be completed very quickly:
            there is can be a task that will be completed during the call of the TaskCollection::push itself.
     */
    TEST_F(TestTaskCollection, PushFastCompletingTasks)
    {
        using namespace nau::async;

        constexpr size_t IterationCount = 7000;

        std::atomic<bool> complete = false;
        WorkQueue::Ptr worker = WorkQueue::create();

        std::thread workerThread([&worker, &complete]
        {
            while (!complete)
            {
                worker->poll(std::nullopt);
            }
        });

        scope_on_leave
        {
            complete = true;
            worker->notify();
            workerThread.join();
        };

        size_t counter = 0;

        std::thread senderThread([&]
        {
            for (unsigned i = 0; i < IterationCount; ++i)
            {
                auto task = [](WorkQueue::Ptr& worker, size_t& counter) -> Task<>
                {
                    co_await worker;
                    ++counter;
                }(worker, counter);

                m_taskCollection.push(std::move(task));
            }
        });

        senderThread.join();
        async::waitResult(m_taskCollection.disposeAsync()).ignore();

        ASSERT_EQ(counter, IterationCount);
    }

}  // namespace nau::test
