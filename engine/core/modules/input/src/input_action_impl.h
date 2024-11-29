// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <EASTL/set.h>

#include "nau/input_system.h"

namespace nau
{
    class InputActionImpl : public IInputAction
    {
    public:
        InputActionImpl(nau::Functor<void(IInputSignal*)> actionCallback) :
            m_actionCallback(eastl::move(actionCallback))
        {
        }

        virtual eastl::string getName() const override
        {
            return m_name;
        }
        virtual Type getType() const override
        {
            return m_type;
        }
        virtual IInputSignal* getSignal() override
        {
            return m_signal.get();
        }

        virtual void update(float dt) override;

        virtual void serialize(DataBlock* blk) const override;
        virtual bool deserialize(const DataBlock* blk) override;

        virtual void addContextTag(const eastl::string& tag) override;
        virtual void removeContextTag(const eastl::string& tag) override;
        virtual bool isContextTag(const eastl::string& tag) const override;

        void setName(const eastl::string& name)
        {
            m_name = name;
        }
        void setType(Type type)
        {
            m_type = type;
        }
        void setSignal(IInputSignal* signal)
        {
            m_signal = eastl::shared_ptr<IInputSignal>(signal);
        }

    private:
        Type m_type = Type::Trigger;
        eastl::string m_name;
        IInputSignal::State m_prevState = IInputSignal::State::Low;
        eastl::shared_ptr<IInputSignal> m_signal;
        nau::Functor<void(IInputSignal*)> m_actionCallback;
        eastl::set<eastl::string> m_tags;
    };

}  // namespace nau
