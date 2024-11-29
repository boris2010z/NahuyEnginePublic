// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/audio/audio_common.hpp"
#include "nau/audio/audio_container.hpp"
#include "nau/meta/class_info.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/scene.h"
#include "nau/rtti/ptr.h"


NAU_AUDIO_BEGIN

struct NAU_AUDIO_EXPORT AudioComponentEmitter
    : public scene::SceneComponent,
      public scene::IComponentUpdate,
      public scene::IComponentActivation
{
    NAU_OBJECT(AudioComponentEmitter, scene::SceneComponent, scene::IComponentUpdate, scene::IComponentActivation)
    NAU_DECLARE_DYNAMIC_OBJECT

    NAU_CLASS_ATTRIBUTES(
        CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
        CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Audio Emitter"),
        CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Audio Emitter (description)"))

    NAU_CLASS_FIELDS(
        CLASS_FIELD(containerAssetUid),
        CLASS_FIELD(loop),
        CLASS_FIELD(playOnStart))

    /**
     * @brief Advances all managed audio sources playbacks.
     * 
     * @param [in] dt Time in seconds elapsed since the last update.
     */
    void updateComponent(float dt) override;

    void activateComponent() override;
    void deactivateComponent() override;

    // Properties
    nau::Uid containerAssetUid;
    AudioAssetContainerPtr container = nullptr;
    AudioSourcePtr source = nullptr;
    bool loop = false;
    bool playOnStart = false;

protected:
    void createContainerFromBlk(const eastl::string& path);

private:
    enum State
    {
        Unloaded,
        Playing
    } state;
};

NAU_AUDIO_END
