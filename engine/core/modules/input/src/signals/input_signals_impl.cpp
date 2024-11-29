// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_impl.h"

#include "nau/service/service_provider.h"

namespace nau
{
    unsigned InputSignalImpl::g_signalIdx = 0;

    InputSignalImpl::InputSignalImpl(const eastl::string& type) :
        m_type(type)
    {
    }

    const eastl::string& InputSignalImpl::getName() const
    {
        return m_name;
    }

    void InputSignalImpl::setName(const eastl::string& name)
    {
        m_name = name;
    }

    const eastl::string& InputSignalImpl::getType() const
    {
        return m_type;
    }

    IInputController* InputSignalImpl::getController()
    {
        return m_controller;
    }

    void InputSignalImpl::setController(IInputController* controller)
    {
        m_controller = controller;
    }

    IInputSignal::State InputSignalImpl::getState() const
    {
        return m_currState;
    }

    IInputSignal::State InputSignalImpl::getPreviousState() const
    {
        return m_prevState;
    }

    void InputSignalImpl::updateState(State state)
    {
        m_prevState = m_currState;
        m_currState = state;
    }

    void InputSignalImpl::generateName()
    {
        eastl::string name = getType() + "_" + eastl::to_string(g_signalIdx++);
        for (size_t i = 0; i < name.size(); ++i)
        {
            char chr = name[i];
            if (std::isalnum(chr) == 0 && chr != '_')
            {
                name[i] = '_';
            }
        }
        setName(name);
    }

    float InputSignalImpl::getValue() const
    {
        return m_vector.getX();
    }

    math::vec2 InputSignalImpl::getVector2() const
    {
        return math::vec2(m_vector.getX(), m_vector.getY());
    }

    math::vec3 InputSignalImpl::getVector3() const
    {
        return m_vector.getXYZ();
    }

    math::vec4 InputSignalImpl::getVector4() const
    {
        return m_vector;
    }

    namespace
    {
        constexpr auto DataType = "type";
        constexpr auto DataName = "name";
        constexpr auto DataController = "controller";
        constexpr auto DataProperties = "properties";
    }  // namespace

    void InputSignalImpl::serialize(DataBlock* blk) const
    {
        blk->addStr(DataName, m_name.c_str());
        blk->addStr(DataType, m_type.c_str());
        blk->addStr(DataController, m_controller->getName().c_str());
        serializeProperties(blk->addBlock(DataProperties));
    }

    bool InputSignalImpl::deserialize(const DataBlock* blk)
    {
        m_name = blk->getStr(DataName);
        IInputSystem& insys = getServiceProvider().get<IInputSystem>();
        setController(insys.getController(blk->getStr(DataController)));
        const DataBlock* params = blk->getBlockByName(DataProperties);
        if (params != nullptr)
        {
            deserializeProperties(params);
        }
        return true;
    }

    void InputSignalImpl::addInput(IInputSignal* source)
    {
    }

    IInputSignal* InputSignalImpl::getInput(unsigned)
    {
        return nullptr;
    }

    unsigned InputSignalImpl::maxInputs() const
    {
        return 0;
    }

    InputSignalProperties& InputSignalImpl::Properties()
    {
        return m_properties;
    }

    const InputSignalProperties& InputSignalImpl::Properties() const
    {
        return m_properties;
    }
}  // namespace nau
