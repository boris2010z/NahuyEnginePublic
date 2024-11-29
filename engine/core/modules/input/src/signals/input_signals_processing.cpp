// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_processing.h"

namespace nau
{
    namespace
    {
        constexpr auto DataDelay = "delay";
        constexpr auto DataNum = "num";
    }  // namespace

    InputSignalDelay::InputSignalDelay() :
        InputSignalGate("delay", 1)
    {
        addProperty<float>(DataDelay, 0.f);
    }

    void InputSignalDelay::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_delay = *m_properties.get<float>(DataDelay);
        }
        updateInputs(dt, [this, dt](IInputSignal* input)
        {
            if (input->getState() == High)
            {
                m_passed += dt;
                if (m_passed > m_delay)
                {
                    updateState(High);
                }
            }
            else
            {
                m_passed = 0.f;
            }
            m_vector = input->getVector4();
        });
    }

    void InputSignalDelay::serializeProperties(DataBlock* blk) const
    {
        InputSignalGate::serializeProperties(blk);
        blk->addReal(DataDelay, *m_properties.get<float>(DataDelay));
    }

    void InputSignalDelay::deserializeProperties(const DataBlock* blk)
    {
        InputSignalGate::deserializeProperties(blk);
        m_properties.set<float>(DataDelay, blk->getReal(DataDelay, 0.f));
    }

    InputSignalMultiple::InputSignalMultiple() :
        InputSignalGate("multiple", 1)
    {
        addProperty<float>(DataDelay, 0.f);
        addProperty<int>(DataNum, 1);
    }

    void InputSignalMultiple::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_delay = *m_properties.get<float>(DataDelay);
            m_num = *m_properties.get<int>(DataNum);
        }

        updateInputs(dt, [this, dt](IInputSignal* input)
        {
            if (input->getState() == High)
            {
                if (input->getPreviousState() == Low)
                {
                    // Increase counter on signal change
                    ++m_numCurrent;
                    m_passed = 0.f;
                }
            }
            else
            {
                m_passed += dt;
                if (m_passed > m_delay)
                {
                    // Reset counter if max time passed
                    m_numCurrent = 0;
                }
            }
            if (m_numCurrent >= m_num)
            {
                updateState(High);
            }
            else
            {
                updateState(Low);
            }
            m_vector = input->getVector4();
        });
    }

    void InputSignalMultiple::serializeProperties(DataBlock* blk) const
    {
        InputSignalGate::serializeProperties(blk);
        blk->addReal(DataDelay, *m_properties.get<float>(DataDelay));
        blk->addInt(DataNum, *m_properties.get<int>(DataNum));
    }

    void InputSignalMultiple::deserializeProperties(const DataBlock* blk)
    {
        InputSignalGate::deserializeProperties(blk);
        m_properties.set<float>(DataDelay, blk->getReal(DataDelay, 0.f));
        m_properties.set<int>(DataNum, blk->getInt(DataNum, 1));
    }

}  // namespace nau
