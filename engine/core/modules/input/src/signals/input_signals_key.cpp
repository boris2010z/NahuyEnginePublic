// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_key.h"

namespace nau
{
    namespace
    {
        constexpr auto DataKey = "key";
    }  // namespace

    InputSignalKey::InputSignalKey(const eastl::string& type) :
        InputSignalImpl(type)
    {
        addProperty<eastl::string>(DataKey, "");
    }

    void InputSignalKey::update(float dt)
    {
        if (m_key == UINT_MAX || m_properties.isChanged())
        {
            auto key = m_properties.get<eastl::string>(DataKey);
            if (key->empty())
            {
                return;
            }
            m_key = m_controller->getDevice()->getKeyByName(*key);
            if (m_key == UINT_MAX)
            {
                return;
            }
        }
        keyToSignal(m_controller->getDevice()->getKeyState(m_key));
    }

    void InputSignalKey::serializeProperties(DataBlock* blk) const
    {
        blk->addStr(DataKey, m_properties.get<eastl::string>(DataKey)->c_str());
    }

    void InputSignalKey::deserializeProperties(const DataBlock* blk)
    {
        m_key = UINT_MAX;
        m_properties.set<eastl::string>(DataKey, blk->getStr(DataKey));
    }

    namespace
    {
        constexpr auto DataAxis = "axis";
        constexpr auto DataCoeff = "coeff";
    }  // namespace

    InputSignalKeyToAxis::InputSignalKeyToAxis() :
        InputSignalKey("key_axis")
    {
        addProperty<int>(DataAxis, -1);
        addProperty<float>(DataCoeff, 0.f);
    }

    void InputSignalKeyToAxis::keyToSignal(IInputDevice::KeyState state)
    {
        auto axis = *m_properties.get<int>(DataAxis);
        if (axis != -1)
        {
            if (state == IInputDevice::KeyState::Pressed)
            {
                updateState(High);
                m_vector.setElem(axis, *m_properties.get<float>(DataCoeff));
            }
            else
            {
                updateState(Low);
                m_vector.setElem(axis, 0.0f);
            }
        }
    }

    void InputSignalKeyToAxis::serializeProperties(DataBlock* blk) const
    {
        InputSignalKey::serializeProperties(blk);
        blk->addInt(DataAxis, *m_properties.get<int>(DataAxis));
        blk->addReal(DataCoeff, *m_properties.get<float>(DataCoeff));
    }

    void InputSignalKeyToAxis::deserializeProperties(const DataBlock* blk)
    {
        InputSignalKey::deserializeProperties(blk);
        m_properties.set<int>(DataAxis, blk->getInt(DataAxis));
        m_properties.set<float>(DataCoeff, blk->getReal(DataCoeff));
    }
}  // namespace nau