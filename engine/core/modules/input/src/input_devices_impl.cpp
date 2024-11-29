// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "input_devices_impl.h"

namespace nau
{
    GAKeyboardDevice::GAKeyboardDevice(gainput::InputManager& inputManager) :
        m_inputManager(inputManager)
    {
        m_id = m_inputManager.CreateDevice<gainput::InputDeviceKeyboard>();
    }

    eastl::string GAKeyboardDevice::getName() const
    {
        return "keyboard";
    }

    IInputDevice::Type GAKeyboardDevice::getType() const
    {
        return Type::Keyboard;
    }

    unsigned GAKeyboardDevice::getKeysNum() const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            auto* state = device->GetInputState();
            if (state != nullptr)
            {
                return state->GetButtonCount();
            }
        }
        return 0;
    }

    unsigned GAKeyboardDevice::getAxisNum() const
    {
        return 0;
    }

    eastl::string GAKeyboardDevice::getKeyName(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        char buffer[64];
        device->GetButtonName(keyId, buffer, 63);
        return buffer;
    }

    eastl::string GAKeyboardDevice::getAxisName(unsigned axisId) const
    {
        return "";
    }

    unsigned GAKeyboardDevice::getKeyByName(const eastl::string_view keyName) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            return device->GetButtonByName(keyName.data());
        }
        return UINT_MAX;
    }

    unsigned GAKeyboardDevice::getAxisByName(const eastl::string_view axisName) const
    {
        return UINT_MAX;
    }

    IInputDevice::KeyState GAKeyboardDevice::getKeyState(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            if (device->GetBool(keyId))
            {
                return KeyState::Pressed;
            }
        }
        return KeyState::Released;
    }

    float GAKeyboardDevice::getAxisState(unsigned axisId) const
    {
        return 0.f;
    }

    GAMouseDevice::GAMouseDevice(gainput::InputManager& inputManager) :
        m_inputManager(inputManager)
    {
        m_id = m_inputManager.CreateDevice<gainput::InputDeviceMouse>();
    }

    eastl::string GAMouseDevice::getName() const
    {
        return "mouse";
    }

    IInputDevice::Type GAMouseDevice::getType() const
    {
        return Type::Mouse;
    }

    unsigned GAMouseDevice::getKeysNum() const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            auto* state = device->GetInputState();
            if (state != nullptr)
            {
                return state->GetButtonCount();
            }
        }
        return 0;
    }

    unsigned GAMouseDevice::getAxisNum() const
    {
        return 3;
    }

    eastl::string GAMouseDevice::getKeyName(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        char buffer[64];
        device->GetButtonName(keyId, buffer, 63);
        return buffer;
    }

    eastl::string GAMouseDevice::getAxisName(unsigned axisId) const
    {
        return "";
    }

    unsigned GAMouseDevice::getKeyByName(const eastl::string_view keyName) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        return device->GetButtonByName(keyName.data());
    }

    unsigned GAMouseDevice::getAxisByName(const eastl::string_view axisName) const
    {
        return UINT_MAX;
    }

    IInputDevice::KeyState GAMouseDevice::getKeyState(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            if (device->GetBool(keyId))
            {
                return KeyState::Pressed;
            }
        }
        return KeyState::Released;
    }

    float GAMouseDevice::getAxisState(unsigned axisId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            return device->GetFloat(gainput::MouseAxisX + axisId);
        }
        return 0.f;
    }
}  // namespace nau