// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/asset_tools/compilers/ui_compilers.h"
#include <format>
#include "nau/diag/assertion.h"
#include "nau/diag/error.h"
#include "nau/diag/logging.h"
#include "nau/usd_meta_tools/usd_meta_info.h"
#include "nau/service/service_provider.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/asset_tools/asset_utils.h"

#include "pxr/usd/sdf/payload.h"
#include "usd_proxy/usd_prim_proxy.h"

namespace nau::compilers
{
    using translateUIScene = void (*)(PXR_NS::UsdStageRefPtr stage, nau::DataBlock& blk);
    translateUIScene getTranslatorFunction()
    {
        void* plugin = utils::getUsdPlugin("UsdTranslatorWrapper.dll");

        if (!plugin)
        {
            return nullptr;
        }
#ifdef WIN32
        auto proc = GetProcAddress(reinterpret_cast<HMODULE>(plugin), "translateUIScene");
#else
        auto proc = dlsym(plugin, "translateUIScene");
#endif
        if (translateUIScene func = reinterpret_cast<translateUIScene>(proc))
        {
            return func;
        }

        return nullptr;
    }

    nau::Result<AssetMetaInfo> UiCompiler::compile(PXR_NS::UsdStageRefPtr stage, const std::string& outputPath, const std::string& projectRootPath, const nau::UsdMetaInfo& metaInfo, int folderIndex)
    {
        translateUIScene translateFn = getTranslatorFunction();
        if (!translateFn)
        {
            return NauMakeError("Failed to get translator function from plugin! Plugin does not exist or is not loaded!");
        }

        const std::string path = metaInfo.assetPath;
        PXR_NS::UsdStageRefPtr uiSceneStage = PXR_NS::UsdStage::Open(path);

        const std::filesystem::path resourcesContentPath = std::filesystem::path(projectRootPath) / "resources";
        std::filesystem::path exprotPath = resourcesContentPath / FileSystemExtensions::replaceExtension(metaInfo.assetSourcePath, ext().data());

        DataBlock blk;
        translateFn(uiSceneStage, blk);

        if (!blk.saveToTextFile(exprotPath.string().c_str()))
        {
            return NauMakeError("Failed to save UI scene asset to file {}", exprotPath.string());
        }

        auto rootPrim = stage->GetPrimAtPath(pxr::SdfPath("/Root"));
        if (!rootPrim)
        {
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

        const std::string relativeSourcePathWithoutExt = FileSystemExtensions::getRelativeAssetPath(metaInfo.assetPath, true).string();

        const std::string relativeSourcePath = FileSystemExtensions::getRelativeAssetPath(metaInfo.assetPath, false).string();
        const std::string sourcePath = std::format("{}", relativeSourcePath.c_str());

        const std::filesystem::path out = std::filesystem::path(outputPath) / std::to_string(folderIndex);
        std::string fileName = stringUID + ".nui";

        AssetMetaInfo uiMeta;
        uiMeta.uid = *Uid::parseString(stringUID);
        uiMeta.dbPath = (out.filename() / fileName).string().c_str();
        uiMeta.sourcePath = std::format("{}", relativeSourcePathWithoutExt.c_str()).c_str();
        uiMeta.nausdPath = (sourcePath + ".nausd").c_str();
        uiMeta.dirty = false;
        uiMeta.kind = "UI";

        return uiMeta;

        //return makeAssetMetaInfo(path, *Uid::parseString(stringUID), std::format("{}/{}{}", folderIndex, stringUID, ext()), "nausd", "ui");
    }
} // namespace nau::compilers
