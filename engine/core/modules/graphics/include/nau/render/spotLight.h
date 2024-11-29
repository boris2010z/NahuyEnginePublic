// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <corecrt_math_defines.h>

#include "nau/math/dag_color.h"
#include "nau/render/dag_hlsl_floatx.h"
#include "nau/render/renderLightsConsts.hlsli"

namespace nau::render
{
    using nau::math::Color3;
    using nau::math::Color4;

    struct SpotLight
    {
        alignas(16) float4 pos_radius;
        alignas(16) float4 color_atten;
        alignas(16) float4 dir_angle;
        alignas(16) float4 texId_scale;
        float culling_radius;
        bool contactShadows = false;
        bool debugDraw = false;
        SpotLight() :
            culling_radius(-1.0f),
            texId_scale(-1, 0, 0, 0)
        {
        }
        SpotLight(const float3& position,
                  const Color3& color,
                  float radius,
                  float intensity,
                  float innerAttenuation,
                  const float3& direction,
                  float angle,
                  bool contact_shadows,
                  bool debugDraw) :
            pos_radius(position.x, position.y, position.z, radius),
            color_atten(float3(color.r, color.g, color.b) * intensity, cosf(innerAttenuation)),
            dir_angle(direction.x, direction.y, direction.z, tanf(angle / 2.f)),
            texId_scale(-1, 0, 0, 0),
            culling_radius(-1.0f),
            contactShadows(contact_shadows),
            debugDraw(debugDraw)
        {
        }
        SpotLight(const float3& p,
                  const Color3& col,
                  float rad,
                  float intensity,
                  float inner_att,
                  const float3& dir,
                  float angle,
                  bool contact_shadows,
                  int tex,
                  float texture_scale,
                  bool tex_rotation,
                  bool debugDraw) :
            SpotLight(p, col, rad, intensity, inner_att, dir, angle, contact_shadows, debugDraw)
        {
            setTexture(tex, texture_scale, tex_rotation);
        }
        void setTexture(int tex, float scale, bool rotation)
        {
            texId_scale = float4(float(tex), rotation ? -scale : scale, 0.f, 0.f);
        }
        void setPos(const float3& p)
        {
            pos_radius.x = p.x;
            pos_radius.y = p.y;
            pos_radius.z = p.z;
        }
        void setRadius(float rad)
        {
            pos_radius.w = rad;
        }
        void setCullingRadius(float rad)
        {
            culling_radius = rad;
        }
        void setColor(const Color3& c, float intensity = 1)
        {
            color_atten.x = c.r * intensity;
            color_atten.y = c.g * intensity;
            color_atten.z = c.b * intensity;
        }
        void setZero()
        {
            pos_radius = float4(0, 0, 0, 0);
            color_atten = float4(0, 0, 0, 0);
            dir_angle = float4(0, 0, 1, 1);
            texId_scale = float4(-1, 0, 0, 0);
        }
        static SpotLight create_empty()
        {
            SpotLight l;
            l.setZero();
            return l;
        }
        struct BoundingSphereDescriptor
        {
            float boundSphereRadius;
            float boundingSphereOffset;  // from pos, along dir
        };
        static BoundingSphereDescriptor get_bounding_sphere_description(float light_radius, float sin_half_angle, float cos_half_angle)
        {
            constexpr float COS_PI_4 = 0.70710678118654752440084436210485;
            BoundingSphereDescriptor ret;
            if (cos_half_angle > COS_PI_4)  // <=> angle/2 < 45 degrees
            {
                // use circumcircle of the spotlight cone
                // the light position is on the surface of the bounding sphere
                ret.boundSphereRadius = light_radius / (2.f * cos_half_angle);
                ret.boundingSphereOffset = ret.boundSphereRadius;
            }
            else
            {
                // only consider the spherical sector
                // the light position is inside of the bounding sphere
                ret.boundSphereRadius = sin_half_angle * light_radius;
                ret.boundingSphereOffset = cos_half_angle * light_radius;
            }
            return ret;
        }
        float getCosHalfAngle() const
        {
            return 1.f / sqrtf(1 + dir_angle.w * dir_angle.w);
        }
        // cone bounding sphere; cosHalfAngle can be precalculated to speed up this process
        float4 getBoundingSphere(float cosHalfAngle) const
        {
            // TODO: further vectorize this (incl getCosHalfAngle()/get_bounding_sphere_description())
            /* TODO: fix this later
            float lightRadius = culling_radius < 0 ? pos_radius.w : culling_radius;
            float sinHalfAngle = dir_angle.w * cosHalfAngle;
            BoundingSphereDescriptor desc = get_bounding_sphere_description(lightRadius, sinHalfAngle, cosHalfAngle);
            float4 center = dir_angle * desc.boundingSphereOffset + pos_radius;
            center.w = desc.boundSphereRadius;
             */
            return pos_radius;
        }
        // cone bounding sphere
        float4 getBoundingSphere() const
        {
            return getBoundingSphere(getCosHalfAngle());
        }
    };

}  // namespace nau::render