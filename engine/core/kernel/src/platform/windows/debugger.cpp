// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/debug/debugger.h"

#include <crtdbg.h>
#include <debugapi.h>

namespace nau::debug
{
    bool isRunningUnderDebugger()
    {
        return ::IsDebuggerPresent() == TRUE;
    }

}  // namespace nau::debug
