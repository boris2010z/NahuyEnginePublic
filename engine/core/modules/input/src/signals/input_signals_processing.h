// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_gate.h"

namespace nau
{
    class InputSignalDelay : public InputSignalGate
    {
    public:
        InputSignalDelay();

        virtual void update(float dt) override;

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;

        virtual void deserializeProperties(const DataBlock* blk) override;

        float m_delay = 0.f;
        float m_passed = 0.f;
    };

    class InputSignalMultiple : public InputSignalGate
    {
    public:
        InputSignalMultiple();

        virtual void update(float dt) override;

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;

        virtual void deserializeProperties(const DataBlock* blk) override;

        float m_delay = 0.f;
        float m_passed = 0.f;
        int m_num = 1;
        int m_numCurrent = 0;
    };
}  // namespace nau
