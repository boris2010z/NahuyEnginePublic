// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_impl.h"

namespace nau
{
    class InputSignalAxis : public InputSignalImpl
    {
    public:
        InputSignalAxis(const eastl::string& type);

    protected:
        virtual void update(float dt) override;

        virtual void serializeProperties(DataBlock* blk) const override;
        virtual void deserializeProperties(const DataBlock* blk) override;

        int m_axesId[4] = {-1, -1, -1, -1};
        math::vec4 m_valuePrev = {0, 0, 0, 0};
    };

    class InputSignalMove : public InputSignalAxis
    {
    public:
        InputSignalMove() :
            InputSignalAxis("move")
        {
        }

    protected:
        virtual void update(float dt) override;
    };

    class InputSignalMoveRelative : public InputSignalAxis
    {
    public:
        InputSignalMoveRelative() :
            InputSignalAxis("move_relative")
        {
        }

    protected:
        virtual void update(float dt) override;

        math::vec4 m_valueCurr = {0, 0, 0, 0};
    };
}  // namespace nau
