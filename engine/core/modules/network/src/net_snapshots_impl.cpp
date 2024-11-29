// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "net_snapshots_impl.h"

#include "nau/diag/logging.h"
#include "nau/network/napi/networking_factory.h"
#include "nau/network/netsync/net_connector.h"
#include "nau/serialization/json.h"
#include "nau/serialization/json_utils.h"
#include "nau/service/service_provider.h"

namespace nau
{
    async::Task<> NetSnapshotsImpl::preInitService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> NetSnapshotsImpl::initService()
    {
        m_frame = 0;
        return async::Task<>::makeResolved();
    }

    void NetSnapshotsImpl::onSceneActivated(IComponentNetScene* scene)
    {
        const char* peerId = scene->getPeerId().data();
        const char* sceneName = scene->getSceneName().data();
        if (m_peers.count(peerId) == 0)
        {
            m_peers.emplace(peerId, PeerData());
        }
        PeerData& pdata = m_peers[peerId];
        pdata.activateScene(scene);
        m_sceneToPeer.emplace(sceneName, &pdata);
    }

    void NetSnapshotsImpl::onSceneDectivated(IComponentNetScene* scene)
    {
        // TODO
    }

    void NetSnapshotsImpl::onSceneUpdated(IComponentNetScene* scene)
    {
        const char* peerId = scene->getPeerId().data();
        const char* sceneName = scene->getSceneName().data();
        if (m_sceneToPeer.count(sceneName) == 0)
        {
            m_onSceneMissing(peerId, sceneName);
            return;
        }
        PeerData* pdata = m_sceneToPeer[sceneName];
        m_sceneToPeer.emplace(sceneName, pdata);
    }

    void NetSnapshotsImpl::setOnSceneMissing(nau::Functor<void(eastl::string_view peerId, eastl::string_view sceneName)> callback)
    {
        m_onSceneMissing = eastl::move(callback);
    }

    void NetSnapshotsImpl::onComponentActivated(IComponentNetSync* component)
    {
        // TODO - track component show/hide
    }

    void NetSnapshotsImpl::onComponentDeactivated(IComponentNetSync* component)
    {
        // TODO - track component show/hide
    }

    void NetSnapshotsImpl::onComponentWrite(IComponentNetSync* component)
    {
        eastl::string_view sceneName = component->getSceneName();
        auto peerPtr = getPeer(sceneName.data());
        if (peerPtr != nullptr)
        {
            peerPtr->writeComponent(sceneName.data(), component);
        }
    }

    void NetSnapshotsImpl::nextFrame()
    {
        auto& connector = getServiceProvider().get<INetConnector>();

        for (auto& peer : m_peers)
        {
            eastl::string buffer;
            peer.second.serializeFrame(m_frame, buffer);
            if (!buffer.empty())
            {
                connector.writeFrame(peer.first, buffer);
            }
        }
        ++m_frame;
        uint32_t oldFrame = m_frame >= 2 ? m_frame - 2 : 0;
        for (auto& peer : m_peers)
        {
            peer.second.advanceToFrame(m_frame);
            peer.second.purgeFrames(oldFrame);
        }
    }

    void NetSnapshotsImpl::applyPeerUpdates()
    {
        auto& connector = getServiceProvider().get<INetConnector>();
        for (auto& peer : m_peers)
        {
            eastl::vector<eastl::string> peers;
            connector.getConnections(peer.first, peers);
            for (auto& connected : peers)
            {
                eastl::string frameBuffer;
                if (connector.readFrame(peer.first, connected, frameBuffer))
                {
                    FrameSnapshot frameSnapshot;
                    auto res = serialization::JsonUtils::parse(frameSnapshot, frameBuffer.c_str());
                    if (res.isSuccess())
                    {
                        if (m_peers.count(connected) == 0)
                        {
                            m_peers.emplace(connected, PeerData());
                        }
                        auto& dstPeer = m_peers[connected];
                        dstPeer.m_frames.clear();
                        dstPeer.m_frames.emplace(frameSnapshot.m_frame, frameSnapshot);
                        applyFrameUpdate(connected, frameSnapshot.m_frame);
                    }
                    else
                    {
                        NAU_LOG_ERROR("applyPeerUpdates parse failed");
                    }
                }
            }
        }
    }

    void NetSnapshotsImpl::applyFrameUpdate(const eastl::string& peerId, uint32_t frameNum)
    {
        if (m_peers.count(peerId) == 0)
        {
            return;
        }
        auto& peer = m_peers[peerId];
        auto& frame = peer.m_frames[frameNum];
        for (auto& sceneSnapshot : frame.m_scenes)
        {
            auto& sceneName = sceneSnapshot.first;
            if (peer.m_peerScenes.count(sceneName) == 0)
            {
                m_onSceneMissing(peerId, sceneName);
                continue;
            }
            auto* scene = peer.m_peerScenes[sceneName];
            for (auto& componentData : sceneSnapshot.second.m_components)
            {
                auto* component = scene->getOrCreateComponent(componentData.first.c_str(), "");
                if (component == nullptr)
                {
                    NAU_LOG_WARNING("applyFrameUpdate dst component not found");
                    continue;
                }
                component->netRead(componentData.second.m_data);
            }
        }
    }

