// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/diag/assertion.h"
#include "nau/diag/device_error.h"
#include "nau/memory/singleton_memop.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::test
{
    class AssertTestDeviceError final : public diag::IDeviceError
    {
        NAU_RTTI_CLASS(AssertTestDeviceError, diag::IDeviceError)

    public:
        diag::FailureActionFlag handleFailure(const diag::FailureData& data) final
        {
            if(data.kind == diag::AssertionKind::Default)
            {
                ++m_errorCounter;
            }
            else
            {
                ++m_fatalErrorCounter;
            }

            return diag::FailureAction::None;
        }

        void resetErrorCounters()
        {
            m_errorCounter = 0;
            m_fatalErrorCounter = 0;
        }

        size_t getErrorCount() const
        {
            return m_errorCounter;
        }

        size_t getFatalErrorCount() const
        {
            return m_fatalErrorCounter;
        }

        bool hasNoErrors() const
        {
            return m_errorCounter == 0 && m_fatalErrorCounter == 0;
        }

        static AssertTestDeviceError* deviceError;
    private:
        std::atomic_uint32_t m_errorCounter = 0;
        std::atomic_uint32_t m_fatalErrorCounter = 0;
    };

}  // namespace nau::test

//  #undef PLATFORM_BREAK
//  #define PLATFORM_BREAK 
//  #undef PLATFORM_ABORT
//  #define PLATFORM_ABORT 
