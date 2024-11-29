// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/async/async_timer.h"

namespace nau::async
{
    namespace
    {
        ITimerManager::Ptr& getTimerManagerInstanceRef()
        {
            static ITimerManager::Ptr s_timerManagerInstance;
            return (s_timerManagerInstance);
        }
    }  // namespace

    void ITimerManager::setInstance(ITimerManager::Ptr instance)
    {
        NAU_ASSERT(!instance || !getTimerManagerInstanceRef(), "Timer manager instance already set");

        getTimerManagerInstanceRef() = std::move(instance);
    }

    ITimerManager& ITimerManager::getInstance()
    {
        auto& instance = getTimerManagerInstanceRef();
        NAU_FATAL(instance);

        return *instance;
    }

    bool ITimerManager::hasInstance()
    {
        return static_cast<bool>(getTimerManagerInstanceRef());
    }

}  // namespace nau::async
