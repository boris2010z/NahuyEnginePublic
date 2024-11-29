// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include "nau/math/dag_bounds3.h"
#include "nau/math/dag_frustum.h"
#include "nau/render/spotLight.h"

namespace nau::render
{
#include "nau/render/renderLights.hlsli"
}

#include <EASTL/bitset.h>
#include <EASTL/vector.h>

#include "nau/utils/dag_relocatableFixedVector.h"

/* TODO: Support photometry
#include <render/iesTextureManager.h>
*/

namespace nau::render
{
    class OmniShadowMap;
    class Occlusion;

    class SpotLightsManager
    {
    public:
        typedef SpotLight Light;
        typedef Light RawLight;
        typedef uint8_t mask_type_t;

        static constexpr int MAX_LIGHTS = 2048;
        enum
        {
            GI_LIGHT_MASK = 0x1,  // TODO: maybe rename it
            MASK_ALL = 0xFF
        };

        SpotLightsManager();
        ~SpotLightsManager();

        void init();
        void close();

        template <bool use_small>
        void prepare(const nau::math::NauFrustum& frustum,
                     eastl::vector<uint16_t>& lights_inside_plane,
                     eastl::vector<uint16_t>& lights_outside_plane,
                     eastl::bitset<MAX_LIGHTS>* visibleIdBitset,
                     Occlusion*,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     nau::math::Vector4 znear_plane,
                     const dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS>& shadow,
                     float markSmallLightsAsFarLimit,
                     nau::math::Point3 cameraPos,
                     mask_type_t accept_mask) const;

        void prepare(const nau::math::NauFrustum& frustum,
                     eastl::vector<uint16_t>& lights_inside_plane,
                     eastl::vector<uint16_t>& lights_outside_plane,
                     eastl::bitset<MAX_LIGHTS>* visibleIdBitset,
                     Occlusion* occ,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     nau::math::Vector4 znear_plane,
                     const dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS>& shadow,
                     float markSmallLightsAsFarLimit,
                     nau::math::Point3 cameraPos,
                     mask_type_t accept_mask) const
        {
            prepare<true>(frustum,
                          lights_inside_plane,
                          lights_outside_plane,
                          visibleIdBitset,
                          occ,
                          inside_box,
                          outside_box,
                          znear_plane,
                          shadow,
                          markSmallLightsAsFarLimit,
                          cameraPos,
                          accept_mask);
        }

        void prepare(const nau::math::NauFrustum& frustum,
                     eastl::vector<uint16_t>& lights_inside_plane,
                     eastl::vector<uint16_t>& lights_outside_plane,
                     eastl::bitset<MAX_LIGHTS>* visibleIdBitset,
                     Occlusion* occ,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     nau::math::Vector4 znear_plane,
                     const dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS>& shadow,
                     mask_type_t accept_mask)
        {
            prepare<false>(frustum,
                           lights_inside_plane,
                           lights_outside_plane,
                           visibleIdBitset,
                           occ,
                           inside_box,
                           outside_box,
                           znear_plane,
                           shadow,
                           0,
                           {},
                           accept_mask);
        }
        void renderDebugBboxes();
        int addLight(const Light& light);  // return -1 if fails
        void destroyLight(unsigned int id);

        const Light& getLight(unsigned int id) const
        {
            return rawLights[id];
        }

