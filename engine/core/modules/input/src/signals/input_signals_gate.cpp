// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_signals_gate.h"

#include "nau/service/service_provider.h"

namespace nau
{
    void InputSignalGate::addInput(IInputSignal* source)
    {
        if (m_inputs.size() >= m_maxInputs)
        {
            NAU_FAILURE();
            return;
        }
        m_inputs.push_back(eastl::shared_ptr<IInputSignal>(source));
    }

    IInputSignal* InputSignalGate::getInput(unsigned idx)
    {
        if (idx < m_inputs.size())
        {
            return m_inputs[idx].get();
        }
        return nullptr;
    }

    void InputSignalGate::updateInputs(float dt, nau::Functor<void(IInputSignal*)> callback)
    {
        m_vector = math::vec4::zero();
        for (auto& input : m_inputs)
        {
            input->update(dt);
            m_vector += input->getVector4();
            callback(input.get());
        }
    }

    unsigned InputSignalGate::maxInputs() const
    {
        return m_maxInputs;
    }

    namespace
    {
        constexpr auto DataType = "type";
        constexpr auto DataSignal = "signal";
        constexpr auto DataSignals = "signals";
    }  // namespace

    void InputSignalGate::serializeProperties(DataBlock* blk) const
    {
        eastl::string name(DataSignal);
        for (size_t i = 0; i < m_inputs.size(); ++i)
        {
            m_inputs[i]->serialize(blk->addBlock((name + eastl::to_string(i)).c_str()));
        }
        blk->addInt(DataSignals, (int)m_inputs.size());
    }

    void InputSignalGate::deserializeProperties(const DataBlock* blk)
    {
        int count = blk->getInt(DataSignals, 0);
        if (count > 0)
        {
            auto& insys = getServiceProvider().get<nau::IInputSystem>();
            eastl::string name(DataSignal);
            for (int i = 0; i < count; ++i)
            {
                auto* signalBlk = blk->getBlockByNameEx((name + eastl::to_string(i)).c_str());
                if (signalBlk != nullptr)
                {
                    auto* type = signalBlk->getStr(DataType);
                    IInputSignal* signal = insys.createSignal(type);
                    signal->deserialize(signalBlk);
                    m_inputs.push_back(eastl::shared_ptr<IInputSignal>(signal));
                }
            }
        }
    }
}  // namespace nau
