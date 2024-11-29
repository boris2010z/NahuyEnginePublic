// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/asset_tools/compilers/animation_compilers.h"

#include "nau/animation/playback/animation_instance.h"
#include "nau/asset_tools/asset_utils.h"
#include "nau/asset_tools/db_manager.h"
#include "nau/assets/animation_asset_accessor.h"
#include "nau/assets/ui_asset_accessor.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/io/stream_utils.h"
#include "nau/NauAnimationClipAsset/nauAnimationClip.h"
#include "nau/NauAnimationClipAsset/nauAnimationTrack.h"
#include <nau/shared/logger.h>
#include "nau/usd_meta_tools/usd_meta_info.h"

#include <EASTL/vector.h>
#include <pxr/usd/sdf/fileFormat.h>
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>

#include "nau/asset_tools/gltf2ozz_copy.h"

namespace nau::compilers
{
    namespace
    {
        struct KeyFrameAnimationTrackData
        {
            KeyFrameAnimationTrackData(const AnimationDataDescriptor& desc)
                : descriptor(desc)
            { }

            virtual ~KeyFrameAnimationTrackData() = default;
            virtual void toBlk(DataBlock& blk) const { }
            virtual void pushVector(float time, const math::vec3& value) { }

            static eastl::unique_ptr<KeyFrameAnimationTrackData> create(const AnimationDataDescriptor& descriptor);

            AnimationDataDescriptor descriptor;
        };

        void descriptorToBlk(DataBlock& blk, const AnimationDataDescriptor& descriptor)
        {
            if (auto* descriptorBlock = blk.addBlock("descriptor"))
            {
                descriptorBlock->addInt("animationIndex", (int)descriptor.animationIndex);
                descriptorBlock->addInt("channelIndex", (int)descriptor.channelIndex);
                descriptorBlock->addInt("dataType", (int)descriptor.dataType);
                descriptorBlock->addInt("interpolation", (int)descriptor.interpolation);
                descriptorBlock->addStr("name", descriptor.name.c_str());
            }
        }

        struct SkeletalAnimationTrackData
        {
            void toBlk(DataBlock& blk) const 
            { 
                if (auto* trackBlock = blk.addNewBlock("track"))
                {
                    AnimationDataDescriptor descriptor;
                    descriptor.dataType = AnimationDataDescriptor::DataType::Skeletal;
                    descriptorToBlk(*trackBlock, descriptor);

                    trackBlock->addStr("skeleton", skeletonAssetPath.c_str());
                    trackBlock->addStr("animation", animationAssetPath.c_str());
                }
            }

            eastl::string skeletonAssetPath;
            eastl::string animationAssetPath;
        };

        template<typename TDataType>
        struct KeyFrameData
        {
            float time = .0f;
            TDataType value;
        };

        template<typename TDataType>
        struct KeyFrameAnimationTrackDataImpl : KeyFrameAnimationTrackData
        {
            using TKeyFrame = KeyFrameData<TDataType>;
            using TDataContainer = eastl::vector<TKeyFrame>;
            using TImpl = KeyFrameAnimationTrackDataImpl<TDataType>;

            KeyFrameAnimationTrackDataImpl(const AnimationDataDescriptor& desc)
                : KeyFrameAnimationTrackData(desc)
            { }

            template<class T>
            void pushKeyFrame(float time, T value) { }
            void pushKeyFrame(float time, const TDataType& value)
            {
                data.push_back({ time, value });
            }

            virtual void pushVector(float time, const math::vec3& value) override
            {
                pushKeyFrame(time, value);
            }

            template<class T>
            void writeData(DataBlock& blk, const T& value) const
            { }

            template<>
            void writeData<math::vec3>(DataBlock& blk, const math::vec3& value) const
            {
                blk.addPoint3("v", value);
            }

            template<>
            void writeData<math::quat>(DataBlock& blk, const math::quat& value) const
            {
                blk.addPoint4("v", { value.getX(), value.getY(), value.getZ(), value.getW() });
            }

