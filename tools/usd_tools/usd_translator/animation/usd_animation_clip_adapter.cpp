// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_animation_clip_adapter.h"

#include "usd_translator/usd_prim_translator.h"
#include "usd_translator/usd_stage_translator.h"
#include "usd_animation_clip_composer.h"

#include "nau/NauAnimationClipAsset/nauAnimationController.h"
#include "nau/NauAnimationClipAsset/nauAnimationClip.h"

#include "nau/math/math.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service_provider.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/components/skinned_mesh_component.h"
#include "nau/shared/file_system.h"
#include "nau/animation/assets/animation_asset.h"
#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/animation/playback/animation_skeleton.h"
#include "nau/animation/controller/animation_controller_blend.h"
#include "nau/animation/controller/animation_controller_direct.h"
#include "nau/asset_tools/db_manager.h"
#include "nau/assets/asset_db.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/io/virtual_file_system.h"

#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>

#include <algorithm>

using namespace nau;

namespace UsdTranslator
{
    namespace
    {
        constexpr std::string_view g_typeName = "NauAnimationController";

        void addKeyframeAnimationToComponent(
            animation::AnimationComponent* component, 
            eastl::string_view animationName, 
            nau::Ptr<nau::animation::Animation> animation,
            nau::animation::PlayMode playMode,
            const pxr::UsdPrim& clipPrim)
        {
            auto rootLayer = clipPrim.GetStage()->GetRootLayer();
            auto rootLayerId = rootLayer ? rootLayer->GetIdentifier() : "";

            eastl::string animNameStr = eastl::string{ animationName };
            eastl::string sourceTrackAssetPath = std::format("{}+[kfanimation]",
                rootLayerId.empty() ? rootLayerId : FileSystemExtensions::getRelativeAssetPath(rootLayerId).string()).c_str();

            auto& dbManager = AssetDatabaseManager::instance();
            auto sourceTrackAssetUid = dbManager.findIf(sourceTrackAssetPath.c_str());

            if (sourceTrackAssetUid.isError())
            {
                sourceTrackAssetUid = Uid::generate();

                AssetMetaInfo sourceTrackAssetMeta;
                sourceTrackAssetMeta.sourcePath = sourceTrackAssetPath;
                sourceTrackAssetMeta.uid = *sourceTrackAssetUid;

                dbManager.addOrReplace(sourceTrackAssetMeta);
            }

            animation::AnimationInstanceCreationData creationData
            {
                false,
                AnimationAssetRef 
                {
                    nau::strings::toStringView(std::format("uid:{}", toString(*sourceTrackAssetUid))),
                    true
                }
            };

            auto animationInstance = rtti::createInstance<nau::animation::AnimationInstance>(animNameStr, std::move(animation), &creationData);
            animationInstance->load();
            animationInstance->setPlayMode(playMode);
            component->addAnimation(std::move(animationInstance));
        };

        void addSkeletalAnimationToComponent(
            animation::AnimationComponent* component,
            const pxr::UsdPrim& clipPrim)
        {
            auto rootLayer = clipPrim.GetStage()->GetRootLayer();
            auto rootLayerId = rootLayer ? rootLayer->GetIdentifier() : "";

            eastl::string animNameStr{ clipPrim.GetName().GetString().c_str() };
            eastl::string sourceTrackAssetPath = std::format("{}",//+[skanimation:{}]",
                rootLayerId != "" ? FileSystemExtensions::getRelativeAssetPath(rootLayerId, true).string() : "").c_str();// ,
                //animNameStr.c_str()).c_str();

            auto& dbManager = AssetDatabaseManager::instance();
            auto sourceTrackAssetUid = dbManager.findIf(sourceTrackAssetPath.c_str());

            if (sourceTrackAssetUid.isError())
            {
                sourceTrackAssetUid = Uid::generate();

                AssetMetaInfo sourceTrackAssetMeta;
                sourceTrackAssetMeta.sourcePath = sourceTrackAssetPath;
                sourceTrackAssetMeta.uid = *sourceTrackAssetUid;

                dbManager.addOrReplace(sourceTrackAssetMeta);
            }

            auto assetRef = AnimationAssetRef
            {
                nau::strings::toStringView(std::format("uid:{}", toString(*sourceTrackAssetUid))),
                true
            };

            auto animation = rtti::createInstance<nau::animation::SkeletalAnimation>();
            auto animationInstance = rtti::createInstance<nau::animation::AnimationInstance>(animNameStr, assetRef);
            //animationInstance->load();
            animationInstance->setPlayMode(nau::animation::PlayMode::Looping);
            component->addAnimation(std::move(animationInstance));
        };
    }

