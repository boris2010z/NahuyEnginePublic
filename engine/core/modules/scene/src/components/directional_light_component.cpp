// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/scene/components/directional_light_component.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(DirectionalLightComponent)

    void DirectionalLightComponent::setColor(const math::Color3& color)
    {
        //m_color = color;
        m_red = color.r;
        m_green = color.g;
        m_blue = color.b;
    }

    void DirectionalLightComponent::setIntensity(float intensity)
    {
        m_intensity = intensity;
    }

    void DirectionalLightComponent::setCastShadows(bool hasShadows)
    {
        m_castShadows = hasShadows;
    }

    void DirectionalLightComponent::setShadowMapSize(uint32_t size)
    {
        m_csmSize = size;
    }

    void DirectionalLightComponent::setShadowCascadeCount(uint32_t count)
    {
        m_csmCascadesCount = count;
    }

    void DirectionalLightComponent::setCsmPowWeight(float weight)
    {
        m_csmPowWeight = weight;
    }

    math::Vector3 DirectionalLightComponent::getDirection() const
    {
        return  getWorldTransform().transformVector(f3Tov3(FORWARD));
    }

    math::Color3 DirectionalLightComponent::getColor() const
    {
        return math::Color3(m_red, m_green, m_blue);
    }

    float DirectionalLightComponent::getIntensity() const
    {
        return m_intensity;
    }

    bool DirectionalLightComponent::hasShadows() const
    {
        return m_castShadows;
    }

    uint32_t DirectionalLightComponent::getShadowMapSize() const
    {
        return m_csmSize;
    }

    uint32_t DirectionalLightComponent::getShadowCascadeCount() const
    {
        return m_csmCascadesCount;
    }

    float DirectionalLightComponent::getCsmPowWeight() const
    {
        return m_csmPowWeight;
    }

}  // namespace nau::scene
