// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include "frustumClusters.h"
#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_resId.h"
#include "nau/3d/dag_resPtr.h"
#include "nau/render/omniLightsManager.h"
#include "nau/render/spotLightsManager.h"
// #include <render/lights/tiledLights.h>

#include <EASTL/array.h>
#include <EASTL/bitset.h>
#include <EASTL/fixed_function.h>
namespace nau::render
{
    class ShadowSystem;
    class DistanceReadbackLights;

    /* TODO: Light shadows
    class DynamicShadowRenderGPUObjects
    {
    };
    */

    struct ClusteredLights
    {
        typedef OmniLightsManager::RawLight OmniLight;
        typedef SpotLightsManager::RawLight SpotLight;
        static const int MAX_SHADOW_PRIORITY = 15;  // 15 times more imprtant than anything else
        ClusteredLights();
        ~ClusteredLights();

        // initial_frame_light_count is total visible lights for frame. In 32 (words)
        // shadows_quality is size of dynmic shadow map. 0 means no shadows
        async::Task<> init(int initial_frame_light_count, uint32_t shadows_quality, bool use_tiled_lights);
        void setMaxClusteredDist(const float max_clustered_dist);

        /* TODO: Light shadows
        void changeShadowResolution(uint32_t shadows_quality, bool dynamic_shadow_32bit);
        */
        void close();

        void cullFrustumLights(math::Point3 cur_view_pos,
                               const math::Matrix4& globtm,
                               const math::Matrix4& view,
                               const math::Matrix4& proj,
                               float znear,
                               Occlusion* occlusion = nullptr,
                               SpotLightsManager::mask_type_t spot_light_mask = SpotLightsManager::MASK_ALL,
                               OmniLightsManager::mask_type_t omni_light_mask = OmniLightsManager::MASK_ALL);

        /* TODO: support tiledLight
        void prepareTiledLights()
        {
            if (tiledLights)
                tiledLights->computeTiledLigths();
        }
         */
        void toggleTiledLights(bool use_tiled);
        bool hasDeferredLights() const
        {
            return (renderFarOmniLights.size() + renderFarSpotLights.size()) != 0;
        }
        bool hasDebugLights() const
        {
            return (debugOmniLights.size() + debugSpotLights.size()) != 0;
        }
        bool hasClusteredLights() const
        {
            return (clustersOmniGrid.size() + clustersSpotGrid.size()) != 0;
        }
        int getVisibleNotClusteredSpotsCount() const
        {
            return renderFarSpotLights.size();
        }
        int getVisibleNotClusteredOmniCount() const
        {
            return renderFarOmniLights.size();
        }
        int getVisibleClusteredSpotsCount() const
        {
            return renderSpotLights.size();
        }  // fixme
        int getVisibleClusteredOmniCount() const
        {
            return renderOmniLights.size();
        }
        int getVisibleSpotsCount() const
        {
            return getVisibleClusteredSpotsCount() + getVisibleNotClusteredSpotsCount();
        }
        int getVisibleOmniCount() const
        {
            return getVisibleClusteredOmniCount() + getVisibleNotClusteredOmniCount();
        }
        void cullOutOfFrustumLights(const math::Matrix4& globtm,
                                    SpotLightsManager::mask_type_t spot_light_mask,
                                    OmniLightsManager::mask_type_t omni_light_mask);  // cull without any grid

        /* TODO: Light shadows
        void setShadowBias(float z_bias,
                           float slope_z_bias,
                           float shader_z_bias,
                           float shader_slope_z_bias);
        void getShadowBias(float& z_bias,
                           float& slope_z_bias,
                           float& shader_z_bias,
                           float& shader_slope_z_bias) const;
        */

        void renderOtherLights();
        void setBuffersToShader();
        void setOutOfFrustumLightsToShader();
        void setInsideOfFrustumLightsToShader();

        void renderDebugLights();
        void renderDebugLightsBboxes();
        void drawDebugClusters(int slice);

        void destroyLight(uint32_t id);
        uint32_t addOmniLight(const OmniLight& light, OmniLightsManager::mask_type_t mask = OmniLightsManager::GI_LIGHT_MASK);
        void setLight(uint32_t id, const OmniLight& light, bool invalidate_shadow = true);
        void setLightWithMask(uint32_t id, const OmniLight& light, OmniLightsManager::mask_type_t mask, bool invalidate_shadow);
        OmniLight getOmniLight(uint32_t id) const;

