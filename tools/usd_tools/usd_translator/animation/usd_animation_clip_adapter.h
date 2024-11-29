// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "usd_translator/usd_translator_api.h"
#include "usd_translator/usd_prim_adapter.h"
#include "usd_proxy/usd_proxy.h"

#include "nau/assets/asset_container.h"

#include <nau/scene/scene.h>


namespace nau::animation
{
    class AnimationComponent;
}

namespace UsdTranslator
{
    class USD_TRANSLATOR_API AnimationControllerAdapter : public IPrimAdapter
    {
    public:
        AnimationControllerAdapter(pxr::UsdPrim prim);
        ~AnimationControllerAdapter() noexcept;

        std::string_view getType() const override;
        nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest) override;
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> getSceneObject() const override;
        nau::async::Task<> update() override;
        bool isValid() const override;

    protected:
        void destroySceneObject() override;

        void setupAnimationClip(const pxr::UsdPrim& clipPrim, nau::animation::AnimationComponent* component);
        void setupAnimationSkel(const pxr::UsdPrim& skelPrim, nau::animation::AnimationComponent* component, nau::Uid uid);

    private:
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> m_obj = nullptr;
    };
}  // namespace UsdTranslator
