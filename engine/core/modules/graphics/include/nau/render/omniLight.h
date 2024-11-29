// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <corecrt_math_defines.h>

#include "nau/math/dag_color.h"
#include "nau/render/dag_hlsl_floatx.h"
#include "nau/render/renderLightsConsts.hlsli"

namespace nau::render
{
    using nau::math::Color4;
    using nau::math::Color3;

    struct OmniLight
    {
        static constexpr float DEFAULT_BOX_SIZE = 100000;
        alignas(16) float4 pos_radius;
        alignas(16) Color4 color_atten;
        alignas(16) float4 dir__tex_scale;
        alignas(16) float4 boxR0;
        alignas(16) float4 boxR1;
        alignas(16) float4 boxR2;
        alignas(16) float4 posRelToOrigin_cullRadius;
        bool debugDraw = false;
        OmniLight()
        {
        }
        OmniLight(const float3& p,
                  const Color3& col,
                  float rad,
                  float att,
                  float intensity,
                  bool debugDraw) :
            pos_radius(p.x, p.y, p.z, rad),
            color_atten(col.r * intensity, col.g * intensity, col.b * intensity, att),
            dir__tex_scale(0, 1, 0, 0),
            posRelToOrigin_cullRadius(0, 0, 0, -1),
            debugDraw(debugDraw)
        {
            setDefaultBox();
        }
        OmniLight(const float3& p,
                  const Color3& col,
                  float rad,
                  float att,
                  float intensity,
                  const float4x4& box,
                  bool debugDraw) :
            pos_radius(p.x, p.y, p.z, rad),
            color_atten(col.r * intensity, col.g * intensity, col.b * intensity, att),
            dir__tex_scale(0, 1, 0, 0),
            posRelToOrigin_cullRadius(0, 0, 0, -1),
            debugDraw(debugDraw)
        {
            setBox(box);
        }
        OmniLight(const float3& p,
                  const Color3& col,
                  float rad,
                  float att,
                  float intensity,
                  const float4x4* box,
                  bool debugDraw) :
            pos_radius(p.x, p.y, p.z, rad),
            color_atten(col.r * intensity, col.g * intensity, col.b * intensity, att),
            dir__tex_scale(0, 1, 0, 0),
            posRelToOrigin_cullRadius(0, 0, 0, -1),
            debugDraw(debugDraw)
        {
            if (box != nullptr && lengthSqr(box->getCol(0)) > 0)
                setBox(*box);
            else
                setDefaultBox();
        }
        OmniLight(const float3& p,
                  const float3& dir,
                  const Color3& col,
                  float rad,
                  float att,
                  float intensity,
                  int tex,
                  float texture_scale,
                  bool tex_rotation,
                  bool debugDraw) :
            pos_radius(p.x, p.y, p.z, rad),
            color_atten(col.r * intensity, col.g * intensity, col.b * intensity, att),
            dir__tex_scale(dir.x, dir.y, dir.z, 0.f),
            posRelToOrigin_cullRadius(0, 0, 0, -1),
            debugDraw(debugDraw)
        {
            setDefaultBox();
            setTexture(tex, texture_scale, tex_rotation);
        }
        OmniLight(const float3& p,
                  const float3& dir,
                  const Color3& col,
                  float rad,
                  float att,
                  float intensity,
                  int tex,
                  float texture_scale,
                  bool tex_rotation,
                  const float4x4& box,
                  bool debugDraw) :
            pos_radius(p.x, p.y, p.z, rad),
            color_atten(col.r * intensity, col.g * intensity, col.b * intensity, att),
            dir__tex_scale(dir.x, dir.y, dir.z, 0.f),
            posRelToOrigin_cullRadius(0, 0, 0, -1),
            debugDraw(debugDraw)
        {
            setBox(box);
            setTexture(tex, texture_scale, tex_rotation);
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
        void setColor(const Color3& c, float intensity = 1)
        {
            color_atten.r = c.r * intensity;
            color_atten.g = c.g * intensity;
            color_atten.b = c.b * intensity;
        }
        void setZero()
        {
            pos_radius = float4(0, 0, 0, 0);
            color_atten = Color4(0, 0, 0, 0);
        }
        void setDirection(const float3& dir)
        {
            dir__tex_scale.x = dir.x;
            dir__tex_scale.y = dir.y;
            dir__tex_scale.z = dir.z;
        }
        void setTexture(int tex, float scale, bool rotation)
        {
            if (tex < 0)
                dir__tex_scale.w = 0;
            else
            {
                NAU_ASSERT(scale >= M_SQRT1_2 && scale < TEX_ID_MULTIPLIER, "Invalid ies scale value: {}", scale);
                dir__tex_scale.w = float(tex) * TEX_ID_MULTIPLIER + scale;
                if (rotation)
                    dir__tex_scale.w *= -1;
            }
        }
        void setBox(const float4x4& box)
        {
            float3 len = float3{lengthSqr(box.getCol(0)), lengthSqr(box.getCol(1)), lengthSqr(box.getCol(2))};
            if (std::min(len.x, std::min(len.y, len.z)) > 0)
            {
                boxR0 = float4{box.getCol(0).getX() / len.getX(), box.getCol(1).getX() / len.getY(), box.getCol(2).getX() / len.getZ(), box.getCol(3).getX()};
                boxR1 = float4{box.getCol(0).getY() / len.getX(), box.getCol(1).getY() / len.getY(), box.getCol(2).getY() / len.getZ(), box.getCol(3).getY()};
                boxR2 = float4{box.getCol(0).getZ() / len.getX(), box.getCol(1).getZ() / len.getY(), box.getCol(2).getZ() / len.getZ(), box.getCol(3).getZ()};
            }
            else
            {
                boxR0 = float4{0, 0, 0, 0};
                boxR1 = float4{0, 0, 0, 0};
                boxR2 = float4{0, 0, 0, 0};
            }
        }
        void setDefaultBox()
        {
            setBoxAround(float3{pos_radius.x, pos_radius.y, pos_radius.z}, DEFAULT_BOX_SIZE);
        }
        void setBoxAround(const float3& p, float size)
        {
            float4x4 box(size);
            box.setCol(3, nau::math::Vector4(p.x, p.y, p.z, 1.f));
            setBox(box);
        }
        void setPosRelToOrigin(const float3& pos, float cullRadius)
        {
            posRelToOrigin_cullRadius.x = pos.x;
            posRelToOrigin_cullRadius.y = pos.y;
            posRelToOrigin_cullRadius.z = pos.z;
            posRelToOrigin_cullRadius.w = cullRadius;
        }
        static OmniLight create_empty()
        {
            OmniLight l;
            l.setZero();
            return l;
        }
    };
}