// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/scene/components/spotlight_component.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(SpotlightComponent)

    math::Vector3 SpotlightComponent::getShift() const
    {
        return m_shift;
    }

    math::Color3 SpotlightComponent::getColor() const
    {
        return math::Color3(m_red, m_green, m_blue);
    }

    float SpotlightComponent::getRadius() const
    {
        return m_radius;
    }

    float SpotlightComponent::getAttenuation() const
    {
        return m_attenuation;
    }

    void SpotlightComponent::setShift(math::Vector3 shift)
    {
        m_shift = shift;
    }

    void SpotlightComponent::setColor(math::Color3 color)
    {
        //m_color = color;
        m_red = color.r;
        m_green = color.g;
        m_blue = color.b;
    }

    void SpotlightComponent::setRadius(float radius)
    {
        m_radius = radius;
    }

    void SpotlightComponent::setAttenuation(float attenuation)
    {
        m_attenuation = attenuation;
    }

    math::Vector3 SpotlightComponent::getDirection() const
    {
        return m_direction;
    }

    float SpotlightComponent::getAngle() const
    {
        return m_angle;
    }

    float SpotlightComponent::getIntensity() const
    {
        return m_intensity;
    }

    void SpotlightComponent::setDirection(math::Vector3 direction)
    {
        m_direction = direction;
    }

    void SpotlightComponent::setIntensity(float intensity)
    {
        m_intensity = intensity;
    }

    void SpotlightComponent::setAngle(float angle)
    {
        m_angle = angle;
    }

    bool SpotlightComponent::getDebugDraw() const
    {
        return m_debugDraw;
    }

    void SpotlightComponent::setDebugDraw(bool debugDraw)
    {
        m_debugDraw = debugDraw;
    }

}  // namespace nau::scene