            void writeDataContainer(DataBlock& blk) const
            {
                for (const auto& kf : data)
                {
                    if (auto* frameBlock = blk.addNewBlock("frame"))
                    {
                        frameBlock->addReal("t", kf.time);
                        writeData(*frameBlock, kf.value);
                    }
                }
            }

            virtual void toBlk(DataBlock& parentBlock) const override
            {
                KeyFrameAnimationTrackData::toBlk(parentBlock);

                if (auto* trackBlock = parentBlock.addNewBlock("track"))
                {
                    descriptorToBlk(*trackBlock, descriptor);

                    if (auto* valuesBlock = trackBlock->addBlock("values"))
                    {
                        writeDataContainer(*valuesBlock);
                    }
                }
            }

            TDataContainer data;
        };

        struct TranslationAnimationTrackData : public KeyFrameAnimationTrackDataImpl<math::vec3>
        {
            TranslationAnimationTrackData(const AnimationDataDescriptor& desc)
                : TImpl(desc)
            { }
        };

        struct RotationAnimationTrackData : public KeyFrameAnimationTrackDataImpl<math::quat>
        {
            RotationAnimationTrackData(const AnimationDataDescriptor& desc)
                : TImpl(desc)
            { }
        };

        struct ScaleAnimationTrackData : public KeyFrameAnimationTrackDataImpl<math::vec3>
        {
            ScaleAnimationTrackData(const AnimationDataDescriptor& desc)
                : TImpl(desc)
            { }
        };

        eastl::unique_ptr<KeyFrameAnimationTrackData> KeyFrameAnimationTrackData::create(const AnimationDataDescriptor& descriptor)
        {
            switch (descriptor.dataType)
            {
            case AnimationDataDescriptor::DataType::Translation:
                return eastl::make_unique<TranslationAnimationTrackData>(descriptor);

            case AnimationDataDescriptor::DataType::Rotation:
                return eastl::make_unique<RotationAnimationTrackData>(descriptor);

            case AnimationDataDescriptor::DataType::Scale:
                return eastl::make_unique<ScaleAnimationTrackData>(descriptor);
            }

            return nullptr;
        }

        struct AnimationAssetData
        {
            virtual ~AnimationAssetData() = default;
            virtual void toBlk(DataBlock& blk) const = 0;
        };

        struct KeyFrameAnimationAssetData : AnimationAssetData
        {
            void toBlk(DataBlock& blk) const
            {
                for (const auto& track : tracks)
                {
                    track->toBlk(blk);
                }
            }
            void addTrack(eastl::unique_ptr<KeyFrameAnimationTrackData>&& track)
            {
                tracks.push_back(std::move(track));
            }

            eastl::vector<eastl::shared_ptr<KeyFrameAnimationTrackData>> tracks;
        };

        struct SkeletalAnimationAssetData : AnimationAssetData
        {
            void toBlk(DataBlock& blk) const
            {
                for (const auto& track : tracks)
                {
                    track.toBlk(blk);
                }

                if (auto jointsBlock = blk.addBlock("joints"))
                {
                    jointsBlock->addInt("jointsCount", jointsCount);

                    if (auto* bindsBlock = jointsBlock->addBlock("bind_matrices"))
                    {
                        for (const auto& bindMatrix : bindMatrices)
                        {
                            if (auto* bmBlock = bindsBlock->addNewBlock("bm"))
                            {
                                bmBlock->addPoint4("c0", bindMatrix.getCol0());
                                bmBlock->addPoint4("c1", bindMatrix.getCol1());
                                bmBlock->addPoint4("c2", bindMatrix.getCol2());
                                bmBlock->addPoint4("c3", bindMatrix.getCol3());
                            }
                        }
                    }
                }
            }
            void addTrack(SkeletalAnimationTrackData&& track)
            {
                tracks.push_back(std::move(track));
            }

            eastl::vector<SkeletalAnimationTrackData> tracks;
            int32_t jointsCount = 0;
            eastl::vector<nau::math::mat4> bindMatrices;
        };

