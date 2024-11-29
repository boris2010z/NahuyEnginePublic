// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "input_action_impl.h"

#include "nau/service/service_provider.h"

namespace nau
{
    namespace
    {
        constexpr auto DataType = "type";
        constexpr auto DataName = "name";
        constexpr auto DataSignal = "signal";
        constexpr auto DataTags = "tags";
    }  // namespace

    void InputActionImpl::update(float dt)
    {
        m_signal->update(dt);
        if (m_signal->getState() == IInputSignal::High)
        {
            if (m_type == Continuous || m_prevState == IInputSignal::Low)
            {
                m_actionCallback(m_signal.get());
            }
        }
        m_prevState = m_signal->getState();
    }

    void InputActionImpl::serialize(DataBlock* blk) const
    {
        blk->addStr(DataName, m_name.c_str());
        blk->addInt(DataType, m_type == Trigger ? 0 : 1);
        m_signal->serialize(blk->addNewBlock(DataSignal));
        if (!m_tags.empty())
        {
            auto* tags = blk->addNewBlock(DataTags);
            size_t i = 0;
            for (auto& tag : m_tags)
            {
                tags->addStr(eastl::to_string(i).c_str(), tag.c_str());
            }
        }
    }

    bool InputActionImpl::deserialize(const DataBlock* blk)
    {
        auto actionName = blk->getStr(DataName);
        if (actionName == nullptr)
        {
            return false;
        }
        auto actionType = blk->getInt(DataType, -1);
        if (actionType == -1)
        {
            return false;
        }
        m_name = actionName;
        m_type = actionType == 0 ? Trigger : Continuous;

        m_tags.clear();
        auto* tags = blk->getBlockByNameEx(DataTags, nullptr);
        if (tags != nullptr)
        {
            for (size_t i = 0; true; ++i)
            {
                auto* tag = tags->getStr(eastl::to_string(i).c_str(), nullptr);
                if (tag == nullptr)
                {
                    break;
                }
                m_tags.emplace(tag);
            }
        }

        auto signalBlock = blk->getBlockByNameEx(DataSignal);
        if (signalBlock == nullptr)
        {
            return false;
        }
        auto signalType = signalBlock->getStr(DataType);
        if (signalType == nullptr)
        {
            return false;
        }
        auto& insys = getServiceProvider().get<IInputSystem>();
        auto* signal = insys.createSignal(signalType);
        if (signal == nullptr)
        {
            return false;
        }
        m_signal = eastl::shared_ptr<IInputSignal>(signal);
        return m_signal->deserialize(signalBlock);
    }

    void InputActionImpl::addContextTag(const eastl::string& tag)
    {
        m_tags.emplace(tag);
    }

    void InputActionImpl::removeContextTag(const eastl::string& tag)
    {
        m_tags.erase(tag);
    }

    bool InputActionImpl::isContextTag(const eastl::string& tag) const
    {
        if (tag.empty() && m_tags.empty())
        {
            return true;
        }
        return m_tags.count(tag) > 0;
    }
}  // namespace nau
