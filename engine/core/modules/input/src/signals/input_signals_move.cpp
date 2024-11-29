// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_move.h"

namespace nau
{
    namespace
    {
        constexpr auto DataAxisX = "axis_x";
        constexpr auto DataAxisY = "axis_y";
        constexpr auto DataAxisZ = "axis_z";
        constexpr auto DataAxisW = "axis_w";
    }  // namespace

    InputSignalAxis::InputSignalAxis(const eastl::string& type) :
        InputSignalImpl(type)
    {
        addProperty<int>(DataAxisX, -1);
        addProperty<int>(DataAxisY, -1);
        addProperty<int>(DataAxisZ, -1);
        addProperty<int>(DataAxisW, -1);
    }

    void InputSignalAxis::serializeProperties(DataBlock* blk) const
    {
        blk->addInt(DataAxisX, *m_properties.get<int>(DataAxisX));
        blk->addInt(DataAxisY, *m_properties.get<int>(DataAxisY));
        blk->addInt(DataAxisZ, *m_properties.get<int>(DataAxisZ));
        blk->addInt(DataAxisW, *m_properties.get<int>(DataAxisW));
    }

    void InputSignalAxis::deserializeProperties(const DataBlock* blk)
    {
        m_properties.set<int>(DataAxisX, blk->getInt(DataAxisX));
        m_properties.set<int>(DataAxisY, blk->getInt(DataAxisY));
        m_properties.set<int>(DataAxisZ, blk->getInt(DataAxisZ));
        m_properties.set<int>(DataAxisW, blk->getInt(DataAxisW));
    }

    void InputSignalAxis::update(float dt)
    {
        if (m_properties.isChanged())
        {
            m_axesId[0] = *m_properties.get<int>(DataAxisX);
            m_axesId[1] = *m_properties.get<int>(DataAxisY);
            m_axesId[2] = *m_properties.get<int>(DataAxisZ);
            m_axesId[3] = *m_properties.get<int>(DataAxisW);
        }
    }

    void InputSignalMove::update(float dt)
    {
        InputSignalAxis::update(dt);
        for (auto axis : m_axesId)
        {
            if (axis != -1)
            {
                m_vector.setElem(axis, m_controller->getDevice()->getAxisState(axis));
            }
        }
        if (m_vector.similar(m_valuePrev))
        {
            updateState(Low);
        }
        else
        {
            updateState(High);
            m_valuePrev = m_vector;
        }
    }

    void InputSignalMoveRelative::update(float dt)
    {
        InputSignalAxis::update(dt);
        for (auto axis : m_axesId)
        {
            if (axis != -1)
            {
                m_valueCurr.setElem(axis, m_controller->getDevice()->getAxisState(axis));
            }
        }
        if (m_valueCurr.similar(m_valuePrev))
        {
            updateState(Low);
        }
        else
        {
            updateState(High);
            m_vector = m_valuePrev - m_valueCurr;
            m_valuePrev = m_valueCurr;
        }
    }
}  // namespace nau