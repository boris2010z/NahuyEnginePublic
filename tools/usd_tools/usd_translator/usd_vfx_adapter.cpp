// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_vfx_adapter.h"

#include <nau/scene/scene_factory.h>
#include <nau/service/service_provider.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCache.h>

#include "nau/assets/asset_descriptor_factory.h"
#include "usd_proxy/usd_proxy_decorators_regestry.h"

#include "../src/components/vfx_component.h"

#include "nau/asset_tools/db_manager.h"
#include "nau/io/virtual_file_system.h"
#include <filesystem>
#include "nau/asset_tools/asset_info.h"

using namespace nau;
using namespace nau::scene;
using namespace PXR_NS;
namespace fs = std::filesystem;

namespace UsdTranslator
{
    namespace
    {
        constexpr std::string_view g_typeName = "NauAssetVFX";

        fs::path dbPath()
        {
            auto& vfs = getServiceProvider().get<io::IVirtualFileSystem>();
            auto projectPath = fs::path(vfs.resolveToNativePath("/content")).parent_path();

            return fs::path(projectPath) / getAssetsDBfolderName();
        }

        nau::Result<AssetMetaInfo> getAssetInfo(Uid uid)
        {
            auto& dbManager = AssetDatabaseManager::instance();
            auto& vfs = getServiceProvider().get<io::IVirtualFileSystem>();
            auto projectPath = fs::path(vfs.resolveToNativePath("/content")).parent_path();
            const fs::path assetsDb = fs::path(projectPath) / getAssetsDBfolderName();
            int g = 0;

            if (!dbManager.isLoaded())
            {
                const fs::path dbFilePath = assetsDb / getAssetsDbName();
                NAU_VERIFY(dbManager.load(assetsDb.string()), "Failed to load assets database!");
                auto contentFs = io::createNativeFileSystem(assetsDb.string());
                vfs.mount(getAssetsDBfolderName(), contentFs);
            }

            return dbManager.get(uid);
        }
    }

    VFXAdapter::VFXAdapter(PXR_NS::UsdPrim prim) :
        IPrimAdapter(prim)
    {
    }

    VFXAdapter::~VFXAdapter()
    {
    }

    bool VFXAdapter::isValid() const
    {
        return !!m_obj;
    }

    nau::async::Task<> VFXAdapter::update()
    {
        if (!m_obj)
            co_return;

        translateWorldTransform(getPrim(), *m_obj);

        auto prim = getPrim();
        if (!prim)
            co_return;

        std::string uidStr;
        if (!prim.GetAttribute("uid"_tftoken).Get(&uidStr))
            co_return;

        auto uid = Uid::parseString(uidStr.c_str());
        if (!uid)
            co_return;

        auto metaInfo = getAssetInfo(*uid);
        if (!metaInfo)
            co_return;

        auto& vfxComponent = m_obj->getRootComponent<nau::vfx::VFXComponent>();

        if (m_vfxTimeStamp == metaInfo->lastModified)
        {
            vfxComponent.forcePositionUpdate();
        }
        else
        {
            vfxComponent.forceBLKUpdate();
            m_vfxTimeStamp = metaInfo->lastModified;
        }
    }

    std::string_view VFXAdapter::getType() const
    {
        return g_typeName;
    }

    nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> VFXAdapter::initializeSceneObject(
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest)
    {
        auto prim = getPrim();
        if (!prim)
            co_return nullptr;

        std::string uidStr;
        if (!prim.GetAttribute("uid"_tftoken).Get(&uidStr))
            co_return nullptr;
        auto uid = Uid::parseString(uidStr.c_str());
        if (!uid)
            co_return nullptr;

        auto metaInfo = getAssetInfo(*uid);
        if (!metaInfo)
            co_return nullptr;

        m_vfxTimeStamp = metaInfo->lastModified;

        auto& sceneFactory = getServiceProvider().get<scene::ISceneFactory>();
        auto component = sceneFactory.createSceneObject<nau::vfx::VFXComponent>();
        component->setName(prim.GetName().GetText());
        auto& vfxComponent = component->getRootComponent<nau::vfx::VFXComponent>();
        m_path = eastl::string((dbPath().string() + "\\").c_str() + metaInfo->dbPath);

        vfxComponent.setAssetPath(metaInfo->dbPath);

        m_obj = *component;

        co_await update();
        co_return co_await dest->attachChildAsync(std::move(component));
    }

    nau::scene::ObjectWeakRef<nau::scene::SceneObject> VFXAdapter::getSceneObject() const
    {
        return m_obj;
    }

    void VFXAdapter::destroySceneObject()
    {
        if (m_obj)
            m_obj->destroy();
        m_obj = nullptr;
    }

    DEFINE_TRANSLATOR(VFXAdapter, "NauAssetVFX"_tftoken);
}  // namespace UsdTranslator