        void setLight(uint32_t id_, const SpotLight& light, SpotLightsManager::mask_type_t mask, bool invalidate_shadow = true);
        void setLight(uint32_t id_, const SpotLight& light, bool invalidate_shadow = true);
        uint32_t addSpotLight(const SpotLight& light, SpotLightsManager::mask_type_t mask = SpotLightsManager::GI_LIGHT_MASK);
        SpotLight getSpotLight(uint32_t id_) const;

        bool isLightVisible(uint32_t id) const;

        // priority - the higher, the better. keep in mind, that with very high value you can steal all updates from other volumes

        // hint_dyamic (not cache static) - light is typically moving, and so will be rendered each frame. Makes sense only
        // only_static_casters == false. only_static_casters - light will not cast shadows from dynamic objects quality - the higher the
        // better. It is the speed of going from lowest mip (min_shadow_size) to high mip (max_shadow_size>>shadow_size_srl).
        //  shadow_size_srl - maximum size degradation (shft right bits count for max shadow. If shadow is 256 maximum, and srl is 2, than
        //  maximum size will be 64)
        /* TODO: Light shadows
        bool addShadowToLight(uint32_t id,
                              bool only_static_casters,
                              bool hint_dynamic,
                              uint16_t quality,
                              uint8_t priority,
                              uint8_t shadow_size_srl,
                              DynamicShadowRenderGPUObjects render_gpu_objects);
        void removeShadow(uint32_t id);
        bool getShadowProperties(uint32_t id,
                                 bool& only_static_casters,
                                 bool& hint_dynamic,
                                 uint16_t& quality,
                                 uint8_t& priority,
                                 uint8_t& shadow_size_srl,
                                 DynamicShadowRenderGPUObjects& render_gpu_objects) const;
        */
        enum
        {
            SPOT_LIGHT_FLAG = (1 << 30),
            INVALID_LIGHT = 0xFFFFFFFF & (~SPOT_LIGHT_FLAG)
        };
        /* TODO: Light shadows
        void invalidateAllShadows();                           //{ lightShadows->invalidateAllVolumes(); }
         */
        void invalidateStaticObjects(const math::BBox3& box);  // invalidate static content within box

        /* TODO: Light shadows
        using StaticRenderCallback = void(const math::Matrix4& globTm,
                                          const math::Matrix4& itm,
                                          DynamicShadowRenderGPUObjects render_gpu_objects);
        using DynamicRenderCallback = void(const math::Matrix4& itm,
                                           const math::Matrix4& view_tm,
                                           const math::Matrix4& proj_tm);
        */

        /* TODO: Light shadows
        void prepareShadows(const math::Point3& viewPos,
                            const math::Matrix4& globtm,
                            float hk,
                            nau::ConstSpan<math::BBox3> dynamicBoxes,
                            eastl::fixed_function<sizeof(void*) * 2, StaticRenderCallback> render_static,
                            eastl::fixed_function<sizeof(void*) * 2, DynamicRenderCallback> render_dynamic);
        void updateShadowBuffers();
       */

        void afterReset();
        void setResolution(uint32_t width, uint32_t height);
        void changeResolution(uint32_t width, uint32_t height);

        /* TODO: Light shadows
        math::BBox3 getActiveShadowVolume() const;
        */

        bool initialized() const
        {
            return lightsInitialized;
        }

        /* TODO: Light shadows
        void setNeedSsss(bool need_ssss);
        void setMaxShadowsToUpdateOnFrame(int max_shadows)
        {
            maxShadowsToUpdateOnFrame = max_shadows;
        }
        */

    protected:
        eastl::vector<uint16_t> visibleSpotLightsId;
        eastl::vector<uint16_t> visibleOmniLightsId;
        eastl::bitset<OmniLightsManager::MAX_LIGHTS> visibleOmniLightsIdSet;
        eastl::bitset<SpotLightsManager::MAX_LIGHTS> visibleSpotLightsIdSet;

        FrustumClusters clusters;  //-V730_NOINIT
        static constexpr int MAX_FRAMES = 2;
        static const float MARK_SMALL_LIGHT_AS_FAR_LIMIT;

