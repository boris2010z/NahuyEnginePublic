// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./scene_loaders.h"
#include "nau/assets/asset_ref.h"
#include "nau/samples/components/camera_control.h"
#include "nau/samples/components/rotator.h"
#include "nau/scene/components/camera_component.h"
#include "nau/scene/components/static_mesh_component.h"
#include "nau/scene/components/billboard_component.h"
#include "nau/scene/scene_factory.h"

namespace nau::sample
{
    scene::IScene::Ptr makeTransformDemoScene(math::vec3 rootPos, bool addCamera)
    {
        using namespace nau::scene;

        auto& factory = getServiceProvider().get<ISceneFactory>();

        IScene::Ptr scene = factory.createEmptyScene();
        scene->setName("tm_demo");

        SceneObject& sceneRoot = scene->getRoot();
        sceneRoot.setTranslation(rootPos);

        auto& meshObject0 = sceneRoot.attachChild(factory.createSceneObject<StaticMeshComponent>());
        meshObject0.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/9]"));
        meshObject0.setTranslation({0, -3, 0});
        meshObject0.setScale({5.f, 0.2f, 5.f});

        auto& meshObject1 = sceneRoot.attachChild(factory.createSceneObject<StaticMeshComponent>());
        meshObject1.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/2]"));
        {
            auto& rotator1 = meshObject1.addComponent<MyRotator>();
            rotator1.setRotationAxis(Axis::Y);
            rotator1.setSpeedFactor(0.25f);
            if (rootPos.getX() < 0.f)
            {
                rotator1.setSpeedFactor(-0.25f);
            }
        }

        auto& meshObject2 = meshObject1.attachChild(factory.createSceneObject<StaticMeshComponent>());
        meshObject2.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/10]"));
        meshObject2.setTranslation({5, 0, 0});

        {
            auto& rotator = meshObject2.addComponent<MyRotator>();
            rotator.setRotationAxis(Axis::X);
            rotator.setSpeedFactor(2.0f);
        }

        auto& meshObject3 = meshObject2.attachChild(factory.createSceneObject<StaticMeshComponent>());
        meshObject3.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/10]"));
        meshObject3.setTranslation({0, 0, -2});
        meshObject3.setScale({0.5f, 0.5f, 0.5f});

        auto& meshObject4 = meshObject2.attachChild(factory.createSceneObject<StaticMeshComponent>());
        meshObject4.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/10]"));
        meshObject4.setTranslation({0, 0, 2});
        meshObject4.setScale({0.5f, 0.5f, 0.5f});
        meshObject4.addComponent<MyRotator>().setRotationAxis(Axis::Z);

        if (addCamera)
        {
            auto& cameraObject = sceneRoot.attachChild(factory.createSceneObject<CameraComponent>());
            cameraObject.setName("Camera.001");
            cameraObject.addComponent<CameraControl>();
            cameraObject.setTranslation({0, 0, -5});
        }

        return scene;
    }


    scene::IScene::Ptr makeInstancingDemoScene(math::vec3 rootPos, bool addCamera)
    {
        using namespace nau::scene;

        auto& factory = getServiceProvider().get<ISceneFactory>();

        IScene::Ptr scene = factory.createEmptyScene();
        scene->setName("inst_demo");

        SceneObject& sceneRoot = scene->getRoot();
        sceneRoot.setTranslation(rootPos);

        constexpr uint32_t instsXDimension = 10;
        constexpr uint32_t instsYDimension = 10;
        constexpr uint32_t instsZDimension = 10;
        constexpr float instStepSize = 5;

        scene::SceneObject::Ptr object = factory.createSceneObject<StaticMeshComponent>();

        for (int i = 0; i < instsXDimension; ++i)
        {
            for (int j = 0; j < instsYDimension; ++j)
            {
                for (int k = 0; k < instsZDimension; ++k)
                {
                    // auto& meshObject0 = sceneRoot.attachChild(factory.createSceneObject<StaticMeshComponent>());
                    auto& meshObject0 = sceneRoot.attachChild(object->clone());
                    meshObject0.getRootComponent<StaticMeshComponent>().setMeshGeometry(StaticMeshAssetRef("file:/content/scenes/scene_demo.gltf+[mesh/2]"));
                    meshObject0.setTranslation({ instStepSize * i, instStepSize * j - 3, instStepSize * k });
                    meshObject0.setScale({ 1.0f, 1.0f, 1.0f });
                }
            }
        }

        if (addCamera)
        {
            auto& cameraObject = sceneRoot.attachChild(factory.createSceneObject<CameraComponent>());
            cameraObject.setName("Camera.001");
            cameraObject.addComponent<CameraControl>();
            cameraObject.setTranslation({0, 0, -5});
        }

        return scene;
    }

    scene::IScene::Ptr makeBillboardsScene(math::vec3 rootPos, bool addCamera)
    {
        using namespace nau::scene;

        auto& factory = getServiceProvider().get<ISceneFactory>();

        IScene::Ptr scene = factory.createEmptyScene();
        scene->setName("billboards_demo");

        SceneObject& sceneRoot = scene->getRoot();
        sceneRoot.setTranslation(rootPos);

        constexpr uint32_t instsXDimension = 3;
        constexpr uint32_t instsYDimension = 3;
        constexpr uint32_t instsZDimension = 3;
        constexpr float instStepSize = 5;
        for (int i = 0; i < instsXDimension; ++i)
        {
            for (int j = 0; j < instsYDimension; ++j)
            {
                for (int k = 0; k < instsZDimension; ++k)
                {
                    auto& billboardObj = sceneRoot.attachChild(factory.createSceneObject<BillboardComponent>());
                    billboardObj.setTranslation({ instStepSize * i, instStepSize * j - 3, instStepSize * k });
                    billboardObj.setScale({ 1.0f, 1.0f, 1.0f });
                    auto& billboard = billboardObj.getRootComponent<BillboardComponent>();
                    billboard.setTextureRef({"file:/content/textures/default.jpg"});
                    billboard.setScreenPercentageSize(0.05f + 0.01f*i);
                }
            }
        }

        if (addCamera)
        {
            auto& cameraObject = sceneRoot.attachChild(factory.createSceneObject<CameraComponent>());
            cameraObject.setName("Camera.001");
            cameraObject.addComponent<CameraControl>();
            cameraObject.setTranslation({0, 0, -5});
        }

        return scene;
    }


}  // namespace nau::sample
