// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "usd_audio_adapter.h"
#include "usd_prim_translator.h"
#include "usd_stage_translator.h"

#include "nau/NauAudioSchema/audioEmitter.h"

#include "nau/service/service_provider.h"
#include "nau/assets/asset_db.h"
#include "nau/shared/file_system.h"
#include "nau/scene/scene_factory.h"

#include "nau/audio/audio_service.hpp"
#include "nau/audio/audio_component_emitter.hpp"
#include "nau/audio/audio_engine.hpp"

#include <filesystem>


namespace UsdTranslator
{
    AudioEmitterAdapter::AudioEmitterAdapter(pxr::UsdPrim prim)
        : IPrimAdapter(prim)
    {
    }

    std::string_view AudioEmitterAdapter::getType() const
    {
        static const std::string name = "AudioEmitterAdapter";
        return name;
    }

    nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> AudioEmitterAdapter::initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest)
    {
        nau::scene::ISceneFactory& sceneFactory = nau::getServiceProvider().get<nau::scene::ISceneFactory>();
        auto child = sceneFactory.createSceneObject<nau::audio::AudioComponentEmitter>();
        m_obj = *child;
        auto result = co_await dest->attachChildAsync(std::move(child));
        co_await update();
        co_return result;
    }

    nau::scene::ObjectWeakRef<nau::scene::SceneObject> AudioEmitterAdapter::getSceneObject() const
    {
        return m_obj;
    }

    nau::async::Task<> AudioEmitterAdapter::update()
    {
        if (!isValid()) return nau::async::Task<>::makeResolved();

        translateWorldTransform(getPrim(), *m_obj);

        auto component = m_obj->findFirstComponent<nau::audio::AudioComponentEmitter>();
        if (component == nullptr) return nau::async::Task<>::makeResolved();

        // Set attributes
        PXR_NS::AudioAudioEmitter emitterData { getPrim() };
    
        // Base attributes
        emitterData.GetLoopAttr().Get(&component->loop);
        emitterData.GetPlayOnStartAttr().Get(&component->playOnStart);
        
        // Asset path
        PXR_NS::SdfAssetPath sdfPathContainer;
        emitterData.GetAudioContainerContainerAttr().Get(&sdfPathContainer);

        if (const eastl::string uidString = sdfPathContainer.GetAssetPath().c_str(); !uidString.empty())
        {
            const auto uid = *nau::Uid::parseString(uidString.substr(4).c_str());
            component->containerAssetUid = uid;
        }

        return nau::async::Task<>::makeResolved();
    }

    bool AudioEmitterAdapter::isValid() const
    {
        return !!m_obj;
    }

    void AudioEmitterAdapter::destroySceneObject()
    {
        if (m_obj) {
            m_obj->destroy();
        }
        m_obj = nullptr;
    }

    [[maybe_unused]] DEFINE_TRANSLATOR(AudioEmitterAdapter, "AudioEmitter"_tftoken);
}