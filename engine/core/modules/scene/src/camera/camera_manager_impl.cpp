// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "camera/camera_manager_impl.h"

#include "camera/detached_camera.h"
#include "camera/readonly_camera.h"
#include "nau/memory/eastl_aliases.h"
#include "nau/scene/components/camera_component.h"
#include "nau/scene/scene_manager.h"
#include "nau/scene/world.h"

namespace nau::scene
{
    async::Task<> CameraManagerImpl::preInitService()
    {
        m_syncThreadId = std::this_thread::get_id();
        return async::Task<>::makeResolved();
    }

    async::Task<> CameraManagerImpl::initService()
    {
        m_checkCamerasLastTime = std::chrono::system_clock::now();
        return async::Task<>::makeResolved();
    }

    void CameraManagerImpl::cleanupActiveCameras(const eastl::unordered_set<Uid>& allCameraUids)
    {
        eastl::erase_if(m_worldMainCameras, [&allCameraUids](const auto& keyValue)
        {
            return !allCameraUids.contains(keyValue.second);
        });
    }

    ICameraManager::CamerasSnapshot CameraManagerImpl::getCameras()
    {
        NAU_FATAL(m_syncThreadId == std::this_thread::get_id(), "Camera synchronization can be performed only from main thread");
        lock_(m_mutex);

        eastl::unordered_set<Uid> allCameraUids;

        CamerasSnapshot snapshot;

        m_detachedCameras.remove_if([&snapshot, &allCameraUids](nau::WeakPtr<ICameraControl>& cameraWeakPtr) -> bool
        {
            nau::Ptr<ICameraControl> camera = cameraWeakPtr.lock();
            if (camera)
            {
                snapshot.m_worldCameras[camera->getWorldUid()].emplace_back(rtti::createInstance<ReadonlyCamera>(camera));
                allCameraUids.emplace(camera->getCameraUid());
                return false;
            }

            return true;  // remove
        });

        m_sceneCameras.remove_if([&snapshot, &allCameraUids](ObjectWeakRef<ICameraControl>& sceneCameraRef) -> bool
        {
            if (sceneCameraRef)
            {
                snapshot.m_worldCameras[sceneCameraRef->getWorldUid()].emplace_back(rtti::createInstance<ReadonlyCamera>(sceneCameraRef));
                allCameraUids.emplace(sceneCameraRef->getCameraUid());
                return false;
            }

            return true;  // remove
        });

        cleanupActiveCameras(allCameraUids);

        snapshot.m_worldMainCameras = m_worldMainCameras;

        return snapshot;
    }