        bool saveAnimAssetBlk(const AnimationAssetData& asset, const std::string& outputFilePath)
        {
            DataBlock outBlk;

            asset.toBlk(outBlk);

            return outBlk.saveToTextFile(outputFilePath.c_str());
        }

        eastl::unique_ptr<KeyFrameAnimationTrackData> loadAnimationTrack(const nau::AnimationDataDescriptor& descriptor, const pxr::UsdPrim& trackPrim)
        {
            eastl::unique_ptr<KeyFrameAnimationTrackData> clip = KeyFrameAnimationTrackData::create(descriptor);

            if (!clip)
            {
                return nullptr;
            }

            const pxr::TfToken keyframeToken{ "keyframes" };
            const auto&& keyframes = trackPrim.GetAttribute(keyframeToken);
            if (!keyframes)
            {
                return nullptr;
            }
            // so far, only the float3 type is supported
            const auto&& typeName = keyframes.GetTypeName();
            if (typeName != pxr::SdfValueTypeNames->Float3)
            {
                return nullptr;
            }

            std::vector<double> timeSamples;
            keyframes.GetTimeSamples(&timeSamples);

            for (double time : timeSamples)
            {
                pxr::GfVec3f vec;
                keyframes.Get(&vec, time);
                clip->pushVector(time, { vec[0], vec[1], vec[2] });
            }

            return clip;
        }

        eastl::unique_ptr<KeyFrameAnimationTrackData> compileAnimationTrack(pxr::UsdNauAnimationClip& clip, const pxr::UsdPrim& prim)
        {
            pxr::TfToken modeToken;
            clip.GetPlayModeAttr().Get(&modeToken);
            const std::string& mode = modeToken.GetString();

            nau::animation::PlayMode playMode;
            if (mode == "Once")
            {
                playMode = nau::animation::PlayMode::Once;
            }
            else if (mode == "Looping")
            {
                playMode = nau::animation::PlayMode::Looping;
            }
            else if (mode == "PingPong")
            {
                playMode = nau::animation::PlayMode::PingPong;
            }

            int animationIndex = 0;

            for (auto&& trackPrim : prim.GetAllChildren())
            {
                const pxr::UsdNauAnimationTrack track{ trackPrim };

                pxr::TfToken dataTypeToken;
                track.GetDataTypeAttr().Get(&dataTypeToken);
                const std::string& dataTypeStr = dataTypeToken.GetString();

                nau::AnimationDataDescriptor descriptor;
                descriptor.name = clip.GetPrim().GetName().GetString().c_str();
                if (dataTypeStr == "Translation")
                {
                    descriptor.dataType = nau::AnimationDataDescriptor::DataType::Translation;
                }
                else if (dataTypeStr == "Rotation")
                {
                    descriptor.dataType = nau::AnimationDataDescriptor::DataType::Rotation;
                }
                else if (dataTypeStr == "Scale")
                {
                    descriptor.dataType = nau::AnimationDataDescriptor::DataType::Scale;
                }

                pxr::TfToken interpolationToken;
                track.GetInterpolationAttr().Get(&interpolationToken);
                const std::string& interpolationStr = interpolationToken.GetString();
                if (interpolationStr == "No")
                {
                    descriptor.interpolation = nau::AnimationDataDescriptor::InterpolationType::No;
                }
                else if (interpolationStr == "Linear")
                {
                    descriptor.interpolation = nau::AnimationDataDescriptor::InterpolationType::Linear;
                }

                descriptor.animationIndex = animationIndex++;
                descriptor.channelIndex = 0;
                return loadAnimationTrack(descriptor, trackPrim);
            }

            return nullptr;
        }

