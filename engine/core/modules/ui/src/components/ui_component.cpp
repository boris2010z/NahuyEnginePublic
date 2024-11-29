// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/ui/components/ui_component.h"
#include "EASTL/vector.h"
#include "nau/assets/asset_path.h"
#include "nau/assets/asset_ref.h"
#include "nau/async/task_base.h"
#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/platform/windows/utils/uid.h"
#include "nau/ui/elements/canvas.h"
#include "nau/ui/assets/ui_asset.h"
#include "nau/ui/data/ui_canvas_builder.h"
#include "nau/ui.h"
#include "nau/service/service_provider.h"
#include "nau/scene/scene_manager.h"
#include "nau/assets/asset_manager.h"
#include "nau/scene/scene_factory.h"

namespace nau::ui
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(UiComponent)

    UiComponent::UiComponent() = default;
    UiComponent::~UiComponent()
    {
        if (m_canvas)
        {
            if (m_canvasInScene)
            {
                nau::getServiceProvider().get<nau::ui::UiManager>().removeCanvas(m_canvas->getCanvasName());
                m_canvasInScene = false;
            }

            clearCanvas();
            CC_SAFE_RELEASE_NULL(m_canvas);
        }
    }

    async::Task<> UiComponent::activateComponentAsync()
    {
        const bool paused = getServiceProvider().get<scene::ISceneManager>().getDefaultWorld().isSimulationPaused();
        if (paused)
        {
            co_return;
        }

        const static std::string assetPahtPrefix = "file:/res/";
        std::string fullAssetPath = assetPahtPrefix + m_uiAssetPath;

        {
            auto assetPtr = getServiceProvider().get<IAssetManager>().findAsset({fullAssetPath.c_str()});
            if (assetPtr)
            {
                assetPtr->unload();
                assetPtr->load();
            }
        }

        if (!m_canvas)
        {
            scene::IScene::Ptr engineScene = nau::getServiceProvider().get<nau::scene::ISceneFactory>().createEmptyScene();
            engineScene->setName("UI service scene");
            getServiceProvider().get<ui::UiManager>().setEngineScene(engineScene.getRef());
            m_engineScene = co_await nau::getServiceProvider().get<nau::scene::ISceneManager>().activateScene(std::move(engineScene));

            m_canvas = Canvas::create(toString(getUid()).c_str());
            m_canvas->retain();
            m_canvas->setReferenceSize({static_cast<float>(m_width), static_cast<float>(m_height)});
            m_canvas->setRescalePolicy(RescalePolicy::Stretch);
        }

        clearCanvas();

        if (AssetPath::isValid(fullAssetPath.c_str()))
        {
            co_await UiCanvasBuilder::loadIntoScene(m_canvas, {fullAssetPath.c_str()});
            nau::getServiceProvider().get<nau::ui::UiManager>().addCanvas(m_canvas);
            m_canvasInScene = true;
        }
    }

    void UiComponent::deactivateComponent()
    {
        if (m_canvasInScene)
        {
            nau::getServiceProvider().get<nau::ui::UiManager>().removeCanvas(m_canvas->getCanvasName());
            CC_SAFE_RELEASE_NULL(m_canvas);
            m_canvasInScene = false;
            nau::getServiceProvider().get<nau::scene::ISceneManager>().deactivateScene(m_engineScene);
            m_engineScene = nullptr;
        }
    }

    void UiComponent::clearCanvas()
    {
        NAU_ASSERT(m_canvas);
        eastl::vector<Node*> children;
        m_canvas->getChildren(children);
        for (auto* child : children)
        {
            m_canvas->removeChild(child);
        }
    }
}
