// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/scene/components/omnilight_component.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(OmnilightComponent)

    math::Vector3 OmnilightComponent::getShift() const
    {
        return m_shift;
    }

    math::Color3 OmnilightComponent::getColor() const
    {
        return math::Color3(m_red, m_green, m_blue);
    }

    float OmnilightComponent::getRadius() const
    {
        return m_radius;
    }

    float OmnilightComponent::getAttenuation() const
    {
        return m_attenuation;
    }

    float OmnilightComponent::getIntensity() const
    {
        return m_intensity;
    }

    void OmnilightComponent::setShift(math::Vector3 shift)
    {
        m_shift = shift;
    }

    void OmnilightComponent::setColor(math::Color3 color)
    {
        //m_color = color;
        m_red = color.r;
        m_green = color.g;
        m_blue = color.b;
    }

    void OmnilightComponent::setRadius(float radius)
    {
        m_radius = radius;
    }

    void OmnilightComponent::setAttenuation(float attenuation)
    {
        m_attenuation = attenuation;
    }

    void OmnilightComponent::setIntensity(float intensity)
    {
        m_intensity = intensity;
    }

    bool OmnilightComponent::getDebugDraw() const
    {
        return m_debugDraw;
    }

    void OmnilightComponent::setDebugDraw(bool debugDraw)
    {
        m_debugDraw = debugDraw;
    }

}  // namespace nau::scene
