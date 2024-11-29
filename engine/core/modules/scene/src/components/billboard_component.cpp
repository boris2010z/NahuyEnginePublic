// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/scene/components/billboard_component.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(BillboardComponent)


    void BillboardComponent::setTextureRef(nau::TextureAssetRef assetRef)
    {
        m_texture = assetRef;
        isBillboardTextureDirty = true;
    }

    nau::TextureAssetRef BillboardComponent::getTextureRef() const
    {
        return m_texture;
    }

    void BillboardComponent::setScreenPercentageSize(float newScreenPercSize)
    {
        m_screenPercentageSize = newScreenPercSize;
    }

    float BillboardComponent::getScreenPercentageSize()
    {
        return m_screenPercentageSize;
    }

    void BillboardComponent::setVisibility(bool isVisible)
    {
        m_isVisible = isVisible;
    }

    bool BillboardComponent::getVisibility() const
    {
        return m_isVisible;
    }

    bool BillboardComponent::isTextureDirty() const
    {
        return isBillboardTextureDirty;
    }

    void BillboardComponent::resetIsTextureDirty()
    {
        isBillboardTextureDirty = false;
    }

}  // namespace nau
