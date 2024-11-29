// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/assets/asset_ref.h"
#include "nau/math/dag_color.h"
#include "nau/math/math.h"
#include "nau/scene/components/scene_component.h"

namespace nau::scene
{
    class NAU_CORESCENE_EXPORT DirectionalLightComponent : public SceneComponent
    {
        NAU_OBJECT(nau::scene::DirectionalLightComponent, SceneComponent)
        NAU_DECLARE_DYNAMIC_OBJECT

        static inline constexpr nau::math::float3 FORWARD = {1.0f, 0.0f, 0.0f};

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_red, "colorRed"),
            CLASS_NAMED_FIELD(m_green, "colorGreen"),
            CLASS_NAMED_FIELD(m_blue, "colorBlue"),
            CLASS_NAMED_FIELD(m_intensity, "intensity"),
            CLASS_NAMED_FIELD(m_castShadows, "castShadows"),
            CLASS_NAMED_FIELD(m_csmSize, "shadow map resolution"),
            CLASS_NAMED_FIELD(m_csmCascadesCount, "cascades count"),
            CLASS_NAMED_FIELD(m_csmPowWeight, "lin to log weight"))

    public:

        void setColor(const math::Color3& color);
        void setIntensity(float intensity);
        void setCastShadows(bool hasShadows);
        void setShadowMapSize(uint32_t size);
        void setShadowCascadeCount(uint32_t count);
        void setCsmPowWeight(float weight);

        math::Vector3 getDirection() const;
        math::Color3 getColor() const;
        float getIntensity() const;
        uint32_t getShadowMapSize() const;
        uint32_t getShadowCascadeCount() const;
        float getCsmPowWeight() const;

        bool hasShadows() const;

    private:
        //math::Color3 m_color = {};
        float m_red = 1;
        float m_green = 1;
        float m_blue = 1;

        float m_intensity = 1;
        bool m_castShadows = false;
        uint32_t m_csmSize = 1024;
        uint32_t m_csmCascadesCount = 4; // 4 maximum
        float m_csmPowWeight = 0.985f;
    };
}  // namespace nau::scene
