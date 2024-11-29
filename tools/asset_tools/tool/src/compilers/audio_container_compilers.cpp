// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/asset_tools/compilers/audio_container_compilers.h"

#include <EASTL/vector.h>
#include <nau/shared/logger.h>
#include <pxr/usd/sdf/fileFormat.h>

#include "nau/asset_tools/asset_utils.h"
#include "nau/asset_tools/db_manager.h"
#include "nau/diag/logging.h"

#include "nau/assets/animation_asset_accessor.h"
#include "nau/assets/ui_asset_accessor.h"
#include "nau/shared/file_system.h"

#include "nau/dataBlock/dag_dataBlock.h"

#include "nau/io/stream_utils.h"
#include "nau/io/virtual_file_system.h"
#include "nau/service/service_provider.h"
#include "nau/shared/file_system.h"
#include "nau/assets/asset_db.h"

#include "nau/usd_meta_tools/usd_meta_info.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "usd_proxy/usd_prim_proxy.h"

namespace nau::compilers
{
    // TODO True Sound compiler
    nau::Result<AssetMetaInfo> UsdAudioContainerCompiler::compile(
        PXR_NS::UsdStageRefPtr stage,
        const std::string& outputPath,
        const std::string& projectRootPath,
        const nau::UsdMetaInfo& metaInfo,
        int folderIndex)
    {
        AssetDatabaseManager& dbManager = AssetDatabaseManager::instance();
        NAU_ASSERT(dbManager.isLoaded(), "Asset database not loaded!");

        auto rootPrim = stage->GetPrimAtPath(pxr::SdfPath("/Root"));
        if (!rootPrim) {
            return NauMakeError("Can't load source stage from '{}'", metaInfo.assetPath);
        }

        auto proxyPrim = UsdProxy::UsdProxyPrim(rootPrim);

        std::string stringUID;
        auto uidProperty = proxyPrim.getProperty(pxr::TfToken("uid"));
        if (uidProperty)
        {
            pxr::VtValue val;
            uidProperty->getValue(&val);

            if (val.IsHolding<std::string>())
            {
                stringUID = val.Get<std::string>();
            }
        }

        AssetMetaInfo nsoundMeta;
        const std::string relativeSourcePath = FileSystemExtensions::getRelativeAssetPath(metaInfo.assetPath, false).string();
        const std::string sourcePath = std::format("{}", relativeSourcePath.c_str());

        auto id = dbManager.findIf(sourcePath);

        if (id.isError() && !stringUID.empty())
        {
            id = Uid::parseString(stringUID);
        }

        const std::filesystem::path out = std::filesystem::path(outputPath) / std::to_string(folderIndex);
        std::string fileName = toString(*id) + ".nsound";

        nsoundMeta.uid = *id;
        nsoundMeta.dbPath = (out.filename() / fileName).string().c_str();
        nsoundMeta.sourcePath = sourcePath.c_str();
        nsoundMeta.nausdPath = (sourcePath + ".nausd").c_str();
        nsoundMeta.dirty = false;
        nsoundMeta.kind = "AudioContainer";

        // Load asset
        const auto stageAsset = PXR_NS::UsdStage::Open(metaInfo.assetPath);
        if (!stageAsset) {
            return NauMakeError("Invalid container prim path: {}", metaInfo.assetPath);
        }

        const auto primRaw = stageAsset->GetPrimAtPath(pxr::SdfPath("/AudioContainer"));
        if (!primRaw.IsValid()) {
            return NauMakeError("Invalid audio container prim at: {}", metaInfo.assetPath);
        }

        auto assetProxyPrim = UsdProxy::UsdProxyPrim(primRaw);
        
        auto outFilePath = utils::compilers::ensureOutputPath(outputPath, nsoundMeta, "");

        DataBlock outBlk;

        // Kind
        const auto containerKindProp = assetProxyPrim.getProperty(PXR_NS::TfToken("containerKind"));
        if (!containerKindProp) {
            return NauMakeError("Invalid audio container prim kind property");
        }

        PXR_NS::VtValue kindValue;
        if (!containerKindProp->getValue(&kindValue)) {
            return NauMakeError("Failed to retrieve audio container kind");
        }

        const auto kindToken = kindValue.Get<PXR_NS::TfToken>();
        outBlk.setStr("kind", kindToken.data());

        // Sources
        DataBlock* sourcesBlk = outBlk.addBlock("sources");
        const auto sourcesProp = assetProxyPrim.getProperty(PXR_NS::TfToken("sources"));
        if (!sourcesProp) {
            NAU_LOG_ERROR("Invalid audio container prim sources property");
        }

        PXR_NS::VtValue sourcesValue;
        if (!sourcesProp->getValue(&sourcesValue)) {
            NAU_LOG_ERROR("Failed to retrieve audio container sources");
        }

        PXR_NS::VtArray<PXR_NS::SdfAssetPath> sourcePaths;
        sourcePaths = sourcesValue.Get<PXR_NS::VtArray<PXR_NS::SdfAssetPath>>();

        auto& assetDb = nau::getServiceProvider().get<nau::IAssetDB>();
        auto& vfs = nau::getServiceProvider().get<io::IVirtualFileSystem>();
        
        for (const auto sdfPath : sourcePaths)
        {
            // Get uid from source path
            const std::string path = sdfPath.GetAssetPath().c_str();
            if (path.empty()) continue;
            const auto relative = nau::FileSystemExtensions::getRelativeAssetPath(std::filesystem::path(path), true).string();
            const auto uid = assetDb.getUidFromSourcePath(relative.c_str());

            // Add to blk
            auto sourceBlk = sourcesBlk->addBlock("source");
            sourceBlk->setStr("uid", toString(uid).c_str());
        }

        const bool result = outBlk.saveToTextFile(outFilePath.string().c_str());

        if (result)
        {
            dbManager.addOrReplace(nsoundMeta);
            return nsoundMeta;
        }

        return NauMakeError("Sound asset loading failed");
    }

}  // namespace nau::compilers
