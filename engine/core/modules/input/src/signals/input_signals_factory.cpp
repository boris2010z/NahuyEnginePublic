// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_gate.h"
#include "input_signals_impl.h"
#include "input_signals_key.h"
#include "input_signals_logic.h"
#include "input_signals_modify.h"
#include "input_signals_move.h"
#include "input_signals_processing.h"
#include "input_system_impl.h"

namespace nau
{
    InputSignalImpl* InputSystemImpl::InputSignalFactory::create(const eastl::string& type)
    {
        if (type == "pressed")
        {
            return new InputSignalPressed();
        }
        else if (type == "released")
        {
            return new InputSignalReleased();
        }
        else if (type == "move")
        {
            return new InputSignalMove();
        }
        else if (type == "move_relative")
        {
            return new InputSignalMoveRelative();
        }
        else if (type == "or")
        {
            return new InputSignalOr();
        }
        else if (type == "and")
        {
            return new InputSignalAnd();
        }
        else if (type == "not")
        {
            return new InputSignalNot();
        }
        else if (type == "key_axis")
        {
            return new InputSignalKeyToAxis();
        }
        else if (type == "delay")
        {
            return new InputSignalDelay();
        }
        else if (type == "multiple")
        {
            return new InputSignalMultiple();
        }
        else if (type == "scale")
        {
            return new InputSignalScale();
        }
        else if (type == "dead_zone")
        {
            return new InputSignalDeadZone();
        }
        else if (type == "clamp")
        {
            return new InputSignalClamp();
        }
        return nullptr;
    }

}  // namespace nau
