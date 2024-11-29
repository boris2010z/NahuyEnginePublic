// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_logic.h"

namespace nau
{
    void InputSignalOr::update(float dt)
    {
        State state = Low;
        updateInputs(dt, [this, &state](IInputSignal* input)
        {
            if (input->getState() == High)
            {
                state = High;
            }
        });
        updateState(state);
    }

    void InputSignalAnd::update(float dt)
    {
        State state = High;
        updateInputs(dt, [this, &state](IInputSignal* input)
        {
            if (input->getState() == Low)
            {
                state = Low;
            }
        });
        updateState(state);
    }

    void InputSignalNot::update(float dt)
    {
        State state = Low;
        updateInputs(dt, [this, &state](IInputSignal* input)
        {
            if (input->getState() == Low)
            {
                state = High;
            }
        });
        updateState(state);
    }
}  // namespace nau