    void NetSnapshotsImpl::applyPeerUpdatesLocal(const char* srcPeerId, const char* dstPeerId)
    {
        if (m_peers.count(srcPeerId) == 0 || m_peers.count(dstPeerId) == 0)
        {
            return;
        }
        auto& srcPeer = m_peers[srcPeerId];
        auto& dstPeer = m_peers[dstPeerId];
        if (srcPeer.m_frames.count(m_frame) == 0)
        {
            return;
        }
        auto& srcFrame = srcPeer.m_frames[m_frame];
        for (auto& srcScene : srcFrame.m_scenes)
        {
            auto& srcSceneName = srcScene.first;
            if (dstPeer.m_peerScenes.count(srcSceneName) == 0)
            {
                NAU_LOG_WARNING("applyPeerUpdates dst scene not found");
                continue;
            }
            auto* dstScene = dstPeer.m_peerScenes[srcSceneName];
            for (auto& componentData : srcScene.second.m_components)
            {
                auto* component = dstScene->getOrCreateComponent(componentData.first.c_str(), "");
                if (component == nullptr)
                {
                    NAU_LOG_WARNING("applyPeerUpdates dst component not found");
                    continue;
                }
                component->netRead(componentData.second.m_data);
            }
        }
    }

    void NetSnapshotsImpl::PeerData::activateScene(IComponentNetScene* scene)
    {
        const char* sceneName = scene->getSceneName().data();
        if (m_peerScenes.count(sceneName) == 0)
        {
            m_peerScenes.emplace(sceneName, scene);
        }
    }

    void NetSnapshotsImpl::PeerData::deactivateScene(IComponentNetScene* scene)
    {
        // TODO - track scene unload
    }

    void NetSnapshotsImpl::PeerData::advanceToFrame(uint32_t frame)
    {
        m_currentFrame = frame;
        m_frames.emplace(m_currentFrame, FrameSnapshot(m_currentFrame));
    }

    void NetSnapshotsImpl::PeerData::purgeFrames(uint32_t oldFrame)
    {
        for (auto it = m_frames.cbegin(); it != m_frames.cend();)
        {
            if (it->first <= oldFrame)
            {
                it = m_frames.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void NetSnapshotsImpl::PeerData::serializeFrame(uint32_t frame, eastl::string& str)
    {
        if (m_frames.count(frame) > 0)
        {
            auto& f = m_frames[frame];
            io::InplaceStringWriter<char> writer{str};
            serialization::JsonSettings settings;
            serialization::jsonWrite(writer, makeValueRef(f, getDefaultAllocator()), settings).ignore();
        }
    }

    void NetSnapshotsImpl::PeerData::deserializeFrame(const eastl::string& str)
    {
        FrameSnapshot frameSnapshot;
        auto res = serialization::JsonUtils::parse(frameSnapshot, str.c_str());
        if (res.isSuccess())
        {
            m_frames.emplace(frameSnapshot.m_frame, frameSnapshot);
        }
    }

    void NetSnapshotsImpl::PeerData::writeComponent(const eastl::string& sceneName, IComponentNetSync* component)
    {
        if (m_peerScenes.count(sceneName) == 0)
        {
            NAU_LOG_ERROR("Net writeComponent - no scene for component");
            return;
        }
        auto& frame = m_frames[m_currentFrame];
        frame.writeComponent(sceneName, component);
    }

    NetSnapshotsImpl::ComponentData::ComponentData(IComponentNetSync* component)
    {
        component->netWrite(m_data);
    }

    void NetSnapshotsImpl::FrameSnapshot::writeComponent(const eastl::string& sceneName, IComponentNetSync* component)
    {
        if (m_scenes.count(sceneName) == 0)
        {
            m_scenes.emplace(sceneName, SceneSnapshot());
        }
        auto& sceneSnapshot = m_scenes[sceneName];
        sceneSnapshot.writeComponent(component);
    }

    void NetSnapshotsImpl::SceneSnapshot::writeComponent(IComponentNetSync* component)
    {
        if (m_components.count(component->getComponentPath().data()) != 0)
        {
            NAU_LOG_ERROR("Net writeComponent must be called once per frame");
            return;
        }
        m_components.emplace(component->getComponentPath().data(), ComponentData(component));
    }

    NetSnapshotsImpl::PeerData* NetSnapshotsImpl::getPeer(const eastl::string& sceneName)
    {
        if (m_sceneToPeer.count(sceneName) == 0)
        {
            NAU_LOG_ERROR("Missing scene while trying getPeer");
            return nullptr;
        }
        return m_sceneToPeer[sceneName];
    }
}  // namespace nau
