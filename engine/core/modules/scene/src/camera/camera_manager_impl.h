// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/app/main_loop/game_system.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/camera/camera_manager.h"
#include "nau/scene/scene_processor.h"
#include "nau/service/service.h"

namespace nau::scene
{
    class CameraManagerImpl final : public ICameraManager,
                                    public IComponentsActivator,
                                    public IServiceInitialization,
                                    public IGamePreUpdate
    {
        NAU_RTTI_CLASS(nau::scene::CameraManagerImpl, ICameraManager, IComponentsActivator, IServiceInitialization, IGamePreUpdate)

    private:
        void cleanupActiveCameras(const eastl::unordered_set<Uid>&);

        async::Task<> preInitService() override;
        async::Task<> initService() override;

        CamerasSnapshot getCameras() override;
        void syncCameras(CamerasSnapshot& cameras, SyncCameraCallback onCameraAdded, SyncCameraCallback onCameraRemoved) override;
        nau::Ptr<ICameraControl> createDetachedCamera(Uid worldUid) override;
        Uid setMainCamera(Uid cameraUid) override;
        void resetWorldMainCamera(Uid worldUid) override;
        Result<> activateComponents(Uid worldUid, eastl::span<Component*> components) override;
        void deactivateComponents(Uid worldUid, eastl::span<Component*> components) override;
        void gamePreUpdate(std::chrono::milliseconds dt) override;

        void checkCameras();

        mutable std::mutex m_mutex;
        std::thread::id m_syncThreadId;
        eastl::list<ObjectWeakRef<ICameraControl>> m_sceneCameras;
        eastl::list<nau::WeakPtr<ICameraControl>> m_detachedCameras;
        eastl::unordered_map<Uid, Uid> m_worldMainCameras;

        std::chrono::system_clock::time_point m_checkCamerasLastTime;
    };
}  // namespace nau::scene
