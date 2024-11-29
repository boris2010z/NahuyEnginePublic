// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/animation/playback/animation_instance.h"

#include "./animation_helper.h"
#include "nau/animation/assets/animation_asset.h"
#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/data/events.h"
#include "nau/animation/playback/animation.h"
#include "nau/messaging/messaging.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau::animation
{
    AnimationInstance::AnimationInstance(const eastl::string& name, nau::Ptr<Animation> animation, const AnimationInstanceCreationData* creationData)
        : m_animation(animation)
        , m_name(name)
    {
        if (creationData)
        {
            m_animationState.isStopped = creationData->isStopped;
            m_animationAsset = creationData->sourceRef;
        }
    }

    AnimationInstance::AnimationInstance(const eastl::string& name, AnimationAssetRef&& assetRef)
        : m_name(name)
    {
        m_animationAsset = std::move(assetRef);
    }

    AnimationInstance::AnimationInstance(const eastl::string& name, const AnimationAssetRef& assetRef)
        : m_name(name)
        , m_animationAsset(assetRef)
    {
    }

    AnimationInstance::AnimationInstance(const eastl::string& name, const AnimationInstance& source)
        : AnimationInstance(source)
    {
        m_name = name;
    }

    async::Task<> AnimationInstance::load()
    {
        if (!m_isLoaded)
        {
            if (m_animation)
            {
                m_isLoaded = true;
            }
            else if (m_animationAsset)
            {
                using namespace data;
                nau::Ptr<AnimationAssetView> loadedAnimation = co_await m_animationAsset.getAssetViewTyped<AnimationAssetView>();
                m_animation = loadedAnimation->getAnimation();
                m_animationState.interpolationMethod = loadedAnimation->getPlaybackData().interpolationMethod;

                m_isLoaded = m_animation != nullptr;
            }

            if (m_isLoaded)
            {
                m_animationState.player = m_animation->createPlayer(*this);
                m_animationState.forcedFrame = 0;
            }
        }
    }

    void AnimationInstance::update(AnimationController& controller, float dt, const IAnimatable::Ptr& target)
    {
        if (m_isLoaded)
        {
            advance(controller, dt);

            if (m_frame == -1)
            {
                return;
            }

            m_animationState.target = target;

            m_animationState.animInstanceName = m_name;

            const bool isPlaying = m_animationState.weight > AnimationController::NEGLIGIBLE_WEIGHT;

            if (!m_animationState.ignoreController)
            {
                m_animationState.weight = controller.getWeight(*this);

                if (isPlaying && m_animationState.blendInTime > 0.0f && m_animationState.blendOutTime > 0.0f)
                {
                    updateBlendInOut(controller);
                }
            }

            if (isPlaying)
            {
                m_animation->apply(m_frame, m_animationState);
            }
        }
    }

    float AnimationInstance::getCurrentTime() const
    {
        return m_animationState.time;
    }

    float AnimationInstance::getDurationSeconds(AnimationController& controller) const
    {
        return m_animation->getDurationInFrames() / controller.getFrameRate();
    }

    bool AnimationInstance::isPlaying() const
    {
        if (!m_isLoaded)
        {
            return false;
        }

        if (!m_animationState.isReversed)
        {
            return m_frame != m_animation->getLastFrame();
        }

        return m_frame != 0;
    }

    int AnimationInstance::getCurrentFrame() const
    {
        return m_frame;
    }

    IAnimationPlayer* AnimationInstance::getPlayer()
    {
        return m_animationState.player.get();
    }

    PlayMode AnimationInstance::getPlayMode() const
    {
        return m_playMode;
    }

    void AnimationInstance::setPlayMode(PlayMode mode)
    {
        m_playMode = mode;
    }

    bool AnimationInstance::isReversed() const
    {
        return m_animationState.isReversed;
    }

    void AnimationInstance::setIsReversed(bool reverse)
    {
        m_animationState.isReversed = reverse;
    }

    float AnimationInstance::getWeight() const
    {
        return m_animationState.weight;
    }

    void AnimationInstance::setWeight(float weight)
    {
        m_animationState.weight = weight;
    }

    AnimationBlendMethod AnimationInstance::getBlendMethod() const
    {
        return m_animationState.blendMethod;
    }

    void AnimationInstance::setBlendMethod(AnimationBlendMethod blendMethod)
    {
        m_animationState.blendMethod = blendMethod;
    }

    void AnimationInstance::debugIgnoreController(bool ignore)
    {
        m_animationState.ignoreController = ignore;
    }

    bool AnimationInstance::getIgnoresController() const
    {
        return m_animationState.ignoreController;
    }

    AnimationInterpolationMethod AnimationInstance::getInterpolationMethod() const
    {
        return m_animationState.interpolationMethod;
    }

    void AnimationInstance::setInterpolationMethod(AnimationInterpolationMethod value)
    {
        m_animationState.interpolationMethod = value;
    }

    void AnimationInstance::restart(AnimationController& controller)
    {
        if (!m_animationState.isReversed)
        {
            m_animationState.time = .0f;
        }
        else
        {
            m_animationState.time = getDurationSeconds(controller);
        }
    }

    const eastl::string& AnimationInstance::getName() const
    {
        return m_name;
    }

    AnimationAssetRef AnimationInstance::getAssetRef() const
    {
        return m_animationAsset;
    }

    const Animation* AnimationInstance::getAnimation() const
    {
        return m_animation.get();
    }

    void AnimationInstance::advance(AnimationController& controller, float dt)
    {
        if (m_animationState.forcedFrame == -1 && m_animationState.isPaused || m_animationState.isStopped)
        {
            return;
        }

        const float duration = getDurationSeconds(controller);
        const float dtToUse = dt * getPlayer()->getPlaybackSpeed();

        if (!m_animationState.isReversed)
        {
            m_animationState.time += dtToUse;

            if (m_animationState.time > duration)
            {
                switch (getPlayMode())
                {
                case PlayMode::Looping:
                    m_animationState.time -= floorf(m_animationState.time / duration) * duration;
                    break;
                case PlayMode::Once:
                    m_animationState.time = duration;
                    break;
                case PlayMode::PingPong:
                    m_animationState.isReversed = true;
                    break;
                }
            }
        }
        else
        {
            m_animationState.time -= dtToUse;

            if (m_animationState.time < .0f)
            {
                switch (getPlayMode())
                {
                case PlayMode::Looping:
                    m_animationState.time = duration;
                    break;
                case PlayMode::Once:
                    m_animationState.time = .0f;
                    break;
                case PlayMode::PingPong:
                    m_animationState.isReversed = false;
                    break;
                }
            }
        }

        if (m_animationState.forcedFrame != -1)
        {
            m_animationState.time = m_animationState.forcedFrame / controller.getFrameRate() + MATH_SMALL_NUMBER;
            m_animationState.forcedFrame = -1;
        }

        m_animationState.time = math::clamp(m_animationState.time, .0f, duration);
        int newFrame = (int)(m_animationState.time * controller.getFrameRate());

        if (newFrame != m_frame)
        {
            m_frame = newFrame;
            updateEvents();
        }

        fireEvents();
    }

    void AnimationInstance::updateBlendInOut(AnimationController& controller)
    {
        const float duration = getDurationSeconds(controller);
        m_animationState.blendInOutWeight = 1.f;

        if (!m_animationState.isReversed)
        {
            if (m_animationState.time < m_animationState.blendInTime)
            {
                m_animationState.blendInOutWeight = m_animationState.time / m_animationState.blendInTime;
            }
            else if (m_animationState.time > duration - m_animationState.blendOutTime)
            {
                m_animationState.blendInOutWeight = (duration - m_animationState.time) / m_animationState.blendOutTime;
            }
        }
        else
        {
            if (m_animationState.time > duration - m_animationState.blendOutTime)
            {
                m_animationState.blendInOutWeight = (duration - m_animationState.time) / m_animationState.blendOutTime;
            }
            else if (m_animationState.time < m_animationState.blendInTime)
            {
                m_animationState.blendInOutWeight = m_animationState.time / m_animationState.blendInTime;
            }
        }
    }

    void AnimationInstance::updateEvents()
    {
        for (auto& event : m_animationState.events)
        {
            if (event.flags.has(FrameEventControl::IsActive) && !event.flags.has(FrameEventControl::IsPinned))
            {
                event.flags.clear();
            }
        }

        if (const Animation* animation = getAnimation())
        {
            if (m_frame == 0)
            {
                m_animationState.clearEvents();
                m_animationState.addEvent(events::ANIMATION_EVENT_TRACK_STARTED, false);
            }
            if (m_frame == animation->getLastFrame())
            {
                m_animationState.clearEvents();
                m_animationState.addEvent(events::ANIMATION_EVENT_TRACK_FINISHED, false);
            }

            for (const auto& newEvent : animation->getEvents(m_frame))
            {
                if (newEvent.getActivationDirection() == FrameEventActivationDirection::Forward && m_animationState.isReversed)
                {
                    continue;
                }
                if (newEvent.getActivationDirection() == FrameEventActivationDirection::Backward && !m_animationState.isReversed)
                {
                    continue;
                }

                if (newEvent.getEventType() == FrameEventType::Stop)
                {
                    m_animationState.removeEvent(newEvent.getId());
                }
                else
                {
                    m_animationState.addEvent(newEvent.getId(), newEvent.getEventType() == FrameEventType::Start);
                }
            }
        }
    }

    void AnimationInstance::fireEvents()
    {
        if (m_animationState.target)
        {
            for (auto& event : m_animationState.events)
            {
                if (event.flags.has(FrameEventControl::IsActive))
                {
                    AnimationHelper::broadcastFrameEvent(m_animationState.target->getOwner(), this, event.id);
                }
            }
        }
    }

}  // namespace nau::animation
