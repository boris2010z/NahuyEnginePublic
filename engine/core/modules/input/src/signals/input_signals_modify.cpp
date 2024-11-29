// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_modify.h"

namespace nau
{
    namespace
    {
        constexpr auto DataScale = "scale";
    }  // namespace

    InputSignalScale::InputSignalScale() :
        InputSignalGate("scale", 1)
    {
        addProperty<float>(DataScale, 0.f);
    }

    void InputSignalScale::serializeProperties(DataBlock* blk) const
    {
        InputSignalGate::serializeProperties(blk);
        blk->addReal(DataScale, *m_properties.get<float>(DataScale));
    }

    void InputSignalScale::deserializeProperties(const DataBlock* blk)
    {
        InputSignalGate::deserializeProperties(blk);
        m_properties.set<float>(DataScale, blk->getReal(DataScale));
    }

    void InputSignalScale::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_scale = *m_properties.get<float>(DataScale);
        }
        updateInputs(dt, [this](IInputSignal* input)
        {
            updateState(input->getState());
        });
        m_vector = m_vector * m_scale;
    }

    namespace
    {
        constexpr auto DataDeadZone = "dead_zone";
    }  // namespace

    InputSignalDeadZone::InputSignalDeadZone() :
        InputSignalGate("dead_zone", 1)
    {
        addProperty<float>(DataDeadZone, 0.f);
    }

    void InputSignalDeadZone::serializeProperties(DataBlock* blk) const
    {
        InputSignalGate::serializeProperties(blk);
        blk->addReal(DataDeadZone, *m_properties.get<float>(DataDeadZone));
    }

    void InputSignalDeadZone::deserializeProperties(const DataBlock* blk)
    {
        InputSignalGate::deserializeProperties(blk);
        m_properties.set<float>(DataDeadZone, blk->getReal(DataDeadZone));
    }

    void InputSignalDeadZone::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_deadZone = *m_properties.get<float>(DataDeadZone);
        }
        updateInputs(dt, [this](IInputSignal* input)
        {
            updateState(input->getState());
        });
        for (unsigned i = 0; i < 4; ++i)
        {
            float val = m_vector.getElem(i);
            if (val >= 0.f && val < m_deadZone)
            {
                m_vector.setElem(i, 0.f);
            }
            else if (val <= 0.f && val > -m_deadZone)
            {
                m_vector.setElem(i, 0.f);
            }
        }
    }

    namespace
    {
        constexpr auto DataClamp = "clamp";
    }  // namespace

    InputSignalClamp::InputSignalClamp() :
        InputSignalGate("clamp", 1)
    {
        addProperty<float>(DataClamp, 0.f);
    }

    void InputSignalClamp::serializeProperties(DataBlock* blk) const
    {
        InputSignalGate::serializeProperties(blk);
        blk->addReal(DataClamp, *m_properties.get<float>(DataClamp));
    }

    void InputSignalClamp::deserializeProperties(const DataBlock* blk)
    {
        InputSignalGate::deserializeProperties(blk);
        m_properties.set<float>(DataClamp, blk->getReal(DataClamp));
    }

    void InputSignalClamp::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_clamp = *m_properties.get<float>(DataClamp);
        }
        updateInputs(dt, [this](IInputSignal* input)
        {
            updateState(input->getState());
        });
        for (unsigned i = 0; i < 4; ++i)
        {
            float val = m_vector.getElem(i);
            if (val >= 0.f && val > m_clamp)
            {
                m_vector.setElem(i, m_clamp);
            }
            else if (val <= 0.f && val < -m_clamp)
            {
                m_vector.setElem(i, -m_clamp);
            }
        }
    }

}  // namespace nau