        // At least on win7 we have a limit for 64k of cb buffer size
        // But drivers requires to keep cb buffer size under 64k on all platforms.
        // So we limit it for all platforms.
        static constexpr int MAX_VISIBLE_FAR_LIGHTS = 65536 / std::max(sizeof(RenderSpotLight), sizeof(RenderOmniLight));

        static bool reallocate_common(Sbuffer*& buf, uint16_t& size, int target_size_in_constants, const char* stat_name);
        static bool updateConsts(Sbuffer* buf, void* data, int data_size, int elems_count);

        template <int elem_size_in_constants, bool store_elems_count>
        struct ReallocatableConstantBuffer
        {
            bool update(void* data, int data_size)
            {
                NAU_ASSERT(data_size % ELEM_SIZE_IN_BYTES == 0);
                int elems_count = data_size / ELEM_SIZE_IN_BYTES;
                wasWritten = true;
                return updateConsts(buf, data, data_size, store_elems_count ? elems_count : -1);
            }
            void close()
            {
                if (buf)
                {
                    buf->destroy();
                }
                buf = nullptr;
                size = 0;
                wasWritten = false;
            }
            ~ReallocatableConstantBuffer()
            {
                close();
            }
            bool reallocate(int target_size_in_elems, int max_size_in_elems, const char* stat_name)
            {
                wasWritten = false;
                int targetSizeInElems = std::min(target_size_in_elems, max_size_in_elems);
                if (d3d::get_driver_code().is(d3d::metal))  // this is because metal validator. buffer size should match shader code
                    targetSizeInElems = max_size_in_elems;
                int targetSizeInConstants = targetSizeInElems * ELEM_SIZE + (store_elems_count ? 1 : 0);
                if (!targetSizeInConstants || size >= targetSizeInConstants)
                    return true;
                return ClusteredLights::reallocate_common(buf, size, targetSizeInConstants, stat_name);
            }
            Sbuffer* get() const
            {
                NAU_ASSERT(wasWritten || !store_elems_count);
                return buf;
            }

        protected:
            enum
            {
                ELEM_SIZE = elem_size_in_constants,
                ELEM_SIZE_IN_BYTES = ELEM_SIZE * sizeof(math::Vector4)
            };
            Sbuffer* buf = nullptr;
            uint16_t size = 0;  // in constants, i.e. 16 bytes*size is size in bytes
            bool wasWritten = false;
        };

        enum class LightType
        {
            Spot,
            Omni,
            Invalid
        };
        struct DecodedLightId
        {
            LightType type;
            uint32_t id;
        };
        __forceinline static DecodedLightId decode_light_id(uint32_t id)
        {
            if (id == INVALID_LIGHT)
                return {LightType::Invalid, uint32_t(INVALID_LIGHT)};

            if (id & SPOT_LIGHT_FLAG)
            {
                id &= ~SPOT_LIGHT_FLAG;
                return {LightType::Spot, id};
            }
            else
                return {LightType::Omni, id};
        }
        __forceinline static uint32_t encode_light_id(LightType type, uint32_t id)
        {
            if (type == LightType::Spot)
                return id | SPOT_LIGHT_FLAG;
            return id;
        }

        eastl::vector<RenderOmniLight> renderOmniLights, renderFarOmniLights, debugOmniLights;
        eastl::vector<RenderSpotLight> renderSpotLights, renderFarSpotLights, debugSpotLights;
        eastl::vector<math::Matrix4> renderSpotLightsShadows;
        eastl::vector<uint32_t> clustersOmniGrid, clustersSpotGrid;
        eastl::vector<SpotLightsManager::mask_type_t> visibleSpotLightsMasks;
        eastl::vector<OmniLightsManager::mask_type_t> visibleOmniLightsMasks;
        ReallocatableConstantBuffer<sizeof(RenderOmniLight) / 16, true> visibleOmniLightsCB, visibleFarOmniLightsCB;
        ReallocatableConstantBuffer<sizeof(RenderSpotLight) / 16, true> visibleSpotLightsCB, visibleFarSpotLightsCB;

        ReallocatableConstantBuffer<sizeof(RenderOmniLight) / 16, true> debugOmniLightsCB;
        ReallocatableConstantBuffer<sizeof(RenderSpotLight) / 16, true> debugSpotLightsCB;

