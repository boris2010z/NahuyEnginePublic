// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/async/task_collection.h"

#include "nau/diag/assertion.h"
#include "nau/threading/lock_guard.h"

namespace nau::async
{
    namespace
    {
        bool operator==(CoreTaskPtr& t, const CoreTask* taskPtr)
        {
            return getCoreTask(t) == taskPtr;
        }
    }  // namespace

    TaskCollection::TaskCollection() = default;

    TaskCollection::~TaskCollection()
    {
#ifdef NAU_ASSERT_ENABLED
        lock_(m_mutex);
        NAU_FATAL(!m_closeAwaiter);
        NAU_FATAL(m_tasks.empty(), "All task collection must be be awaited");
#endif
    }

    bool TaskCollection::isEmpty() const
    {
        lock_(m_mutex);
        return m_tasks.empty();
    }

    void TaskCollection::pushInternal(CoreTaskPtr task)
    {
        NAU_ASSERT(task);
        if (!task)
        {
            return;
        }

        CoreTask* const coreTask = async::getCoreTask(task);
        NAU_FATAL(coreTask);
        if (coreTask->isReady())
        {
            // Even if a task is not ready here, it may become so immediately after (or in the process of) placing it in the collection.
            // Therefore, the scope of the mutex lock must be limited.
            return;
        }

        {
            lock_(m_mutex);
            NAU_ASSERT(!m_isDisposed);
            m_tasks.emplace_back(std::move(task));
        }

        coreTask->setReadyCallback([](void* selfPtr, void* taskPtr) noexcept
        {
            NAU_FATAL(selfPtr);
            NAU_FATAL(taskPtr);
            auto& self = *reinterpret_cast<TaskCollection*>(selfPtr);
            auto completedTask = reinterpret_cast<CoreTask*>(taskPtr);

            lock_(self.m_mutex);

            auto& tasks = self.m_tasks;

            auto iter = std::find_if(tasks.begin(), tasks.end(), [completedTask](TaskEntry& task)
            {
                return task == completedTask;
            });
            NAU_ASSERT(iter != tasks.end());
            if (iter != tasks.end())
            {
                tasks.erase(iter);
            }

            if (tasks.empty() && self.m_closeAwaiter)
            {
                NAU_ASSERT(self.m_isDisposing);
                auto awaiter = std::move(self.m_closeAwaiter);
                awaiter.resolve();
            }
        }, this, coreTask);
    }

    Task<> TaskCollection::awaitCompletionInternal(bool dispose)
    {
        {
            lock_(m_mutex);
            const bool alreadyDisposing = std::exchange(m_isDisposing, true);

            NAU_ASSERT(!alreadyDisposing, "TaskCollection::disposeAsync called multiply times");
            if (alreadyDisposing)
            {
                co_return;
            }
        }

        scope_on_leave
        {
            lock_(m_mutex);
            NAU_ASSERT(m_isDisposing);
            m_isDisposing = false;
        };

        do
        {
            Task<> awaiterTask;

            {
                lock_(m_mutex);
                if (m_tasks.empty())
                {
                    if (dispose)
                    {
                        NAU_ASSERT(!m_isDisposed);
                        m_isDisposed = true;
                    }
                    break;
                }

                NAU_ASSERT(!m_closeAwaiter);
                m_closeAwaiter = {};
                awaiterTask = m_closeAwaiter.getTask();
            }

            co_await awaiterTask;
        } while (true);
    }

}  // namespace nau::async
