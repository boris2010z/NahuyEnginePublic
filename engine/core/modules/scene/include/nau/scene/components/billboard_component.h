// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/assets/asset_ref.h"
#include "nau/scene/components/scene_component.h"

namespace nau::scene
{
    class NAU_CORESCENE_EXPORT BillboardComponent : public SceneComponent
    {
        NAU_OBJECT(nau::scene::BillboardComponent, SceneComponent)
        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_texture, "texture"),
            CLASS_NAMED_FIELD(m_isVisible, "visibility"),
            CLASS_NAMED_FIELD(m_screenPercentageSize, "screenPercentageSize"),
        )

    public:
        void setTextureRef(nau::TextureAssetRef assetRef);
        nau::TextureAssetRef getTextureRef() const;

        void setScreenPercentageSize(float newScreenPercSize);
        float getScreenPercentageSize();

        void setVisibility(bool isVisible);
        bool getVisibility() const;

        bool isTextureDirty() const;
        void resetIsTextureDirty();

    private:
        mutable nau::TextureAssetRef m_texture;
        mutable float m_screenPercentageSize;
        mutable bool m_isVisible = true;
        bool isBillboardTextureDirty;
    };
}  // namespace nau
