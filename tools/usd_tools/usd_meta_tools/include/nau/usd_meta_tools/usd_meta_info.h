// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/usd_meta_tools/usd_meta_api.h"
#include "nau/usd_meta_tools/usd_meta_generator.h"

#include "nau/platform/windows/utils/uid.h"

namespace nau
{
    using UsdMetaInfoArray = std::vector<struct UsdMetaInfo>;

    struct USD_META_TOOLS_API IExtraInfo
    {
        virtual const std::string& getType() const = 0;
        virtual ~IExtraInfo() {}
    };
    using ExtraInfoPtr = std::shared_ptr<IExtraInfo>;


    template<class TTypeInfo>
    struct ExtraInfoTyped : public IExtraInfo
    {
        static const std::string& getTypeName()
        {
            static TTypeInfo info;
            return info.typeName;
        }

        const std::string& getType() const override
        {
            return getTypeName();
        }
    };

    struct UsdMetaInfo
    {
        bool isValid = false;
        Uid uid;
        std::string name;
        std::string type = "undefined";
        std::string metaSourcePath;
        std::string assetPath;
        std::string assetSourcePath;

        ExtraInfoPtr extraInfo;
        UsdMetaInfoArray children;

        template<class ExtraInfoType>
        ExtraInfoType* getExtraInfoAs() const
        {
            if (!extraInfo || ExtraInfoType::getTypeName() != extraInfo->getType())
            {
                return nullptr;
            }
            return reinterpret_cast<ExtraInfoType*>(extraInfo.get());
        }
    };

#define EXTRA_INFO_OBJECT(className)\
    struct TypeInfo##className                    \
    {                                             \
        const std::string typeName = #className;  \
    };                                            \
    struct USD_META_TOOLS_API className final : public ExtraInfoTyped<TypeInfo##className>
   
       
    // default extra info
    EXTRA_INFO_OBJECT(ExtraInfoMesh)
    {
        enum class UpAxis{ X, Y, Z};

        UpAxis upAxis = UpAxis::Y;
        float unitScale = 1.0;
        bool ignoreAnimation = false;
        bool generateLods = false;
        bool generateCollider = false;
        bool generateTangents = true;
        bool flipU = false;
        bool flipV = false;
        bool skinned = false;
        std::string meshSource;
        std::string skeletonSource;
    };

    EXTRA_INFO_OBJECT(ExtraInfoGroup)
    {
    };

    EXTRA_INFO_OBJECT(ExtraInfoTexture)
    {
        std::string path;
    };

    EXTRA_INFO_OBJECT(ExtraInfoMaterial)
    {
        struct ConfigItem
        {
            PXR_NS::VtArray<std::string> shaders;
            MetaArgs properties;

            std::optional<bool> isScissorsEnabled;
            std::optional<std::string> cullMode;
            std::optional<std::string> blendMode;
            std::optional<std::string> depthMode;
            std::optional<std::string> stencilCmpFunc;
        };

        std::map<std::string, ConfigItem> configs;
    };

    EXTRA_INFO_OBJECT(ExtraInfoShader)
    {
        struct InputItem
        {
            std::string type;
            int bufferIndex = 0;
        };

        struct InputLayout
        {
            std::string stream;
            std::map<std::string, InputItem> items;
        };

        struct Configs
        {
            std::string entryPoint;
            std::string target;
            std::vector<std::string> defines;
            std::string inputLayout;
        };

        std::string path;
        std::map<std::string, InputLayout> layouts;
        std::map<std::string, Configs> configs;
    };

    EXTRA_INFO_OBJECT(ExtraInfoSound)
    {
        std::string path;
    };

    EXTRA_INFO_OBJECT(ExtraInfoVFX)
    {
        std::string path;
    };

    EXTRA_INFO_OBJECT(ExtraInfoInput)
    {
        std::string path;
    };

    EXTRA_INFO_OBJECT(ExtraInfoVideo)
    {
        std::string path;
    };

    EXTRA_INFO_OBJECT(ExtraInfoUI)
    {
        std::string path;
    };
    EXTRA_INFO_OBJECT(ExtraInfoFont)
    {
        std::string path;
    };
    EXTRA_INFO_OBJECT(ExtraInfoScene)
    {
        std::string path;
    };
    EXTRA_INFO_OBJECT(ExtraInfoAnimation)
    {
        std::string path;

        std::string source;
    };
    EXTRA_INFO_OBJECT(ExtraInfoGltf)
    {
        std::string path;
    };
}