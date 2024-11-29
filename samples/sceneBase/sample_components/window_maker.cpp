// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./window_maker.h"

#include <nau/scene/scene_object.h>

#include "nau/animation/animation_manager.h"
#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/animation/components/skeleton_socket_component.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/animation/playback/animation_scalars.h"
#include "nau/animation/playback/animation_transforms.h"
#include "nau/app/core_window_manager.h"
#include "nau/graphics/core_graphics.h"
#include "nau/platform/windows/app/windows_window.h"
#include "nau/render/render_window.h"
#include "nau/samples/components/camera_control.h"
#include "nau/scene/components/directional_light_component.h"
#include "nau/scene/components/environment_component.h"
#include "scene_reloader.h"


namespace nau::sample
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(WindowMaker)
    namespace
    {
        struct CustomHeightAnimTarget : animation::ScalarParameterAnimatable
        {
            NAU_CLASS(CustomHeightAnimTarget, rtti::RCPolicy::StrictSingleThread, animation::ScalarParameterAnimatable)

            CustomHeightAnimTarget(scene::SceneObject* obj) :
                objectToAnimate(*obj)
            {
                rootPos = obj->getTranslation();
            }

            virtual void animateFloat(float value) override
            {
                if (auto* object = objectToAnimate.get())
                {
                    auto newPos = rootPos + math::vec3(.0f, value, .0f);
                    object->setTranslation(newPos);
                }
            }

            scene::ObjectWeakRef<scene::SceneObject> objectToAnimate;
            math::vec3 rootPos;
        };
        async::Task<scene::IWorld::WeakRef> loadGltfWorld(const eastl::string& sceneAssetPath, bool setupCamera)
        {
            using namespace nau::scene;

            ASYNC_SWITCH_EXECUTOR(getApplication().getExecutor());

            AssetRef<> sceneAssetRef = AssetPath{sceneAssetPath};

            SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();

            IScene::Ptr scene = getServiceProvider().get<scene::ISceneFactory>().createSceneFromAsset(*sceneAsset);

            bool isScalarAnimAttached = false;

#ifdef USE_PHYSICS
            auto& physWorld = getServiceProvider().get<physics::IPhysicsWorld>();
            auto& physShapesFactory = getServiceProvider().get<physics::ICollisionShapesFactory>();
#endif

#ifdef USE_ANIMATION
            // todo: Must get rid of this: move to scene service
            scene->getRoot().addComponent<animation::AnimationManager>();
#endif

            for (SceneObject* const obj : scene->getRoot().getChildObjects(true))
            {
                if (auto* const staticMesh = obj->findFirstComponent<StaticMeshComponent>(); staticMesh)
                {
                    if (obj->getName().find("Teapot", 0) == 0)
                    {
                        using namespace animation;
                        using namespace math;

                        auto& animComp = obj->addComponent<animation::AnimationComponent>();

                        if (!isScalarAnimAttached)
                        {
                            isScalarAnimAttached = true;

                            auto animation = rtti::createInstance<FloatAnimation>();

                            if (auto animationEditor = animation->createEditor())
                            {
                                animationEditor.addKeyFrame(0, .0f);
                                animationEditor.addKeyFrame(50, 12.f);

                                animationEditor.addFrameEvent(3, FrameEvent("one-time"));
                                animationEditor.addFrameEvent(10, FrameEvent("long-lasting", FrameEventType::Start));
                                animationEditor.addFrameEvent(20, FrameEvent("long-lasting", FrameEventType::Stop));

                                DataBlock block;
                                animationEditor.serialize(block);
                                animationEditor.deserialize(block);
                            }

                            auto animInstance = rtti::createInstance<AnimationInstance>("jump", animation);
                            animInstance->setPlayMode(PlayMode::PingPong);
                            animInstance->setInterpolationMethod(animation::AnimationInterpolationMethod::Step);
                            animComp.addAnimation(animInstance);

                            IAnimatable::Ptr heightSetTarget = rtti::createInstance<CustomHeightAnimTarget>(obj);

                            animComp.addCustomAnimationTarget(std::move(heightSetTarget), animInstance->getPlayer());
                        }
                        else
                        {
                            auto animation = rtti::createInstance<TransformAnimation>();

                            if (auto animationEditor = animation->createEditor())
                            {
                                animationEditor.addKeyFrame(0, Transform::identity());
                                animationEditor.addKeyFrame(100, Transform(quat::identity(), vec3(10.f, .0f, 10.0f), vec3(1.f, 1.f, 1.f)));
                                animationEditor.addKeyFrame(200, Transform(quat::identity(), vec3(.0f, .0f, 20.0f), vec3(1.f, 1.f, 1.f)));
                                animationEditor.addKeyFrame(300, Transform(quat::identity(), vec3(-10.f, .0f, 10.0f), vec3(1.f, 1.f, 1.f)));
                                animationEditor.addKeyFrame(400, Transform::identity());
                            }

                            auto animInstance = rtti::createInstance<AnimationInstance>("move", animation);
                            animInstance->setPlayMode(PlayMode::Looping);
                            animComp.addAnimation(animInstance);
                        }
                    }
                    else if (obj->getName().find("Torus", 0) == 0)
                    {
                        using namespace animation;
                        using namespace math;

                        auto& animComp = obj->addComponent<animation::AnimationComponent>();

                        auto animation = rtti::createInstance<TransformAnimation>();

                        if (auto animationEditor = animation->createEditor())
                        {
                            animationEditor.addKeyFrame(0, Transform::identity());
                            animationEditor.addKeyFrame(100, Transform(quat::identity(), vec3(.0f, 3.0f, .0f), vec3(1.f, 1.f, 1.f)));
                        }

                        auto animInstance = rtti::createInstance<AnimationInstance>("up-down", animation);
                        animInstance->setPlayMode(PlayMode::PingPong);
                        animComp.addAnimation(animInstance);
                    }
                }

                if (auto* const skeletonComponent = obj->findFirstComponent<SkeletonComponent>(); skeletonComponent)
                {
                    // example for adding skeleton socket from code
                    auto& sceneFactory = getServiceProvider().get<ISceneFactory>();
                    scene::ObjectUniquePtr<SceneObject> skeletonSocket = sceneFactory.createSceneObject<SkeletonSocketComponent>();
                    skeletonSocket->setName("SkeletonSocket_HEAD");

                    auto& skeletonSocketComponent = skeletonSocket->getRootComponent<SkeletonSocketComponent>();
                    skeletonSocketComponent.setBoneName("head.R");
                    skeletonSocketComponent.setRelativeToBoneOffset(math::Transform(math::quat::identity(), math::Vector3(0, 0.2, 0)));

                    obj->attachChild(std::move(skeletonSocket));
                }
            }

            auto& sceneManager = getServiceProvider().get<scene::ISceneManager>();

            auto world = sceneManager.createWorld();

            auto& sceneFactory = getServiceProvider().get<scene::ISceneFactory>();

            auto scene_00 = sceneFactory.createEmptyScene();

            scene_00->setName("scene_00");

            ObjectWeakRef<SceneObject> cameraObject;

            cameraObject = scene_00->getRoot().attachChild(sceneFactory.createSceneObject<scene::CameraComponent>());

            cameraObject->setName("Camera.Main");
            cameraObject->addComponent<CameraControl>().setCamControlKind(CamControlKind::UseSceneObject);

            auto activatedScene1 = co_await world->addScene(std::move(scene_00));
            auto activatedScene2 = co_await world->addScene(std::move(scene));

            co_return world;
        }
    }  // namespace

    void WindowMaker::updateComponent(float dt)
    {
        if (m_task)
        {
            if (m_task.isReady())
            {
                m_rendWindows.push_back(m_task.result());
                m_task = {};
            }
        }

        if (m_sceneLoaderTask)
        {
            if (m_sceneLoaderTask.isReady())
            {
                m_worlds = m_sceneLoaderTask.result();
                m_sceneLoaderTask = {};
            }
        }
        else
        {
            if (!m_worlds.size())
            {
                m_sceneLoaderTask = [this]() -> async::Task<eastl::vector<scene::IWorld::WeakRef>>
                {
                    using namespace nau::async;
                    using namespace nau::scene;
                    eastl::vector<scene::IWorld::WeakRef> worlds;

                    worlds.push_back(co_await loadGltfWorld("file:/content/scenes/scene_demo.gltf", true));
                    worlds.push_back(co_await loadGltfWorld("file:/content/scenes/damaged_helmet/DamagedHelmet.gltf", false));
                    worlds.push_back(co_await loadGltfWorld("file:/content/scenes/damaged_helmet_translucent/DamagedHelmet_Translucent.gltf", false));
                    worlds.push_back(co_await loadGltfWorld("file:/content/scenes/robot/robot_skeletal_pbr.gltf", false));

                    co_return worlds;
                }();
            }
        }

        if (m_rendWindows.size() && m_worlds.size())
        {
            static int shift = 0;
            shift++;
            for (int i = 0; i < m_rendWindows.size(); ++i)
            {
                m_rendWindows[i].acquire()->setWorld(m_worlds[(i + shift/100000) % m_worlds.size()]->getUid());
            }
        }

        ImGui::Begin("Demo");
        ImGui::Text("Window sys");
        if (ImGui::Button("New window"))
        {
            auto* const windowManager = getServiceProvider().find<ICoreWindowManager>();
            if (windowManager)
            {
                auto window = windowManager->createWindow(false);
                window->setVisible(true);

                auto* windowsWindow = window->as<IWindowsWindow*>();

                HWND hwnd = windowsWindow->getWindowHandle();

                auto* const coreGraphics = getServiceProvider().find<ICoreGraphics>();
                m_task = coreGraphics->createRenderWindow(hwnd);

                m_windows.emplace_back(std::move(window));
            }
            else
            {
                NAU_LOG_WARNING("No ICoreWindowManager service found");
            }
        }
        ImGui::End();

        auto envs = this->getParentObject().getDirectComponents<scene::EnvironmentComponent>();
        auto lights = this->getParentObject().getDirectComponents<scene::DirectionalLightComponent>();
        if (!lights.empty())
        {
            scene::DirectionalLightComponent* lightComponent = lights[0]->as<scene::DirectionalLightComponent*>();
            ImGui::Begin("DirectionalLightControl");

            static float intensity = 1;
            ImGui::SliderFloat("Intensity", &intensity, 0, 10);
            lightComponent->setIntensity(intensity);

            static float direction[3]= {0.5f, -0.5f, 0.0f};
            ImGui::SliderFloat3("Direction", direction, -1, 1);
            auto vDirection = math::Vector3(direction[0], direction[1], direction[2]);
            if (!vDirection.similar(math::Vector3(0, 0, 0)))
            {
                using namespace nau::math;
                Matrix4 mat = Matrix4::lookAtRH(Point3(), Point3(normalize(-vDirection)), Vector3(0.0f, 1.0f, 0.0f));
                Quat rot = Quat(mat.getUpper3x3());
                lightComponent->setRotation(rot);
            }

            static float powWeigth = 0.985f;
            ImGui::SliderFloat("PowWeigth", &powWeigth, 0.0f, 1.0f);
            lightComponent->setCsmPowWeight(powWeigth);

            static int32_t cascadesCount = 4;
            ImGui::SliderInt("CascadesCount", &cascadesCount, 0, 4);
            lightComponent->setShadowCascadeCount(cascadesCount);

            static int32_t csmSize = 256;
            ImGui::SliderInt("CSM Size", &csmSize, 256, 2048);
            lightComponent->setShadowMapSize(csmSize);

            static bool hasShadows = false;
            ImGui::Checkbox("HasShadows", &hasShadows);
            lightComponent->setCastShadows(hasShadows);

            if (!envs.empty())
            {
                auto* env = envs[0]->as<scene::EnvironmentComponent*>();

                static float envIntensity = 1.0f;
                ImGui::SliderFloat("envIntensity", &envIntensity, 0.0f, 1.0f);
                env->setIntensity(envIntensity);

                if (ImGui::Button("Update Texture"))
                {
                    TextureAssetRef panoramaTex = TextureAssetRef("file:/content/textures/environment/default_cubemap_4k.hdr");
                    env->setTextureAsset(panoramaTex);
                }
            }

            static float color[3] = {1,1,1};
            ImGui::ColorPicker3("LightColor", color);
            lightComponent->setColor({color[0], color[1], color[2]});

            ImGui::End();
        }
    }
}  // namespace nau::sample
