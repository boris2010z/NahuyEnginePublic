// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_impl.h"

namespace nau
{
    class InputSignalKey : public InputSignalImpl
    {
    protected:
        InputSignalKey(const eastl::string& type);

        virtual void keyToSignal(IInputDevice::KeyState state) = 0;

        virtual void update(float dt) override;

    protected:
        virtual void serializeProperties(DataBlock* blk) const override;

        virtual void deserializeProperties(const DataBlock* blk) override;

        // Cached key id
        unsigned m_key = UINT_MAX;
    };

    class InputSignalPressed : public InputSignalKey
    {
    public:
        InputSignalPressed() :
            InputSignalKey("pressed")
        {
        }

        void keyToSignal(IInputDevice::KeyState state)
        {
            if (state == IInputDevice::KeyState::Pressed)
            {
                updateState(State::High);
            }
            else
            {
                updateState(State::Low);
            }
        }
    };

    class InputSignalReleased : public InputSignalKey
    {
    public:
        InputSignalReleased() :
            InputSignalKey("released")
        {
        }

        void keyToSignal(IInputDevice::KeyState state)
        {
            if (state == IInputDevice::KeyState::Released)
            {
                updateState(State::High);
            }
            else
            {
                updateState(State::Low);
            }
        }
    };

    class InputSignalKeyToAxis : public InputSignalKey
    {
    public:
        InputSignalKeyToAxis();

    private:
        virtual void keyToSignal(IInputDevice::KeyState state) override;

        virtual void serializeProperties(DataBlock* blk) const override;

        virtual void deserializeProperties(const DataBlock* blk) override;
    };

}  // namespace nau
