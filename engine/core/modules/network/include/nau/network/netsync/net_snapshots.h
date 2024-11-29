// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

#include "nau/memory/bytes_buffer.h"
#include "nau/network/components/net_component_api.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/functor.h"

namespace nau
{
    /**
     * @brief Provides an interface for network snapshot service.
     */
    struct NAU_ABSTRACT_TYPE INetSnapshots
    {
        NAU_TYPEID(INetSnapshots)

        /**
         * @brief Callback that is dispatched when a scene with an IComponentNetScene object attached is activated.
         * 
         * @param [in] scene A pointer to IComponentNetScene object.
         */
        virtual void onSceneActivated(IComponentNetScene* scene) = 0;

        /**
         * @brief Callback that is dispatched when a scene with an IComponentNetScene object attached is deactivated.
         *
         * @param [in] scene A pointer to IComponentNetScene object.
         */
        virtual void onSceneDectivated(IComponentNetScene* scene) = 0;

        /**
         * @brief Callback that is dispatched when a scene with an IComponentNetScene object attached is updated (which is expected to happen once per frame).
         * 
         * @param [in] scene A pointer to IComponentNetScene object.
         */
        virtual void onSceneUpdated(IComponentNetScene* scene) = 0;

        /**
         * @brief Sets a callback for the scene manager that is dispatched when a snapshot for not yet activated scene is received.
         * 
         * @param [in] callback Callback to assign.
         * 
         * @note The callback should take the peer ID and the scene name as parameters.
         */
        virtual void setOnSceneMissing(nau::Functor<void(eastl::string_view peerId, eastl::string_view sceneName)> callback) = 0;

        /**
         * @brief Callback that is dispatched when an IComponentNetSync scene component is activated.
         * 
         * @param [in] component A pointer to IComponentNetSync component.
         */
        virtual void onComponentActivated(IComponentNetSync* component) = 0;

        /**
         * @brief Callback that is dispatched when an IComponentNetSync scene component is deactivated.
         *
         * @param [in] component A pointer to IComponentNetSync component.
         */
        virtual void onComponentDeactivated(IComponentNetSync* component) = 0;

        /**
         * @brief Callback that is dispatched when scene an IComponentNetSync component is updated and serialized.
         * 
         * @param [in] component A pointer to IComponentNetSync component.
         */
        virtual void onComponentWrite(IComponentNetSync* component) = 0;

        /**
         * @brief Advances networking to the next frame. The function must be called once per frame.
         */
        virtual void nextFrame() = 0;

        /**
         * @brief Applies all incoming scene/component updates.
         */
        virtual void applyPeerUpdates() = 0;

        virtual bool doSelfTest() = 0;
    };
}  // namespace nau
