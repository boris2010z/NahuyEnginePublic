// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/input.h"
#include "input_manager.h"
#include "nau/rtti/type_info.h"
#include "nau/service/service_provider.h"


void nau::input::setScreenResolution(int x, int y)
{
  nau::getServiceProvider().get<nau::input::InputManager>().setScreenResolution(x, y);
}

void nau::input::update()
{
    if (nau::getServiceProvider().has<nau::input::InputManager>())
    {
        nau::getServiceProvider().get<nau::input::InputManager>().update();
    }
}

void nau::input::update(float dt)
{
    if (nau::getServiceProvider().has<nau::input::InputManager>())
    {
        nau::getServiceProvider().get<nau::input::InputManager>().update(dt);
    }
}

int nau::input::getKeyboardDeviceCount()
{
  return 1;
}

bool nau::input::isKeyboardButtonPressed(int deviceId, nau::input::Key key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().isKeyboardButtonPressed(deviceId, key);
}

bool nau::input::isKeyboardButtonHold(int deviceId, nau::input::Key key)
{
   return nau::getServiceProvider().get<nau::input::InputManager>().isKeyboardButtonHold(deviceId, key);
}

int nau::input::getMouseDeviceCount()
{
  return 1;
}

bool nau::input::isMouseButtonPressed(int deviceId, nau::input::MouseKey key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().isMouseButtonPressed(deviceId, key);
}

bool nau::input::isMouseButtonReleased(int deviceId, nau::input::MouseKey key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().isMouseButtonReleased(deviceId, key);
}

bool nau::input::isMouseButtonHold(int deviceId, nau::input::MouseKey key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().isMouseButtonHold(deviceId, key);
}

float nau::input::getMouseAxisValue(int deviceId, nau::input::MouseKey key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().getMouseAxisValue(deviceId, key);
}

float nau::input::getMouseAxisDelta(int deviceId, nau::input::MouseKey key)
{
  return nau::getServiceProvider().get<nau::input::InputManager>().getMouseAxisDelta(deviceId, key);
}

NAU_COREINPUT_EXPORT void* nau::input::getGaInputManager()
{
    auto& gaInput = nau::getServiceProvider().get<GainputAccess>().getGainput();
    return &gaInput;
}
