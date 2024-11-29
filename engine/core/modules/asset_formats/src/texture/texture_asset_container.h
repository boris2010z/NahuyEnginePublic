// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/assets/asset_container.h"
#include "nau/assets/texture_asset_accessor.h"
#include "nau/io/stream.h"
#include "nau/meta/class_info.h"
#include "nau/rtti/rtti_impl.h"
#include "texture_source_data.h"

namespace nau
{
    /**
     */
    struct ImportSettings
    {
        bool generateMipmaps = true;
        bool isCompressed = true;

#pragma region Class Info
        NAU_CLASS_FIELDS(
            CLASS_FIELD(generateMipmaps),
            CLASS_FIELD(isCompressed))
#pragma endregion
    };

    /**
     */
    class TextureAssetContainer final : public IAssetContainer,
                                        public ITextureAssetAccessor
    {
        NAU_CLASS_(nau::TextureAssetContainer, IAssetContainer, ITextureAssetAccessor)

    public:
        TextureAssetContainer(TextureSourceData textureData);

    private:
        TextureDescription getDescription() const override;

        void copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, eastl::span<DestTextureData> destination) override;

        nau::Ptr<> getAsset(eastl::string_view) override;

        eastl::vector<eastl::string> getContent() const override;

        TextureSourceData m_textureData;
    };

    /**
     */
    class TextureAssetContainerLoader final : public IAssetContainerLoader
    {
        NAU_INTERFACE(nau::TextureAssetContainerLoader, IAssetContainerLoader)

    public:
        TextureAssetContainerLoader() = default;

    private:
        eastl::vector<eastl::string_view> getSupportedAssetKind() const override;

        async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr, AssetContentInfo info) override;

        RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const override;
    };
}  // namespace nau