        ReallocatableConstantBuffer<1, false> commonLightShadowsBufferCB;

        /* TODO: redo Dagor resources
        UniqueBufHolder spotLightSsssShadowDescBuffer;
        UniqueBufHolder visibleSpotLightsMasksSB;
        UniqueBufHolder visibleOmniLightsMasksSB;
         */

        ReallocatableConstantBuffer<sizeof(RenderOmniLight) / 16, true> outOfFrustumOmniLightsCB;
        ReallocatableConstantBuffer<sizeof(RenderSpotLight) / 16, true> outOfFrustumVisibleSpotLightsCB;
        ReallocatableConstantBuffer<1, false> outOfFrustumCommonLightsShadowsCB;

        eastl::array<UniqueBuf, MAX_FRAMES> lightsFullGridCB;
        eastl::array<uint32_t, MAX_FRAMES> currentIndicesSize;
        enum
        {
            NO_CLUSTERED_LIGHTS,
            HAS_CLUSTERED_LIGHTS,
            NOT_INITED
        } gridFrameHasLights = NOT_INITED;
        shaders::UniqueOverrideStateId depthBiasOverrideId;
        shaders::OverrideState depthBiasOverrideState;
        float shaderShadowZBias = 0.001f, shaderShadowSlopeZBias = 0.005f;

        void validateDensity(uint32_t words);
        uint32_t lightsGridFrame = 0, allocatedWords = 0;

        MaterialAssetView::Ptr lightsMat;

        uint32_t v_count = 0, f_count = 0;
        Sbuffer* coneSphereVb = nullptr;
        Sbuffer* coneSphereIb = nullptr;
        static constexpr int INVALID_VOLUME = 0xFFFF;

        OmniLightsManager omniLights;                      //-V730_NOINIT
        SpotLightsManager spotLights;                      //-V730_NOINIT
        float closeSliceDist = 4, maxClusteredDist = 500;  //?
        /* TODO: Light shadows
        int maxShadowsToUpdateOnFrame = 4;                 // quality param
        float maxShadowDist = 120.f;// quality and scene param
        eastl::unique_ptr<ShadowSystem> lightShadows;
        eastl::unique_ptr<DistanceReadbackLights> dstReadbackLights;
        */
        dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS> dynamicSpotLightsShadows;  //-V730_NOINIT
        dag::RelocatableFixedVector<uint16_t, OmniLightsManager::MAX_LIGHTS> dynamicOmniLightsShadows;  //-V730_NOINIT
        eastl::bitset<SpotLightsManager::MAX_LIGHTS> dynamicLightsShadowsVolumeSet;
        bool buffersFilled = false;
        bool lightsInitialized = false;

        /* TODO: Light shadows
        void setSpotLightShadowVolume(int spot_light_id);
        void setOmniLightShadowVolume(int omni_light_id);
        */

        void initClustered(int initial_light_density);
        void clusteredCullLights(const math::Matrix4& view,
                                 const math::Matrix4& proj,
                                 float znear,
                                 float minDist,
                                 float maxDist,
                                 nau::ConstSpan<RenderOmniLight> omni_lights,
                                 nau::ConstSpan<SpotLightsManager::RawLight> spot_lights,
                                 nau::ConstSpan<math::Vector4> spot_light_bounds,
                                 bool use_occlusion,
                                 bool& has_omni,
                                 bool& has_spot,
                                 uint32_t* omni,
                                 uint32_t omni_words,
                                 uint32_t* spot,
                                 uint32_t spot_words);
        bool fillClusteredCB(uint32_t* omni, uint32_t omni_words, uint32_t* spot, uint32_t spot_words);
        void closeOmni();
        void closeSpot();

        /* TODO: Light shadows
        void closeOmniShadows();
        void closeSpotShadows();
        */

        void initConeSphere();
        async::Task<> initLightMaterial();
        void closeLightMaterial();

        void fillBuffers();

        void setBuffers();
        void resetBuffers();
        void renderPrims(MaterialAssetView::Ptr material, const char* pipeline, Sbuffer* replaced_buffer, int inst_count, int vstart, int vcount, int index_start, int fcount);

        /* TODO: support TiledLight
        eastl::unique_ptr<TiledLights> tiledLights;
        */
    };
}  // namespace nau::render