    template <typename AnimationType, typename DataType>
    static void setupAnimation(nau::Ptr<AnimationType> animation, float frameRate, const eastl::vector<float>& times, const eastl::vector<DataType>& data)
    {
        auto editor = animation->createEditor();
        if (!editor)
        {
            return;
        }
        std::for_each(times.begin(), times.end(), [frameRate, &editor, dataIt = data.begin()](float time) mutable
        {
            const int frame = static_cast<int>(std::round(frameRate * time));
            editor.addKeyFrame(frame, *dataIt);
            ++dataIt;
        });
    }

    class UsdAnimationClipAccessor final : public IAnimationAssetAccessor
    {
        NAU_CLASS_(UsdTranslator::UsdAnimationClipAccessor, IAnimationAssetAccessor);

    public:
        UsdAnimationClipAccessor(pxr::UsdPrim prim) :
            m_clip(prim)
        {
        }

        AnimationDataDescriptor getDataDescriptor() const override
        {
            if (const auto& descriptors = m_clip.getAnimationDataDescriptors(); !descriptors.empty())
            {
                return descriptors.front();
            }
            return {};
        }

        async::Task<> copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const override
        {
            m_clip.getTrackData(desc, times, data);

            return async::Task<>::makeResolved();
        }

        async::Task<> copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const override
        {
            m_clip.getTrackData(desc, times, data);

            return async::Task<>::makeResolved();
        }

        async::Task<> copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const override
        {
            // todo: implement
            return async::Task<>::makeResolved();
        }

        virtual nau::Ptr<nau::ISkeletonAssetAccessor> getSkeletonAsset() const override
        {
            return nullptr;
        }

    private:
        AnimationClipComposer m_clip;
    };


    AnimationControllerAdapter::AnimationControllerAdapter(pxr::UsdPrim prim) :
        IPrimAdapter(prim)
    {
    }

    AnimationControllerAdapter::~AnimationControllerAdapter() noexcept = default;

    std::string_view AnimationControllerAdapter::getType() const
    {
        return g_typeName;
    }

    nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> AnimationControllerAdapter::initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest)
    {
        m_obj = nau::scene::ObjectWeakRef(dest);

        co_await m_obj->addComponentAsync<animation::AnimationComponent>();
        co_await update();

        co_return m_obj;
    }

    nau::scene::ObjectWeakRef<nau::scene::SceneObject> AnimationControllerAdapter::getSceneObject() const
    {
        return m_obj;
    }

    nau::async::Task<> AnimationControllerAdapter::update()
    {
        if (!isValid())
        {
            co_return;
        }

        auto* component = m_obj->findFirstComponent<animation::AnimationComponent>();
        if (component == nullptr)
        {
            co_return;
        }

        pxr::SdfAssetPath assetPath;
        pxr::UsdNauAnimationController usdController{ getPrim() };
        usdController.GetAnimationAssetAttr().Get(&assetPath);
        const std::string& clipUid = assetPath.GetAssetPath();
        if (clipUid.empty())
        {
            co_return;
        }

        auto&& assetDb = nau::getServiceProvider().get<nau::IAssetDB>();
        auto&& uidResult = nau::Uid::parseString(clipUid.starts_with("uid:") ? clipUid.substr(4) : clipUid);
        if (uidResult.isError())
        {
            // todo: fix this
            uidResult = assetDb.getUidFromSourcePath(clipUid.c_str());
            if (*uidResult)
            {
                assetPath = pxr::SdfAssetPath{ nau::toString(*uidResult) };
                usdController.GetAnimationAssetAttr().Set(assetPath);
            }
            else
            {
                co_return;
            }
        }

        auto&& sourcePath = assetDb.getSourcePathFromUid(*uidResult);
        if (sourcePath.empty())
        {
            co_return;
        }

        const std::string path = nau::Paths::instance().getAssetsPath() + '/' + sourcePath.substr(0, sourcePath.find('+')).c_str();
        pxr::UsdStageRefPtr assetStage = pxr::UsdStage::Open(path.ends_with(".usda") ? path : path + ".usda");
        if (assetStage == nullptr)
        {
            co_return;
        }

        pxr::UsdPrimRange primList = assetStage->Traverse();
        [[maybe_unused]]
        const bool result = std::any_of(primList.begin(), primList.end(), [this, component, uidResult](const pxr::UsdPrim& prim)
        {
            if (prim.IsA<pxr::UsdNauAnimationClip>())
            {
                setupAnimationClip(prim, component);
                return true;
            }
            if (prim.IsA<pxr::UsdSkelRoot>())
            {
                setupAnimationSkel(prim, component, *uidResult);
                return true;
            }
            return false;
        });
    }

    bool AnimationControllerAdapter::isValid() const
    {
        return !!m_obj;
    }

    void AnimationControllerAdapter::destroySceneObject()
    {
        m_obj = nullptr;
    }

    void AnimationControllerAdapter::setupAnimationClip(const pxr::UsdPrim& clipPrim, nau::animation::AnimationComponent* component)
    {
        auto newController = rtti::createInstance<nau::animation::DirectAnimationController, nau::animation::AnimationController>();
        component->setController(std::move(newController));

        AnimationClipComposer composer{ clipPrim };

        const auto& descriptors = composer.getAnimationDataDescriptors();
        if (descriptors.empty())
        {
            return;
        }

        const float frameRate = component->getController()->getFrameRate();
        for (const auto& descriptor : descriptors)
        {
            eastl::vector<float> dataTimes;
            eastl::vector<nau::math::vec3> dataVec3;
            eastl::vector<nau::math::quat> dataQuat;

            switch (descriptor.dataType)
            {
                case nau::AnimationDataDescriptor::DataType::Translation:
                {
                    composer.getTrackData(descriptor, dataTimes, dataVec3);

                    auto animation = rtti::createInstance<nau::animation::TranslationAnimation>();
                    setupAnimation(animation, frameRate, dataTimes, dataVec3);
                    addKeyframeAnimationToComponent(component, descriptor.name, std::move(animation), composer.getPlayMode(), clipPrim);
                }
                break;
                case nau::AnimationDataDescriptor::DataType::Rotation:
                {
                    eastl::vector<nau::math::quat> data;
                    composer.getTrackData(descriptor, dataTimes, dataQuat);

                    auto animation = rtti::createInstance<nau::animation::RotationAnimation>();
                    setupAnimation(animation, frameRate, dataTimes, dataQuat);
                    addKeyframeAnimationToComponent(component, descriptor.name, std::move(animation), composer.getPlayMode(), clipPrim);
                }
                break;
                case nau::AnimationDataDescriptor::DataType::Scale:
                {
                    composer.getTrackData(descriptor, dataTimes, dataVec3);

                    auto animation = rtti::createInstance<nau::animation::ScaleAnimation>();
                    setupAnimation(animation, frameRate, dataTimes, dataVec3);
                    addKeyframeAnimationToComponent(component, descriptor.name, std::move(animation), composer.getPlayMode(), clipPrim);
                }
                break;
            }
        }
    }

    void AnimationControllerAdapter::setupAnimationSkel(const pxr::UsdPrim& skelPrim, nau::animation::AnimationComponent* component, nau::Uid uid)
    {
        nau::Ptr<nau::animation::AnimationMixer> animMixer = rtti::createInstance<nau::animation::SkeletalAnimationMixer>().get();
        nau::Ptr<nau::animation::BlendAnimationController> blendController = rtti::createInstance<nau::animation::BlendAnimationController>(animMixer);
        component->setController(std::move(blendController));

        auto* skeleton = m_obj->findFirstComponent<nau::SkeletonComponent>();
        if (skeleton == nullptr)
        {
            skeleton = &m_obj->addComponent<nau::SkeletonComponent>();
        }
        auto skeletonAsset = nau::SkeletonAssetRef
        {
            nau::strings::toStringView(std::format("uid:{}", toString(uid))),
            true
        };
        skeleton->setSkeletonAsset(std::move(skeletonAsset));
        addSkeletalAnimationToComponent(component, skelPrim);;
    }

    [[maybe_unused]] DEFINE_TRANSLATOR(AnimationControllerAdapter, "AnimationController"_tftoken);
}  // namespace UsdTranslator