    void CameraManagerImpl::syncCameras(CamerasSnapshot& snapshot, SyncCameraCallback onCameraAdded, SyncCameraCallback onCameraRemoved)
    {
        NAU_FATAL(m_syncThreadId == std::this_thread::get_id(), "Camera synchronization can be performed only from main thread");
        lock_(m_mutex);

        // sync existing camera properties and remove non existent
        for (auto& [worldUid, cameras] : snapshot.m_worldCameras)
        {
            eastl::erase_if(cameras, [&onCameraRemoved](nau::Ptr<ICameraProperties>& camera) -> bool
            {
                NAU_FATAL(camera);
                const bool syncOk = camera->as<ReadonlyCamera&>().syncCameraProperties();
                if (!syncOk && onCameraRemoved)
                {
                    onCameraRemoved(*camera);
                }

                return !syncOk;
            });
        }

        const auto containsCamera = [&snapshot](const ICameraProperties& cam) -> bool
        {
            auto& cameras = snapshot.m_worldCameras[cam.getWorldUid()];

            return eastl::any_of(cameras.begin(), cameras.end(), [cameraUid = cam.getCameraUid()](const auto& camera)
            {
                return camera->getCameraUid() == cameraUid;
            });
        };

        eastl::unordered_set<Uid> allCameraUids;

        // iterate over detached cameras and add that not exists in cameras collection:
        m_detachedCameras.remove_if([&](nau::WeakPtr<ICameraControl>& cameraWeakPtr) -> bool
        {
            nau::Ptr<ICameraControl> camera = cameraWeakPtr.lock();
            if (!camera)
            {
                return true;
            }

            allCameraUids.emplace(camera->getCameraUid());

            if (!containsCamera(*camera))
            {
                auto& worldCameras = snapshot.m_worldCameras[camera->getWorldUid()];
                auto& newCamera = worldCameras.emplace_back(rtti::createInstance<ReadonlyCamera>(camera));
                if (onCameraAdded)
                {
                    onCameraAdded(*newCamera);
                }
            }

            return false;
        });

        // iterate over scene cameras and add that not exists in cameras collection:
        m_sceneCameras.remove_if([&](ObjectWeakRef<ICameraControl>& sceneCameraRef) -> bool
        {
            if (!sceneCameraRef)
            {
                return true;
            }

            allCameraUids.emplace(sceneCameraRef->getCameraUid());

            if (!containsCamera(*sceneCameraRef))
            {
                auto& worldCameras = snapshot.m_worldCameras[sceneCameraRef->getWorldUid()];
                auto& newCamera = worldCameras.emplace_back(rtti::createInstance<ReadonlyCamera>(sceneCameraRef));
                allCameraUids.emplace(sceneCameraRef->getCameraUid());

                if (onCameraAdded)
                {
                    onCameraAdded(*newCamera);
                }
            }

            return false;
        });

        cleanupActiveCameras(allCameraUids);

        // clean up worlds that does not contains any camera.
        for (auto iter = snapshot.m_worldCameras.begin(); iter != snapshot.m_worldCameras.end();)
        {
            if (iter->second.empty())
            {
                iter = snapshot.m_worldCameras.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        // update worlds main cameras
        for (const auto& [worldUid, cameraUid] : m_worldMainCameras)
        {
            snapshot.m_worldMainCameras[worldUid] = cameraUid;
        }

        for (auto iter = snapshot.m_worldMainCameras.begin(); iter != snapshot.m_worldMainCameras.end();)
        {
            if (!m_worldMainCameras.contains(iter->first))
            {
                iter = snapshot.m_worldMainCameras.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    nau::Ptr<ICameraControl> CameraManagerImpl::createDetachedCamera(Uid worldUid)
    {
        lock_(m_mutex);

        if (worldUid == NullUid)
        {
            // getting Uid for the default world is thread safe operation
            worldUid = getServiceProvider().get<ISceneManager>().getDefaultWorld().getUid();
        }

        auto camera = rtti::createInstance<DetachedCamera>(worldUid);
        m_detachedCameras.emplace_back(camera);
        return camera;
    }

    Uid CameraManagerImpl::setMainCamera(Uid cameraUid)
    {
        if (cameraUid == NullUid)
        {
            NAU_LOG_WARNING("Call setMainCamera({NullUid}) will do nothing");
            return NullUid;
        }

        lock_(m_mutex);

        ICameraControl* camera = nullptr;

        {
            auto iter = std::find_if(m_sceneCameras.begin(), m_sceneCameras.end(), [&](const ObjectWeakRef<ICameraControl>& cam)
            {
                return cam && cam->getCameraUid() == cameraUid;
            });

            if (iter != m_sceneCameras.end())
            {
                camera = (*iter).get();
            }
        }

        // need to keep camera instance while it used.
        Ptr<ICameraControl> detachedCamera;

        if (camera == nullptr)
        {
            for (WeakPtr<ICameraControl>& camRef : m_detachedCameras)
            {
                if (Ptr<ICameraControl> cameraPtr = camRef.lock(); cameraPtr && cameraPtr->getCameraUid() == cameraUid)
                {
                    detachedCamera = std::move(cameraPtr);
                    camera = detachedCamera.get();
                    break;
                }
            }
        }

        if (!camera)
        {
            NAU_LOG_ERROR("Camera ({}) not found", toString(cameraUid));
            return NullUid;
        }

        m_worldMainCameras[camera->getWorldUid()] = camera->getCameraUid();
        return camera->getWorldUid();
    }

    void CameraManagerImpl::resetWorldMainCamera(Uid worldUid)
    {
        if (worldUid == NullUid && hasServiceProvider())
        {
            worldUid = getServiceProvider().get<ISceneManager>().getDefaultWorld().getUid();
        }

        lock_(m_mutex);
        m_worldMainCameras.erase(worldUid);
    }

    Result<> CameraManagerImpl::activateComponents([[maybe_unused]] Uid worldUid, eastl::span<Component*> components)
    {
        NAU_ASSERT(m_syncThreadId == std::this_thread::get_id());

        for (Component* const component : components)
        {
            CameraComponent* const cameraComponent = component->as<CameraComponent*>();
            if (!cameraComponent)
            {
                continue;
            }

            lock_(m_mutex);
            m_sceneCameras.emplace_back(*cameraComponent);
        }

        return ResultSuccess;
    }

    void CameraManagerImpl::deactivateComponents([[maybe_unused]] Uid worldUid, eastl::span<Component*> components)
    {
        NAU_ASSERT(m_syncThreadId == std::this_thread::get_id());
    }

    void CameraManagerImpl::gamePreUpdate([[maybe_unused]] std::chrono::milliseconds dt)
    {
        checkCameras();
    }

    void CameraManagerImpl::checkCameras()
    {
        using namespace std::chrono;

        constexpr auto CheckCameraTimeout = milliseconds(2000);
        constexpr auto MissingCameraLogLevel = diag::LogLevel::Critical;

        {
            const auto currentTime = system_clock::now();
            if (duration_cast<milliseconds>(currentTime - m_checkCamerasLastTime) < CheckCameraTimeout)
            {
                return;
            }

            m_checkCamerasLastTime = currentTime;
        }

        eastl::unordered_set<Uid> worldsWithCameras;

        m_detachedCameras.remove_if([&worldsWithCameras](nau::WeakPtr<ICameraControl>& cameraWeakPtr) -> bool
        {
            if (nau::Ptr<ICameraControl> camera = cameraWeakPtr.lock(); camera)
            {
                worldsWithCameras.emplace(camera->getWorldUid());
                return false;
            }

            return true;  // remove
        });

        m_sceneCameras.remove_if([&worldsWithCameras](ObjectWeakRef<ICameraControl>& sceneCameraRef) -> bool
        {
            if (sceneCameraRef)
            {
                worldsWithCameras.emplace(sceneCameraRef->getWorldUid());
                return false;
            }

            return true;  // remove
        });

        auto& sceneManger = getServiceProvider().get<ISceneManager>();
        auto worlds = sceneManger.getWorlds();

        for (IWorld::WeakRef& world : worlds)
        {
            if (world->getScenes().empty())
            {
                continue;
            }

            if (!worldsWithCameras.contains(world->getUid()))
            {
                NAU_LOG_MESSAGE(MissingCameraLogLevel)
                ("The world ({}) does not have any camera", toString(world->getUid()));
            }
        }
    }

    Ptr<ICameraProperties> ICameraManager::CamerasSnapshot::getCameraByUid(Uid cameraUid)
    {
        for (const auto& [worldUid, cameras] : m_worldCameras)
        {
            auto camIter = eastl::find_if(cameras.begin(), cameras.end(), [cameraUid](const Ptr<ICameraProperties>& cam)
            {
                return cam->getCameraUid() == cameraUid;
            });

            if (camIter != cameras.end())
            {
                NAU_ASSERT(worldUid == (*camIter)->getWorldUid());
                return *camIter;
            }
        }

        return nullptr;
    }

    eastl::span<Ptr<ICameraProperties>> ICameraManager::CamerasSnapshot::getWorldAllCameras(Uid worldUid)
    {
        if (worldUid == NullUid && hasServiceProvider())
        {
            worldUid = getServiceProvider().get<ISceneManager>().getDefaultWorld().getUid();
        }

        if (auto iter = m_worldCameras.find(worldUid); iter != m_worldCameras.end())
        {
            return iter->second;
        }

        return {};
    }

    Ptr<ICameraProperties> ICameraManager::CamerasSnapshot::getWorldMainCamera(Uid worldUid)
    {
        if (worldUid == NullUid && hasServiceProvider())
        {
            worldUid = getServiceProvider().get<ISceneManager>().getDefaultWorld().getUid();
        }

        CameraCollection* const worldCameras = EXPR_Block->CameraCollection*
        {
            auto camIter = m_worldCameras.find(worldUid);
            if (camIter == m_worldCameras.end() || camIter->second.empty())
            {
                return nullptr;
            }

            return &camIter->second;
        };

        // there are no cameras at all for the specified world
        if (!worldCameras)
        {
            return nullptr;
        }

        size_t cameraIndex = 0;

        auto mainCamIter = m_worldMainCameras.find(worldUid);
        if (mainCamIter != m_worldMainCameras.end() && mainCamIter->second != NullUid)
        {
            const Uid& mainCameraUid = mainCamIter->second;
            auto iter = eastl::find_if(worldCameras->begin(), worldCameras->end(), [&mainCameraUid](const Ptr<ICameraProperties>& cam)
            {
                return cam->getCameraUid() == mainCameraUid;
            });

            if (iter == worldCameras->end())
            {
                NAU_LOG_WARNING("Main camera uid ({}) is specified, but camera data not found", toString(mainCameraUid));
                m_worldMainCameras.erase(mainCamIter);
            }
            else
            {
                cameraIndex = static_cast<size_t>(eastl::distance(worldCameras->begin(), iter));
            }
        }

        NAU_FATAL(cameraIndex < worldCameras->size());
        return worldCameras->at(cameraIndex);
    }

    Ptr<ICameraProperties> ICameraManager::CamerasSnapshot::getWorldExplicitMainCamera(Uid worldUid)
    {
        if (worldUid == NullUid && hasServiceProvider())
        {
            worldUid = getServiceProvider().get<ISceneManager>().getDefaultWorld().getUid();
        }

        CameraCollection* const worldCameras = EXPR_Block->CameraCollection*
        {
            auto camIter = m_worldCameras.find(worldUid);
            if (camIter == m_worldCameras.end() || camIter->second.empty())
            {
                return nullptr;
            }

            return &camIter->second;
        };

        // there are no cameras at all for the specified world
        if (!worldCameras)
        {
            return nullptr;
        }

        auto mainCamIter = m_worldMainCameras.find(worldUid);
        if (mainCamIter == m_worldMainCameras.end() || mainCamIter->second == NullUid)
        {
            return nullptr;
        }

        const Uid& mainCameraUid = mainCamIter->second;
        auto iter = eastl::find_if(worldCameras->begin(), worldCameras->end(), [&mainCameraUid](const Ptr<ICameraProperties>& cam)
        {
            return cam->getCameraUid() == mainCameraUid;
        });

        if (iter == worldCameras->end())
        {
            NAU_LOG_WARNING("Main camera uid ({}) is specified, but camera data not found", toString(mainCameraUid));
            m_worldMainCameras.erase(mainCamIter);
            return nullptr;
        }

        const size_t cameraIndex = static_cast<size_t>(eastl::distance(worldCameras->begin(), iter));
        NAU_FATAL(cameraIndex < worldCameras->size());
        return worldCameras->at(cameraIndex);
    }

    bool ICameraManager::CamerasSnapshot::isEmpty() const
    {
        if (m_worldCameras.empty())
        {
            return true;
        }

        return eastl::all_of(m_worldCameras.begin(), m_worldCameras.end(), [](const auto& entry)
        {
            return entry.second.empty();
        });
    }
}  // namespace nau::scene
