// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/async/task_base.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/scene_component.h"
#include "nau/assets/asset_ref.h"
#include "nau/ui/elements/canvas.h"
#include "nau/scene/scene.h"


namespace nau::ui
{
    class NAU_UI_EXPORT UiComponent final : public scene::SceneComponent,
                                            public scene::IComponentEvents,
                                            public scene::IComponentActivation
    {
        NAU_OBJECT(nau::ui::UiComponent,
            scene::SceneComponent,
            scene::IComponentEvents,
            scene::IComponentActivation
        )
        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "UI scene"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "UI scene (description)"),
        )

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_uiAssetPath, "uiAssetName"),
            CLASS_NAMED_FIELD(m_width, "canvasWidth"),
            CLASS_NAMED_FIELD(m_height, "canvasHeight"),
        )

    public:
        UiComponent();
        ~UiComponent();

        virtual async::Task<> activateComponentAsync() override;
        virtual void deactivateComponent() override;

    private:
        void clearCanvas();

    private:
        mutable std::string m_uiAssetPath;
        mutable double m_width = 800.;
        mutable double m_height = 600.;

        Canvas* m_canvas = nullptr;
        bool m_canvasInScene = false;
        scene::IScene::WeakRef m_engineScene;
    };

}  // namespace nau::ui
