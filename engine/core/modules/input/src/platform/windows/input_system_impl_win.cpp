// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "input_system_impl_win.h"

#include "input_system_impl.h"
#include "nau/service/service_provider.h"


namespace nau::input
{
    void InputSystemImplWin::setGetSources(nau::Functor<void(eastl::vector<eastl::shared_ptr<InputSource>>& sources)> getSources)
    {
        m_getSources = std::move(getSources);
    }

    IWindowsApplicationMessageHandler::PreDispatchMsgResult InputSystemImplWin::preDispatchMsg(MSG& msg)
    {
        return PreDispatchMsgResult::Normal;
    }

    void InputSystemImplWin::postDispatchMsg(const MSG& msg)
    {
        auto& insys = getServiceProvider().get<InputSystemImpl>();
        if (msg.hwnd != 0)
        {
            // Check if user want to deal with multiple input sources
            if (m_getSources && m_currentWindow != msg.hwnd)
            {
                if (!m_windowMapping.contains(msg.hwnd))
                {
                    // Update sources list
                    m_windowMapping.clear();
                    eastl::vector<eastl::shared_ptr<InputSource>> sources;
                    m_getSources(sources);
                    for (auto& source : sources)
                    {
                        m_windowMapping.emplace((HWND)source->m_handle, std::move(source));
                    }
                }
                if (!m_windowMapping.contains(msg.hwnd))
                {
                    // Skip messages from not specefied sources
                    return;
                }
                insys.setInputSource(m_windowMapping[msg.hwnd]->m_name);
                std::cout << "Input source set " << m_windowMapping[msg.hwnd]->m_name.c_str() << std::endl;
                m_currentWindow = msg.hwnd;
            }
        }
        insys.getGainput().HandleMessage(msg);
    }

    bool InputSystemImplWin::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_SIZE)
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            auto& insys = getServiceProvider().get<InputSystemImpl>();
            insys.getGainput().SetDisplaySize(width, height);
        }
        return false;
    }
}  // namespace nau::input
