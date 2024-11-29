// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/assets/asset_ref.h"
#include "nau/math/dag_color.h"
#include "nau/scene/components/scene_component.h"

namespace nau::scene
{
    class NAU_CORESCENE_EXPORT OmnilightComponent : public SceneComponent
    {
        NAU_OBJECT(nau::scene::OmnilightComponent, SceneComponent)
        NAU_DECLARE_DYNAMIC_OBJECT
    public:


        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_shift, "shift"),
            CLASS_NAMED_FIELD(m_red, "colorRed"),
            CLASS_NAMED_FIELD(m_green, "colorGreen"),
            CLASS_NAMED_FIELD(m_blue, "colorBlue"),
            CLASS_NAMED_FIELD(m_radius, "radius"),
            CLASS_NAMED_FIELD(m_intensity, "intensity"),
            CLASS_NAMED_FIELD(m_attenuation, "attenuation"),
            CLASS_NAMED_FIELD(m_debugDraw, "debugDraw"), )
    public:
        math::Vector3 getShift() const;
        math::Color3 getColor() const;
        float getRadius() const;
        float getAttenuation() const;
        float getIntensity() const;
        bool getDebugDraw() const;

        void setShift(math::Vector3);
        void setColor(math::Color3);
        void setRadius(float);
        void setAttenuation(float);
        void setIntensity(float);
        void setDebugDraw(bool);
    private:
        math::Vector3 m_shift = {};

        //math::Color3 m_color = {};
        float m_red = 1;
        float m_green = 1;
        float m_blue = 0;

        float m_radius = 1;
        float m_intensity = 1;
        float m_attenuation = 0.5;

        bool m_debugDraw = false;
    };
}  // namespace nau::scene