        void setLight(unsigned int id, const Light& l)
        {
            if (isnan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
            {
                NAU_FAILURE("nan in setLight");
                return;
            }
            rawLights[id] = l;
            resetLightOptimization(id);
            updateBoundingSphere(id);
        }

        RenderSpotLight getRenderLight(unsigned int id) const
        {
            const Light& l = rawLights[id];
            const float cosInner = l.color_atten.w;
            const float cosOuter = cosHalfAngles[id];
            const float lightAngleScale = 1.0f / std::max(0.001f, (cosInner - cosOuter));
            const float lightAngleOffset = -cosOuter * lightAngleScale;
            RenderSpotLight ret;
            ret.lightPosRadius = (const float4&)l.pos_radius;
            ret.lightColorAngleScale = (const float4&)l.color_atten;
            ret.lightColorAngleScale.w = lightAngleScale * (l.contactShadows ? -1 : 1);
            ret.lightDirectionAngleOffset = (const float4&)l.dir_angle;
            ret.lightDirectionAngleOffset.w = lightAngleOffset;
            ret.texId_scale = (const float4&)l.texId_scale;
            return ret;
        }

        void updateBoundingSphere(unsigned id)
        {
            const Light& l = rawLights[id];
            cosHalfAngles[id] = l.getCosHalfAngle();
            boundingSpheres[id] = l.getBoundingSphere(cosHalfAngles[id]).toVec4();
            updateBoundingBox(id);
        }
        void updateBoundingBox(unsigned id);
        nau::math::BBox3 getBoundingBox(unsigned id) const
        {
            return boundingBoxes[id];
        }
        nau::math::Vector4 getBoundingSphere(unsigned id) const
        {
            return boundingSpheres[id];
        }

        void destroyAllLights();

        int addLight(const nau::math::Point3& pos,
                     const Color3& color,
                     const nau::math::Point3& dir,
                     const float angle,
                     float radius,
                     float attenuation_k = 1.f,
                     bool contact_shadows = false,
                     int tex = -1);

        void setLightPos(unsigned int id, const nau::math::Point3& pos)
        {
            if (isnan(sum(pos)))
            {
                NAU_FAILURE("nan in setLightPos");
                return;
            }
            rawLights[id].pos_radius.x = pos.getX();
            rawLights[id].pos_radius.y = pos.getY();
            rawLights[id].pos_radius.z = pos.getZ();
            resetLightOptimization(id);
            updateBoundingSphere(id);
        }
        const mask_type_t getLightMask(unsigned int id) const
        {
            return masks[id];
        }
        void setLightMask(unsigned int id, mask_type_t mask)
        {
            masks[id] = mask;
        }
        const nau::math::Point3 getLightPos(unsigned int id) const
        {
            return nau::math::Point3(rawLights[id].pos_radius.toVec4());
        }
        const nau::math::Vector4 getLightPosRadius(unsigned int id) const
        {
            return rawLights[id].pos_radius.toVec4();
        }
        void getLightView(unsigned int id, nau::math::Matrix4& viewITM);
        void getLightPersp(unsigned int id, nau::math::Matrix4& proj);
        void setLightDirAngle(unsigned int id, const nau::math::Vector4& dir_angle)
        {
            rawLights[id].dir_angle = float4(dir_angle);
            resetLightOptimization(id);
            updateBoundingSphere(id);
        }
        nau::math::Vector4 getLightDirAngle(unsigned int id) const
        {
            return rawLights[id].dir_angle.toVec4();
        }
        void setLightCol(unsigned int id, const Color3& col, float intensity = 1)
        {
            rawLights[id].color_atten.x = col.r * intensity;
            rawLights[id].color_atten.y = col.g * intensity;
            rawLights[id].color_atten.z = col.b * intensity;
        }
        void setLightPosAndCol(unsigned int id, const nau::math::Point3& pos, const Color3& color)
        {
            setLightPos(id, pos);
            setLightCol(id, color);
            updateBoundingSphere(id);
        }
        void setLightRadius(unsigned int id, float radius)
        {
            if (isnan(radius))
            {
                NAU_FAILURE("nan in setLightRadius");
                return;
            }
            rawLights[id].pos_radius.w = radius;
            resetLightOptimization(id);
            updateBoundingSphere(id);
        }
        void setLightCullingRadius(unsigned int id, float radius)
        {
            rawLights[id].culling_radius = radius;
            updateBoundingSphere(id);
        }

        void removeEmpty();
        int maxIndex() const
        {
            return maxLightIndex;
        }

        bool isLightNonOptimized(int id)
        {
            return nonOptLightIds.test(id);
        }
        bool tryGetNonOptimizedLightId(int& id)
        {
            if (nonOptLightIds.any())
            {
                id = nonOptLightIds.find_first();
                return true;
            }
            return false;
        }
        void setLightOptimized(int id)
        {
            nonOptLightIds.set(id, false);
        }
        void resetLightOptimization(int id)
        {
            // Additional checks if optimization is needed can be added here
            bool shouldBeOptimized = (rawLights[id].pos_radius.w > 0) && (masks[id] & GI_LIGHT_MASK);
            nonOptLightIds.set(id, shouldBeOptimized);
            rawLights[id].culling_radius = -1.0f;
        }

        /* TODO: Support photometry
        IesTextureCollection::PhotometryData getPhotometryData(int texId) const;
        */

    private:
        eastl::array<Light, MAX_LIGHTS> rawLights;                     //-V730_NOINIT
        eastl::array<nau::math::Vector4, MAX_LIGHTS> boundingSpheres;  //-V730_NOINIT
        eastl::array<nau::math::BBox3, MAX_LIGHTS> boundingBoxes;      //-V730_NOINIT
        alignas(16) eastl::array<float, MAX_LIGHTS> cosHalfAngles;     //-V730_NOINIT
        // masks allows to ignore specific lights in specific cases
        // for example, we can ignore highly dynamic lights for GI
        eastl::array<mask_type_t, MAX_LIGHTS> masks;  //-V730_NOINIT

        dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS> freeLightIds;  //-V730_NOINIT
        eastl::bitset<MAX_LIGHTS> nonOptLightIds;
        /* TODO: Support photometry
         IesTextureCollection* photometryTextures = nullptr;
        */
        int maxLightIndex = -1;
    };
}  // namespace nau::render