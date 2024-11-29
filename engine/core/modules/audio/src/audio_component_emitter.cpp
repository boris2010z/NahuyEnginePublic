// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/audio/audio_component_emitter.hpp"

#include "nau/service/service_provider.h"
#include "nau/scene/scene_manager.h"
#include "nau/scene/world.h"

#include "nau/audio/audio_service.hpp"
#include "nau/assets/asset_db.h"
#include "nau/io/virtual_file_system.h"

#include "nau/dataBlock/dag_dataBlock.h"

#include <filesystem>


NAU_AUDIO_BEGIN

NAU_IMPLEMENT_DYNAMIC_OBJECT(AudioComponentEmitter)

void AudioComponentEmitter::updateComponent(float dt)
{
    // Temporary solution while we don't have proper play mode
    const auto paused = getServiceProvider().get<scene::ISceneManager>().getDefaultWorld().isSimulationPaused();
    if (paused) {
        if (state != Unloaded) {
            if (source) {
                source->stop();
                NAU_ASSERT(source.use_count() == 1);
                source.reset();
            }
            state = Unloaded;
        }
        return;
    }
    
    if (state == Unloaded) {
        if (playOnStart) {
            if (!container) {
                return;
            }
            source = container->instantiate();
            if (!source) return;
            if (loop) {
                source->setEndCallback([this] {
                    source->stop();
                    source->rewind();
                    source->play();
                });
            }
            source->play();
            state = Playing;
        }
    }
}

void AudioComponentEmitter::activateComponent()
{
    if (!containerAssetUid) {
        return;
    }

    auto& assetDb = nau::getServiceProvider().get<nau::IAssetDB>();
    auto& engine = nau::getServiceProvider().get<nau::audio::AudioService>().engine();
    auto& vfs = nau::getServiceProvider().get<nau::io::IVirtualFileSystem>();

    const auto containerMeta = assetDb.findAssetMetaInfoByUid(containerAssetUid);
    const std::string absolutePath = std::filesystem::path(vfs.resolveToNativePath(containerMeta.dbPath)).string();

    const auto containers = engine.containerAssets();
    const auto itContainer = std::find_if(containers.begin(), containers.end(), [this, absolutePath](nau::audio::AudioAssetContainerPtr container) {
        return std::filesystem::path(container->name().c_str()) == std::filesystem::path(absolutePath);
    });

    if (itContainer != containers.end()) {  // Already loaded into the engine
        container = *itContainer;
    } else {  // Create container
        createContainerFromBlk(absolutePath.c_str());
    }

    NAU_LOG_DEBUG("Audio emmiter component activated");
}

void AudioComponentEmitter::deactivateComponent()
{
    NAU_LOG_DEBUG("Audio emmiter component deactivated");
}

void AudioComponentEmitter::createContainerFromBlk(const eastl::string& path)
{
    if (path.empty()) {
        NAU_LOG_ERROR("Failed to create an audio container from blk: empty path");
        return;
    }

    nau::DataBlock blk;
    if (!blk.load(path.c_str())) {
        NAU_LOG_ERROR("Failed to load audio container at: {}", path);
        return;
    }

    auto& engine = nau::getServiceProvider().get<nau::audio::AudioService>().engine();
    auto& assetDb = nau::getServiceProvider().get<nau::IAssetDB>();
    auto& vfs = nau::getServiceProvider().get<nau::io::IVirtualFileSystem>();

    // Set container kind
    const auto kindStr = blk.getStr("kind");
    const auto kind = nau::EnumTraits<nau::audio::AudioContainerKind>().parse(kindStr);
    auto container = engine.createContainer(path);
    container->setKind(*kind);
    
    // Add sources
    auto sourcesBlk = blk.getBlockByName("sources");
    for (int blockIndex = 0; blockIndex < sourcesBlk->blockCount(); blockIndex++)
    {
        // Resolve asset path
        auto sourceBlk = sourcesBlk->getBlock(blockIndex);
        auto sourceUid = sourceBlk->getStr("uid");
        const auto soundMeta = assetDb.findAssetMetaInfoByUid(*nau::Uid::parseString(sourceUid));
        const std::string absolutePath = std::filesystem::path(vfs.resolveToNativePath(soundMeta.dbPath)).string();

        // Check if asset already exists in the engine
        const auto assets = engine.audioAssets();
        const auto itAsset = std::find_if(assets.begin(), assets.end(), [this, absolutePath](nau::audio::AudioAssetPtr asset) {
            return std::filesystem::path(asset->name().c_str()) == std::filesystem::path(absolutePath);
        });

        if (itAsset != assets.end()) {  // Already loaded into the engine
            container->add(*itAsset);
        } else {  // Loading this particular asset for the first time
            auto sound = engine.loadSound(absolutePath.c_str());
            container->add(sound);
        }
    }
}

NAU_AUDIO_END