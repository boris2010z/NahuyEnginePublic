// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_gate.h"

namespace nau
{
    class InputSignalOr : public InputSignalGate
    {
    public:
        InputSignalOr() :
            InputSignalGate("or", 4)
        {
        }

        virtual void update(float dt) override;
    };

    class InputSignalAnd : public InputSignalGate
    {
    public:
        InputSignalAnd() :
            InputSignalGate("and", 4)
        {
        }

        virtual void update(float dt) override;
    };

    class InputSignalNot : public InputSignalGate
    {
    public:
        InputSignalNot() :
            InputSignalGate("not", 1)
        {
        }

        virtual void update(float dt) override;
    };

}  // namespace nau
