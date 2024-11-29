// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/assets/asset_ref.h"
#include "nau/math/math.h"
#include "nau/scene/components/component.h"

namespace nau::scene
{
    class NAU_CORESCENE_EXPORT EnvironmentComponent : public Component
    {
        NAU_OBJECT(nau::scene::EnvironmentComponent, Component)
        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_textureAsset, "texture"),
            CLASS_NAMED_FIELD(m_enviIntensity, "environment_intensity"))

    public:
        TextureAssetRef getTextureAsset() const;
        void setTextureAsset(const TextureAssetRef& texture);

        bool isTextureDirty() const;
        void resetIsTextureDirty();

        void setIntensity(float intensity);
        float getIntensity() const;

    private:
        TextureAssetRef m_textureAsset;
        float m_enviIntensity = 1.0f;

        bool m_isTextureDirty = true;
    };
}  // namespace nau::scene
