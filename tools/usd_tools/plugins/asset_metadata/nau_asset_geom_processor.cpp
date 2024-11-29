// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau_asset_geom_processor.h"

#include "nau/NauAssetMetadata/nauAssetMesh.h"
#include "nau/NauAssetMetadata/nauAssetGroup.h"

#include "usd_proxy/usd_proxy.h"

#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/bindingAPI.h>

#include <type_traits>

bool NauAssetGroupProcessor::process(PXR_NS::UsdPrim prim, nau::UsdMetaInfo& dest)
{
    PXR_NS::UsdNauAssetGroup group(prim);
    if (!group)
    {
        return false;
    }
    auto info = std::make_shared<nau::ExtraInfoGroup>();

    dest.type = "group";
    dest.extraInfo = info;
    return true;
}

bool NauAssetMeshProcessor::process(PXR_NS::UsdPrim prim, nau::UsdMetaInfo& dest)
{
    PXR_NS::UsdNauAssetMesh mesh(prim);
    if (!mesh)
    {
        return false;
    }

    auto info = std::make_shared<nau::ExtraInfoMesh>();
    mesh.GetGenerateColliderAttr().Get(&info->generateCollider);
    mesh.GetGenerateLodsAttr().Get(&info->generateLods);
    mesh.GetGenerateTangentsAttr().Get(&info->generateTangents);
    mesh.GetIgnoreAnimationAttr().Get(&info->ignoreAnimation);
    mesh.GetUnitScaleAttr().Get(&info->unitScale);
    mesh.GetFlipUAttr().Get(&info->flipU);
    mesh.GetFlipVAttr().Get(&info->flipV);
    mesh.GetSkinnedAttr().Get(&info->skinned);

    PXR_NS::TfToken upAxis;
    mesh.GetUpAxisAttr().Get(&upAxis);
    info->upAxis =
        upAxis == "X"_tftoken ? nau::ExtraInfoMesh::UpAxis::X :
        upAxis == "Y"_tftoken ? nau::ExtraInfoMesh::UpAxis::Y :
        nau::ExtraInfoMesh::UpAxis::Z;

    PXR_NS::SdfPathVector targets;
    mesh.GetMeshSourceRel().GetTargets(&targets);
    if (!targets.empty())
    {
        info->meshSource = targets[0].GetAsString();
    }

    mesh.GetSkeletonRel().GetTargets(&targets);
    if (!targets.empty())
    {
        info->skeletonSource = targets[0].GetAsString();
    }

    dest.type = "mesh";
    dest.extraInfo = info;   
    return true;
}

PXR_NS::UsdPrim NauAssetGeomGenerator::generate(PXR_NS::UsdPrim source, PXR_NS::UsdStagePtr stage, const PXR_NS::SdfPath& dest, const nau::MetaArgs& args)
{
    if (PXR_NS::UsdGeomGprim(source))
    {
        auto getVal = [&](const PXR_NS::TfToken& arg, auto& dest)
        {
            if (auto it = args.find(arg); it != args.end() && it->second.CanCast<typename std::remove_reference<decltype(dest)>::type>())
            {
                dest = it->second.Get<typename std::remove_reference<decltype(dest)>::type>();
            }
        };

        auto upAxis = "Y"_tftoken;
        auto unitScale = 1.0f;
        auto ignoreAnimation = false;
        auto generateLods = false;
        auto generateCollider = false;
        auto generateTangents = true;
        auto flipU = false;
        auto flipV = false;

        getVal("upAxis"_tftoken, upAxis);
        getVal("unitScale"_tftoken, unitScale);
        getVal("ignoreAnimation"_tftoken, ignoreAnimation);
        getVal("generateLods"_tftoken, generateLods);
        getVal("generateCollider"_tftoken, generateCollider);
        getVal("generateTangents"_tftoken, generateTangents);
        getVal("flipU"_tftoken, generateTangents);
        getVal("flipV"_tftoken, generateTangents);

        auto meta = PXR_NS::UsdNauAssetMesh::Define(stage, dest);

        PXR_NS::SdfPathVector targets;
        PXR_NS::UsdSkelBindingAPI bindingApi{ source };
        bindingApi.GetSkeletonRel().GetTargets(&targets);
        if (!targets.empty())
        {
            meta.CreateSkeletonRel().SetTargets({ targets[0] });
            meta.CreateSkinnedAttr().Set(true);
        }

        meta.CreateMeshSourceRel().SetTargets({ source.GetPath() });
        meta.CreateGenerateColliderAttr().Set(generateCollider);
        meta.CreateGenerateLodsAttr().Set(generateLods);
        meta.CreateGenerateTangentsAttr().Set(generateTangents);
        meta.CreateIgnoreAnimationAttr().Set(ignoreAnimation);
        meta.CreateUnitScaleAttr().Set(unitScale);
        meta.CreateUpAxisAttr().Set(upAxis);
        meta.CreateFlipUAttr().Set(flipU);
        meta.CreateFlipVAttr().Set(flipV);

        return meta.GetPrim();
    }

    if (auto xformable = PXR_NS::UsdGeomXformable(source))
    {
        auto group = PXR_NS::UsdNauAssetGroup::Define(stage, dest);

        PXR_NS::UsdGeomXformCache cache;
        bool resetsXformStack = false;
        
        auto transform = group.AddTransformOp();
        transform.Set(cache.GetLocalTransformation(source, &resetsXformStack));

        return group.GetPrim();
    }

    return PXR_NS::UsdNauAssetGroup::Define(stage, dest).GetPrim();
  
}

const nau::MetaArgs& NauAssetGeomGenerator::getDefaultArgs() const
{
    static const nau::MetaArgs defaultArgs =
    {
        {"upAxis"_tftoken, PXR_NS::VtValue("Y"_tftoken)},
        {"unitScale"_tftoken, PXR_NS::VtValue(1.0f)},
        {"ignoreAnimation"_tftoken, PXR_NS::VtValue(false)},
        {"generateLods"_tftoken, PXR_NS::VtValue(false)},
        {"generateCollider"_tftoken, PXR_NS::VtValue(false)},
        {"generateTangents"_tftoken, PXR_NS::VtValue(true)},
        {"flipU"_tftoken, PXR_NS::VtValue(false)},
        {"flipV"_tftoken, PXR_NS::VtValue(false)}
    };

    return defaultArgs;

}


