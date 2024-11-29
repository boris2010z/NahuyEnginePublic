// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "input_signals_impl.h"
#include "nau/utils/functor.h"

namespace nau
{
    class InputSignalGate : public InputSignalImpl
    {
    protected:
        InputSignalGate(const eastl::string& name, unsigned maxInputs) :
            InputSignalImpl(name),
            m_maxInputs(maxInputs)
        {
        }

        virtual void addInput(IInputSignal* source) override;

        virtual IInputSignal* getInput(unsigned idx) override;

        virtual unsigned maxInputs() const override;

        void updateInputs(float dt, nau::Functor<void(IInputSignal*)> callback);

        virtual void serializeProperties(DataBlock* blk) const override;

        virtual void deserializeProperties(const DataBlock* blk) override;

    private:
        eastl::vector<eastl::shared_ptr<IInputSignal>> m_inputs;
        unsigned m_maxInputs = 0;
    };
}  // namespace nau
