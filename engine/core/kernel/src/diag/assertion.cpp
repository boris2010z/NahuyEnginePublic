// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/diag/assertion.h"

#include "nau/debug/debugger.h"
#include "nau/diag/device_error.h"

// #include "nau/debug/debugger.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::diag
{
    namespace
    {

        IDeviceError::Ptr& getDeviceErrorRef()
        {
            static IDeviceError::Ptr s_deviceError;  //= eastl::make_unique<DefaultDeviceErrorImpl>();
            return (s_deviceError);
        }

    }  // namespace

    void setDeviceError(IDeviceError::Ptr newDeviceError, IDeviceError::Ptr* prevDeviceError)
    {
        if(prevDeviceError)
        {
            *prevDeviceError = std::move(getDeviceErrorRef());
        }

        getDeviceErrorRef() = std::move(newDeviceError);
    }

    IDeviceError* getDeviceError()
    {
        return getDeviceErrorRef().get();
    }

}  // namespace nau::diag

namespace nau::diag_detail
{
    diag::FailureActionFlag raiseFailure(uint32_t error, diag::AssertionKind kind, diag::SourceInfo source, eastl::string_view condition, eastl::string_view message)
    {
        using namespace nau::diag;

        static thread_local unsigned threadRaiseFailureCounter = 0;

        if(threadRaiseFailureCounter > 0)
        {
            NAU_PLATFORM_BREAK;
            NAU_PLATFORM_ABORT;
        }

        ++threadRaiseFailureCounter;

        scope_on_leave
        {
            --threadRaiseFailureCounter;
        };

        if(auto& customDeviceError = diag::getDeviceErrorRef(); customDeviceError)
        {
            const FailureData failureData{
                error,
                kind,
                source,
                condition,
                message};

            return customDeviceError->handleFailure(failureData);
        }

        return kind == AssertionKind::Default ? FailureAction::DebugBreak : (FailureAction::DebugBreak | FailureAction::Abort);
    }
}  // namespace nau::diag_detail