        eastl::optional<KeyFrameAnimationAssetData> loadUsdAnimation(const pxr::UsdPrim& prim, const std::string& outputPath, const UsdMetaInfo& metaInfo)
        {
            KeyFrameAnimationAssetData loadedAsset;

            if (pxr::UsdNauAnimationClip clip{ prim })
            {
                if (auto clipAsset = compileAnimationTrack(clip, prim))
                {
                    loadedAsset.addTrack(std::move(clipAsset));
                }

                return loadedAsset;
            }

            bool isLoaded = false;

            for (auto child : prim.GetAllChildren())
            {
                if (pxr::UsdNauAnimationClip clip{ child })
                {
                    isLoaded = true;

                    if (auto clipAsset = compileAnimationTrack(clip, child))
                    {
                        loadedAsset.addTrack(std::move(clipAsset));
                    }
                }
            }

            if (!isLoaded)
            {
                for (const auto& spec : prim.GetPrimStack())
                {
                    auto&& items = spec.GetSpec().GetReferenceList().GetPrependedItems();
                    for (const pxr::SdfReference& reference : items)
                    {
                        if (auto assetStage = pxr::UsdStage::Open(reference.GetAssetPath()))
                        {
                            auto&& prim = assetStage->GetPrimAtPath(reference.GetPrimPath());
                            if (pxr::UsdNauAnimationClip clip{ prim })
                            {
                                if (auto clipAsset = compileAnimationTrack(clip, prim))
                                {
                                    loadedAsset.addTrack(std::move(clipAsset));
                                    isLoaded = true;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if (isLoaded)
            {
                return loadedAsset;
            }

            return { };
        }

        std::filesystem::path ensureTempDirectory(const std::filesystem::path& targetDirectoryPath, bool clearDirectory)
        {
            namespace fs = std::filesystem;
            auto tmpDirPath = targetDirectoryPath / "__tmp";

            if (!fs::exists(tmpDirPath))
            {
                fs::create_directories(tmpDirPath);
            }
            else if (clearDirectory)
            {
                for (const auto& tmpDirEntry : fs::directory_iterator(tmpDirPath))
                {
                    fs::remove(tmpDirEntry);
                }
            }

            return tmpDirPath;
        }

        nau::Result<AssetMetaInfo> exportSkeletalGltf2Nanim(
            const std::string& outputPath,
            const std::string& assetPath,
            const std::string& sourceGltfPath,
            int folderIndex,
            bool clearTempDir,
            SkeletalAnimationAssetData& nanimData)
        {
            const std::string relativeSourcePath = FileSystemExtensions::getRelativeAssetPath(assetPath, true).string();
            const std::string skeletonSourcePath = std::format("{}+[skeleton]", relativeSourcePath.c_str());

            AssetDatabaseManager& dbManager = AssetDatabaseManager::instance();
            auto skeletonAssetId = dbManager.findIf(skeletonSourcePath);

            if (skeletonAssetId.isError())
            {
                skeletonAssetId = Uid::generate();
            }

            namespace fs = std::filesystem;
            const fs::path targetDirectoryPath = fs::path(outputPath) / std::to_string(folderIndex);
            std::string skeletonFileName = toString(*skeletonAssetId) + ".ozz";

            AssetMetaInfo skeletonMeta;
            skeletonMeta.uid = *skeletonAssetId;
            skeletonMeta.dbPath = (targetDirectoryPath.filename() / skeletonFileName).string().c_str();
            skeletonMeta.sourcePath = skeletonSourcePath.c_str();
            skeletonMeta.sourceType = "usda";
            skeletonMeta.kind = "";
            dbManager.addOrReplace(skeletonMeta);

            std::string outputSkeletonPath = (targetDirectoryPath / skeletonFileName).string();

            auto tmpDirPath = ensureTempDirectory(targetDirectoryPath, clearTempDir);
            auto tempGltfPath = tmpDirPath / fs::path{ sourceGltfPath }.filename().replace_extension(".gltf");

            if (fs::exists(tempGltfPath) || fs::copy_file(fs::path{ sourceGltfPath }, tempGltfPath))
            {
                auto tempBinPath = fs::path{ tempGltfPath }.replace_extension(".bin");
                auto sourceBinPath = fs::path{ sourceGltfPath }.replace_extension(".bin");

                if (fs::exists(sourceBinPath) && !fs::exists(tempBinPath))
                {
                    fs::copy_file(sourceBinPath, tempBinPath);
                }

                std::string outputAnimationsPath = (tmpDirPath / "*.ozz").string();

                std::string ozzGltfPath = std::format("--file={}", tempGltfPath.string());
                std::replace(ozzGltfPath.begin(), ozzGltfPath.end(), '\\', '/');
                std::string ozzParams = std::format("--config={{\"skeleton\":{{\"filename\":\"{}\"}},\"animations\":[{{\"filename\":\"{}\"}}]}}", outputSkeletonPath, outputAnimationsPath);
                std::replace(ozzParams.begin(), ozzParams.end(), '\\', '/');

                const char* params[] =
                {
                    "exec_name_unused",
                    ozzGltfPath.c_str(),
                    ozzParams.c_str()
                };

                bool isExported = Gltf2OzzConverter().executeGltf2OzzTool(3, params) == 0;

                fs::remove(tempGltfPath);
                fs::remove(tempBinPath);

                if (isExported)
                {
                    for (const auto& exportedAnimFileEntry : fs::directory_iterator(tmpDirPath))
                    {
                        const std::string animationSourcePath = std::format("{}+[skanimation:{}]",
                            relativeSourcePath.c_str(),
                            exportedAnimFileEntry.path().stem().string());

                        auto animationAssetId = dbManager.findIf(animationSourcePath);

                        if (animationAssetId.isError())
                        {
                            animationAssetId = Uid::generate();
                        }

                        std::string targetAnimationFileName = toString(*animationAssetId) + ".ozz";
                        auto targetAnimationPath = targetDirectoryPath / targetAnimationFileName;

                        if (fs::exists(targetAnimationPath))
                        {
                            fs::remove(targetAnimationPath);
                        }

                        if (fs::copy_file(exportedAnimFileEntry, targetAnimationPath))
                        {
                            AssetMetaInfo animationMeta;
                            animationMeta.uid = *animationAssetId;
                            animationMeta.dbPath = (targetDirectoryPath.filename() / targetAnimationFileName).string().c_str();
                            animationMeta.sourcePath = animationSourcePath.c_str();
                            animationMeta.sourceType = "usda";
                            animationMeta.kind = "";
                            dbManager.addOrReplace(animationMeta);

                            nanimData.addTrack(SkeletalAnimationTrackData
                                {
                                    std::format("uid:{}", toString(skeletonMeta.uid)).c_str(),
                                    std::format("uid:{}", toString(animationMeta.uid)).c_str()
                                });
                        }

                        fs::remove(exportedAnimFileEntry);
                    }

                    AssetMetaInfo nanimMeta;
                    const std::string nanimSourcePath = std::format("{}", relativeSourcePath);

                    auto id = dbManager.findIf(nanimSourcePath);

                    if (id.isError())
                    {
                        id = Uid::generate();
                    }

                    const std::filesystem::path out = std::filesystem::path(outputPath) / std::to_string(folderIndex);
                    std::string fileName = toString(*id) + ".nanim";

                    nanimMeta.uid = *id;
                    nanimMeta.dbPath = (out.filename() / fileName).string().c_str();
                    nanimMeta.sourcePath = nanimSourcePath.c_str();
                    nanimMeta.sourceType = "usda";
                    nanimMeta.kind = "Animation";

                    auto nanimOutPath = utils::compilers::ensureOutputPath(outputPath, nanimMeta, "");

                    fs::remove(tmpDirPath);

                    if (saveAnimAssetBlk(nanimData, nanimOutPath.string()))
                    {
                        return nanimMeta;
                    }
                }
            }

            return NauMakeError("Skeletal animation export to ozz failed");
        }

        nau::Result<SkeletalAnimationAssetData> getBindsDataFromPrim(const pxr::UsdSkelRoot& skelRoot)
        {
            SkeletalAnimationAssetData data;

            pxr::UsdSkelCache skelCache;
            skelCache.Populate(skelRoot, pxr::UsdTraverseInstanceProxies());

            std::vector<pxr::UsdSkelBinding> bindings;
            skelCache.ComputeSkelBindings(skelRoot, &bindings, pxr::UsdTraverseInstanceProxies());
            if (bindings.empty())
            {
                return data;
            }

            pxr::VtArray<pxr::GfMatrix4d> matrixList;
            bindings.front().GetSkeleton().GetBindTransformsAttr().Get(&matrixList);
            data.jointsCount = static_cast<int32_t>(matrixList.size());

            data.bindMatrices.reserve(matrixList.size());
            for (const auto& matrix : matrixList)
            {
                auto inverseMatrix = matrix.GetInverse();
                const auto row0 = inverseMatrix.GetRow(0);
                const auto row1 = inverseMatrix.GetRow(1);
                const auto row2 = inverseMatrix.GetRow(2);
                const auto row3 = inverseMatrix.GetRow(3);

                data.bindMatrices.emplace_back(
                    nau::math::vec4(row0[0], row0[1], row0[2], row0[3]),
                    nau::math::vec4(row1[0], row1[1], row1[2], row1[3]),
                    nau::math::vec4(row2[0], row2[1], row2[2], row2[3]),
                    nau::math::vec4(row3[0], row3[1], row3[2], row3[3])
                );
            }

            return data;
        }

        nau::Result<SkeletalAnimationAssetData> getBindsDataFromPrim(const pxr::UsdPrim& prim)
        {
            if (pxr::UsdSkelRoot skelRoot{ prim })
            {
                return getBindsDataFromPrim(skelRoot);
            }

            for (auto childPrim : prim.GetAllChildren())
            {
                if (auto bindsData = getBindsDataFromPrim(childPrim))
                {
                    return bindsData;
                }
            }

            return NauMakeError("No binds data");
        }

    } // namespace

    nau::Result<AssetMetaInfo> UsdKeyFrameAnimationCompiler::compile(
        PXR_NS::UsdStageRefPtr stage,
        const std::string& outputPath,
        const std::string& projectRootPath,
        const nau::UsdMetaInfo& metaInfo,
        int folderIndex)
    {
        AssetDatabaseManager& dbManager = AssetDatabaseManager::instance();
        NAU_ASSERT(dbManager.isLoaded(), "Asset database not loaded!");

        auto stageToCompile = pxr::UsdStage::Open(metaInfo.assetPath);

        if (!stageToCompile)
        {
            return NauMakeError("Can't load source stage from '{}'", metaInfo.assetPath);
        }

        auto extraInfo = reinterpret_cast<ExtraInfoAnimation*>(metaInfo.extraInfo.get());

        if (!extraInfo)
        {
            return NauMakeError("Empty extra info for asset '{}'", metaInfo.assetPath);
        }

        auto primToCompile = stageToCompile->GetPrimAtPath(pxr::SdfPath{ extraInfo->source });

        if (!primToCompile.IsValid())
        {
            if (auto defaultPrim = stageToCompile->GetDefaultPrim())
            {
                primToCompile = stageToCompile->GetDefaultPrim();
            }
            else
            {
                primToCompile = stageToCompile->GetPseudoRoot();
            }
        }

        if (!primToCompile.IsValid())
        {
            LOG_ERROR("Prim {} is invalid!", metaInfo.assetPath);
            return NauMakeError("Prim {} is invalid!", metaInfo.assetPath);
        }

        if (auto usdAnimation = loadUsdAnimation(primToCompile, outputPath, metaInfo))
        {
            AssetMetaInfo nanimMeta;
            const std::string relativeSourcePath = FileSystemExtensions::getRelativeAssetPath(metaInfo.assetPath, false).string();
            const std::string sourcePath = std::format("{}+[kfanimation]", relativeSourcePath.c_str(), primToCompile.GetName().GetString());
            
            auto id = dbManager.findIf(sourcePath); 
            
            if (id.isError())
            {
                id = Uid::generate();
            }

            const std::filesystem::path out = std::filesystem::path(outputPath) / std::to_string(folderIndex);
            std::string fileName = toString(*id) + ".nanim";

            nanimMeta.uid = *id;
            nanimMeta.dbPath = (out.filename() / fileName).string().c_str();
            nanimMeta.sourcePath = sourcePath.c_str();
            nanimMeta.kind = "Animation";

            auto outFilePath = utils::compilers::ensureOutputPath(outputPath, nanimMeta, "");

            if (saveAnimAssetBlk(*usdAnimation, outFilePath.string()))
            {
                dbManager.addOrReplace(nanimMeta);

                for (auto& track : usdAnimation->tracks)
                {
                    const std::string trackSourcePath = std::format("{}+[kfanimation:{}]", 
                        relativeSourcePath.c_str(), 
                        track->descriptor.name.c_str());

                    auto trackId = dbManager.findIf(trackSourcePath);

                    if (trackId.isError())
                    {
                        trackId = Uid::generate();
                    }

                    AssetMetaInfo trackMeta;
                    trackMeta.uid = *trackId;
                    trackMeta.dbPath = nanimMeta.dbPath; // the same file for all tracks
                    trackMeta.sourcePath = trackSourcePath.c_str();
                    dbManager.addOrReplace(trackMeta);
                }

                return nanimMeta;
            }

            return NauMakeError("Failed to save animation asset");
        }

        return NauMakeError("Animation asset loading failed");
    }

    nau::Result<AssetMetaInfo> UsdSkeletalAnimationCompiler::compile(
        PXR_NS::UsdStageRefPtr stage,
        const std::string& outputPath,
        const std::string& projectRootPath,
        const nau::UsdMetaInfo& metaInfo,
        int folderIndex)
    {
        AssetDatabaseManager& dbManager = AssetDatabaseManager::instance();
        NAU_ASSERT(dbManager.isLoaded(), "Asset database not loaded!");

        auto stageToCompile = pxr::UsdStage::Open(metaInfo.assetPath);

        if (!stageToCompile)
        {
            return NauMakeError("Can't load source stage from '{}'", metaInfo.assetPath);
        }

        auto extraInfo = reinterpret_cast<ExtraInfoAnimation*>(metaInfo.extraInfo.get());

        if (!extraInfo)
        {
            return NauMakeError("Empty extra info for asset '{}'", metaInfo.assetPath);
        }

        pxr::SdfFileFormat::FileFormatArguments exportArguments;
        exportArguments["embedImages"] = false;
        exportArguments["useMaterialExtensions"] = false;

        namespace fs = std::filesystem;
        const fs::path targetDirectoryPath = fs::path(outputPath) / std::to_string(folderIndex);
        auto tmpDirPath = ensureTempDirectory(targetDirectoryPath, true);

        std::string intermediateGltfPath = (tmpDirPath / toString(metaInfo.uid)).string() + ".gltf";

        if (!stageToCompile->Export(intermediateGltfPath, true, exportArguments))
        {
            return NauMakeError("Failed to save skeletal animation gltf {}", intermediateGltfPath);
        }

        auto primToCompile = stageToCompile->GetPrimAtPath(pxr::SdfPath{ extraInfo->source });

        if (!primToCompile)
        {
            primToCompile = stageToCompile->GetDefaultPrim();
        }

        if (auto bindsResult = getBindsDataFromPrim(primToCompile))
        {
            return exportSkeletalGltf2Nanim(outputPath, metaInfo.assetPath, intermediateGltfPath, folderIndex, false, *bindsResult);
        }

        return NauMakeError("Failed to get bind matrices");
    }

    nau::Result<AssetMetaInfo> GltfSkeletalAnimationCompiler::compile(
        PXR_NS::UsdStageRefPtr stage,
        const std::string& outputPath,
        const std::string& projectRootPath,
        const nau::UsdMetaInfo& metaInfo,
        int folderIndex)
    {
        SkeletalAnimationAssetData emptyBindsData;

        return exportSkeletalGltf2Nanim(outputPath, metaInfo.assetPath, metaInfo.assetPath, folderIndex, true, emptyBindsData);
    }

}  // namespace nau::compilers
