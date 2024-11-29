// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include <gainput/gainput.h>

#include "nau/input_system.h"

namespace nau
{
    class GAKeyboardDevice : public IInputDevice
    {
    public:
        GAKeyboardDevice(gainput::InputManager& inputManager);

        eastl::string getName() const override;

        Type getType() const override;

        unsigned getKeysNum() const override;

        unsigned getAxisNum() const override;

        eastl::string getKeyName(unsigned keyId) const override;

        eastl::string getAxisName(unsigned axisId) const override;

        unsigned getKeyByName(const eastl::string_view keyName) const override;

        unsigned getAxisByName(const eastl::string_view axisName) const override;

        KeyState getKeyState(unsigned keyId) const override;

        float getAxisState(unsigned axisId) const override;

    private:
        gainput::InputManager& m_inputManager;
        gainput::DeviceId m_id;
    };

    class GAMouseDevice : public IInputDevice
    {
    public:
        GAMouseDevice(gainput::InputManager& inputManager);

        eastl::string getName() const override;

        Type getType() const override;

        unsigned getKeysNum() const override;

        unsigned getAxisNum() const override;

        eastl::string getKeyName(unsigned keyId) const override;

        eastl::string getAxisName(unsigned axisId) const override;

        unsigned getKeyByName(const eastl::string_view keyName) const override;

        unsigned getAxisByName(const eastl::string_view axisName) const override;

        KeyState getKeyState(unsigned keyId) const override;

        float getAxisState(unsigned axisId) const override;

    private:
        gainput::InputManager& m_inputManager;
        gainput::DeviceId m_id;
    };
}  // namespace nau
