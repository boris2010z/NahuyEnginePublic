// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "nau/input_system.h"

namespace nau
{
    class InputSignalImpl : public IInputSignal
    {
    public:
        InputSignalImpl(const eastl::string& type);

        virtual const eastl::string& getName() const override;

        virtual void setName(const eastl::string& name);

        virtual const eastl::string& getType() const override;

        virtual IInputController* getController() override;

        virtual void setController(IInputController*) override;

        virtual State getState() const override;

        virtual State getPreviousState() const override;

        virtual float getValue() const override;

        virtual math::vec2 getVector2() const override;

        virtual math::vec3 getVector3() const override;

        virtual math::vec4 getVector4() const override;

        virtual void serialize(DataBlock* blk) const override;

        virtual bool deserialize(const DataBlock* blk) override;

        virtual void addInput(IInputSignal* source) override;

        virtual IInputSignal* getInput(unsigned idx) override;

        virtual unsigned maxInputs() const override;

        virtual InputSignalProperties& Properties() override;
        virtual const InputSignalProperties& Properties() const override;

        virtual void serializeProperties(DataBlock* blk) const = 0;
        virtual void deserializeProperties(const DataBlock* blk) = 0;

        void generateName();

    protected:
        template <typename T>
        void addProperty(const eastl::string& key, const T& value)
        {
            m_properties.add<T>(key, value);
        }

        void updateState(State state);

        eastl::string m_name;
        eastl::string m_type;
        math::vec4 m_vector = {0, 0, 0, 0};
        IInputController* m_controller = nullptr;
        InputSignalProperties m_properties;

        static unsigned g_signalIdx;

    private:
        State m_currState = State::Low;
        State m_prevState = State::Low;
    };
}  // namespace nau
