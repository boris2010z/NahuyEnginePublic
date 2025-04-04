// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/asset_tools/interface/prim_processor.h"

namespace nau
{
    namespace prim_processors
    {
        class AnimationPrimProcessor : public IPrimProcessor
        {
        public:
            virtual std::string_view getType() const override
            {
                return "animation";
            }
            virtual bool canProcess(const nau::UsdMetaInfo& metaInfo) const override;
            virtual nau::Result<AssetMetaInfo> process(PXR_NS::UsdStageRefPtr stage, const std::string& outputPath, const std::string& projectRootPath, const nau::UsdMetaInfo& metaInfo, int folderIndex) override;
        };

        class SkeletalAnimationPrimProcessor : public IPrimProcessor
        {
        public:
            virtual std::string_view getType() const override
            {
                return "prim-animation-skeleton";
            }
            virtual bool canProcess(const nau::UsdMetaInfo& metaInfo) const override;
            virtual nau::Result<AssetMetaInfo> process(PXR_NS::UsdStageRefPtr stage, const std::string& outputPath, const std::string& projectRootPath, const nau::UsdMetaInfo& metaInfo, int folderIndex) override;
        };

        class GltfPrimProcessor : public IPrimProcessor
        {
        public:
            virtual std::string_view getType() const override
            {
                return "prim-gltf";
            }
            virtual bool canProcess(const nau::UsdMetaInfo& metaInfo) const override;
            virtual nau::Result<AssetMetaInfo> process(PXR_NS::UsdStageRefPtr stage, const std::string& outputPath, const std::string& projectRootPath, const nau::UsdMetaInfo& metaInfo, int folderIndex) override;
        };
    }  // namespace prim_processors
}  // namespace nau
