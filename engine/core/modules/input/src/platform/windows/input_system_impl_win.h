// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/platform/windows/app/window_message_handler.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/input_system.h"

namespace nau::input
{
    class InputSystemImplWin final : public IWindowMessageHandler,
                                     public IWindowsApplicationMessageHandler,
                                     public IInputSourceManager,
                                     public IRttiObject
    {
        NAU_RTTI_CLASS(InputSystemImplWin, IWindowMessageHandler, IWindowsApplicationMessageHandler, IInputSourceManager, IRttiObject)

        virtual void setGetSources(nau::Functor<void(eastl::vector<eastl::shared_ptr<InputSource>>&)>) override;

    private:
        virtual bool handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
        virtual PreDispatchMsgResult preDispatchMsg(MSG& msg) override;
        virtual void postDispatchMsg(const MSG& msg) override;

        eastl::unordered_map<HWND, eastl::shared_ptr<InputSource>> m_windowMapping;
        HWND m_currentWindow = 0;
        nau::Functor<bool(InputSource*)> m_onInputSource;
        nau::Functor<void(eastl::vector<eastl::shared_ptr<InputSource>>&)> m_getSources;
    };
}  // namespace nau::input
