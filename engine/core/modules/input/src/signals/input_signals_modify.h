// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_gate.h"

namespace nau
{
    class InputSignalScale : public InputSignalGate
    {
    public:
        InputSignalScale();

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;
        virtual void deserializeProperties(const DataBlock* blk) override;

        virtual void update(float dt) override;

    private:
        float m_scale = 0.f;
    };

    class InputSignalDeadZone : public InputSignalGate
    {
    public:
        InputSignalDeadZone();

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;
        virtual void deserializeProperties(const DataBlock* blk) override;

        virtual void update(float dt) override;

    private:
        float m_deadZone = 0.f;
    };

    class InputSignalClamp : public InputSignalGate
    {
    public:
        InputSignalClamp();

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;
        virtual void deserializeProperties(const DataBlock* blk) override;

        virtual void update(float dt) override;

    private:
        float m_clamp = 0.f;
    };

}  // namespace nau
