// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/debug/debugger.h"
#include "nau/diag/assertion.h"
#include "nau/diag/device_error.h"
#include "nau/diag/logging.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service_provider.h"

namespace nau::diag
{

    class WindowsDeviceError final : public IDeviceError
    {
    public:
        NAU_RTTI_CLASS(nau::diag::WindowsDeviceError, IDeviceError)
        NAU_DECLARE_SINGLETON_MEMOP(WindowsDeviceError)

        FailureActionFlag handleFailure(const FailureData& data) override
        {
            nau::string message;

            if(!data.message.empty())
            {
                message = nau::utils::format("Failed \"{}\". At [{}] {}({}). Error: {}.\nMessage: \"{}\"",
                                              data.condition.data(),
                                              data.source.functionName.data(),
                                              data.source.filePath.data(),
                                              data.source.line.value_or(0),
                                              data.error,
                                              data.message.data());
            }
            else
            {
                message = nau::string::format(u8"Failed \"{}\". At [{}] {}({}). Error: {}.",
                                              (const char8_t*)data.condition.data(),
                                              (const char8_t*)data.source.functionName.data(),
                                              (const char8_t*)data.source.filePath.data(),
                                              data.source.line.value_or(0),
                                              data.error);
            }

            if (hasLogger())
            {
                getLogger().logMessage(LogLevel::Critical,
                                       {"Fatal"},
                                       data.source,
                                       nau::utils::format(message.data()));
            }

            return data.kind == AssertionKind::Fatal ? FailureAction::DebugBreak | FailureAction::Abort : FailureAction::DebugBreak;
        }
    };

    IDeviceError::Ptr createDefaultDeviceError()
    {
        return eastl::make_unique<WindowsDeviceError>();
    }

}  // namespace nau::diag