// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "helpers/runtime_guard.h"

#include "nau/async/async_timer.h"
#include "nau/async/executor.h"
#include "nau/async/thread_pool_executor.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_component.h"

#include "nau/utils/scope_guard.h"

namespace nau::test
{
    class RuntimeGuardImpl final : public RuntimeGuard
    {
    public:
        RuntimeGuardImpl()
        {
            using namespace nau::async;

            ITimerManager::setDefaultInstance();
            m_defaultExecutor = createThreadPoolExecutor(4);
            Executor::setDefault(m_defaultExecutor);
        }

        ~RuntimeGuardImpl()
        {
            resetInternal();
        }

        void reset() override
        {
            resetInternal();
        }

    private:
        void resetInternal()
        {
            using namespace std::chrono_literals;
            using namespace nau::async;

            scope_on_leave
            {
                Executor::setDefault(nullptr);
                ITimerManager::releaseInstance();
            };

            std::vector<IRttiObject*> components;
            if (ITimerManager::hasInstance())
            {
                components.push_back(&ITimerManager::getInstance());
            }

            components.push_back(m_defaultExecutor.get());

            for(auto* const component : components)
            {
                if(auto* const disposable = component->as<IDisposable*>())
                {
                    disposable->dispose();
                }
            }

            const auto componentInUse = [](IRttiObject* component)
            {
                IRuntimeComponent* const runtimeComponent = component ? component->as<IRuntimeComponent*>() : nullptr;
                if (!runtimeComponent)
                {
                    return false;
                }

                if (runtimeComponent->hasWorks())
                {
                    return true;
                }

                const auto* const refCounted = runtimeComponent->as<const IRefCounted*>();
                return  refCounted && refCounted->getRefsCount() > 1;
            };

            const auto anyComponentInUse = [&components, componentInUse]
            {
                return std::any_of(components.begin(), components.end(), componentInUse);
            };

            while(anyComponentInUse())
            {
                std::this_thread::sleep_for(50ms);
            }
        }

        async::Executor::Ptr m_defaultExecutor;
    };

    RuntimeGuard::Ptr RuntimeGuard::create()
    {
        return eastl::make_unique<RuntimeGuardImpl>();
    }
}  // namespace nau::test
