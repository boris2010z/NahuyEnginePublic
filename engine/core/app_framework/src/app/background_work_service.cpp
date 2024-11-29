// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/app/background_work_service.h"

#include <EASTL/unique_ptr.h>

#include "nau/async/work_queue.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/service/service.h"
#include "nau/threading/event.h"
#include "nau/threading/set_thread_name.h"

namespace nau
{
    class BackgroundWorkServiceImpl : public BackgroundWorkService
    {
        NAU_TYPEID(nau::BackgroundWorkServiceImpl)
        NAU_CLASS_BASE(BackgroundWorkService);

    public:
        BackgroundWorkServiceImpl()
        {
            m_thread = std::thread([](BackgroundWorkServiceImpl& self)
            {
                threading::setThisThreadName("Nau Background Work (Default)");
                self.m_isCompleted = false;
                scope_on_leave
                {
                    self.m_isCompleted = true;
                };

                while (self.m_isAlive)
                {
                    self.m_workQueue->poll(std::nullopt);
                }

                if (self.m_workQueue->is<IRuntimeComponent>())
                {
                    while (self.m_workQueue->as<IRuntimeComponent&>().hasWorks())
                    {
                        self.m_workQueue->poll(std::nullopt);
                    }
                }
            }, std::ref(*this));
        }

        ~BackgroundWorkServiceImpl()
        {
            m_isAlive = false;
            while (!m_isCompleted)
            {
                m_workQueue->notify();
                std::this_thread::yield();
            }

            m_thread.join();
            m_workQueue.reset();
        }

    private:
        async::Executor::Ptr getExecutor() override
        {
            return m_workQueue;
        }

        WorkQueue::Ptr m_workQueue = WorkQueue::create();
        std::thread m_thread;
        std::atomic<bool> m_isAlive = true;
        std::atomic<bool> m_isCompleted = false;
    };

    eastl::unique_ptr<BackgroundWorkService> createBackgroundWorkService()
    {
        return eastl::make_unique<BackgroundWorkServiceImpl>();
    }
}  // namespace nau
