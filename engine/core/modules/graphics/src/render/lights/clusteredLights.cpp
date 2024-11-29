// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "render/lights/clusteredLights.h"

#include "nau/3d/dag_lockSbuffer.h"
#include "nau/shaders/shader_globals.h"
#include "nau/utils/dag_stlqsort.h"

namespace nau::render
{
    using namespace math;

    static constexpr int CLUSTERS_PER_GRID = CLUSTERS_W * CLUSTERS_H * (CLUSTERS_D + 1);  // one more slice so we can sample zero for it,
                                                                                          // instead of branch in shader
    static const uint32_t MAX_SHADOWS_QUALITY = 4u;

    const float ClusteredLights::MARK_SMALL_LIGHT_AS_FAR_LIMIT = 0.03;

    static const char* lights_full_gridVarId = nullptr;
    static const char* omni_lightsVarId = nullptr;
    static const char* spot_lightsVarId = nullptr;
    static const char* common_lights_shadowsVarId = nullptr;

    static const char* omniLightsCountVarId = nullptr;
    static const char* omniLightsWordCountVarId = nullptr;
    static const char* spotLightsCountVarId = nullptr;
    static const char* spotLightsWordCountVarId = nullptr;
    static const char* depthSliceScaleVarId = nullptr;
    static const char* depthSliceBiasVarId = nullptr;
    static const char* shadowAtlasTexelVarId = nullptr;
    static const char* shadowDistScaleVarId = nullptr;
    static const char* shadowDistBiasVarId = nullptr;
    static const char* shadowZBiasVarId = nullptr;
    static const char* shadowSlopeZBiasVarId = nullptr;

    bool equalWithEps(const Vector4& a, const Vector4& b, float eps)
    {
        const Vector4 diff = absPerElem((a - b));
        return (diff.getX() < eps) && (diff.getY() < eps) && (diff.getZ() < eps) && (diff.getW() < eps);
    }

    bool equalWithEps(const float4& a, const float4& b, float eps)
    {
        const Vector4 diff = absPerElem((a - b).toVec4());
        return (diff.getX() < eps) && (diff.getY() < eps) && (diff.getZ() < eps) && (diff.getW() < eps);
    }

    bool isInvaliatingShadowsNeeded(const OmniLight& oldLight, const OmniLight& newLight)
    {
        return !equalWithEps(oldLight.pos_radius, newLight.pos_radius, eastl::numeric_limits<float>::epsilon());
    }

    bool isInvaliatingShadowsNeeded(const SpotLight& oldLight, const SpotLight& newLight)
    {
        return !equalWithEps(oldLight.pos_radius, newLight.pos_radius, eastl::numeric_limits<float>::epsilon()) ||
               !equalWithEps(oldLight.dir_angle, newLight.dir_angle, eastl::numeric_limits<float>::epsilon());
    }

    void ClusteredLights::validateDensity(uint32_t words)
    {
        if (words <= allocatedWords)
            return;
        allocatedWords = words;
        /*
        for (int i = 0; i < lightsFullGridCB.size(); ++i)
        {
            eastl::string name = nau::utils::format("lights_full_grid_{}", i);
            lightsFullGridCB[i].close();
            lightsFullGridCB[i] = dag::create_sbuffer(sizeof(uint32_t),
                                                      CLUSTERS_PER_GRID * allocatedWords,
                                                      SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED,
                                                      0,
                                                      name.c_str());
        }
        */
    }

    void ClusteredLights::initClustered(int initial_light_density)
    {
        uint32_t words = clamp((initial_light_density + 31) / 32, (int)2, (int)(MAX_SPOT_LIGHTS + MAX_OMNI_LIGHTS + 31) / 32);
        validateDensity(words);
        gridFrameHasLights = NOT_INITED;
        lightsGridFrame = 0;

        omniLightsCountVarId = "omniLightsCount";
        omniLightsWordCountVarId = "omniLightsWordCount";
        spotLightsCountVarId = "spotLightsCount";
        spotLightsWordCountVarId = "spotLightsWordCount";
        depthSliceScaleVarId = "depthSliceScale";
        depthSliceBiasVarId = "depthSliceBias";
        shadowAtlasTexelVarId = "shadowAtlasTexel";
        shadowDistScaleVarId = "shadowDistScale";
        shadowDistBiasVarId = "shadowDistBias";
        shadowZBiasVarId = "shadowZBias";
        shadowSlopeZBiasVarId = "shadowSlopeZBias";

        // TODO: maybe use texture with R8 format instead of custom byte packing
        /*
        int spotMaskSizeInBytes = (MAX_SPOT_LIGHTS + 3) / 4;
        int omniMaskSizeInBytes = (MAX_OMNI_LIGHTS + 3) / 4;
        visibleSpotLightsMasksSB = dag::buffers::create_one_frame_sr_structured(sizeof(uint), spotMaskSizeInBytes, "spot_lights_flags");
        visibleOmniLightsMasksSB = dag::buffers::create_one_frame_sr_structured(sizeof(uint), omniMaskSizeInBytes, "omni_lights_flags");
        */
    }

    ClusteredLights::ClusteredLights()
    {
        mem_set_0(currentIndicesSize);
    }

    ClusteredLights::~ClusteredLights()
    {
        close();
    }

    void ClusteredLights::close()
    {
        lightsInitialized = false;
        /* TODO: Light shadows
        dstReadbackLights.reset();
        closeOmniShadows();
        closeSpotShadows();
        lightShadows.reset();
         */
        allocatedWords = 0;

        /* TODO: redo Dagor resources
        spotLightSsssShadowDescBuffer.close();
        visibleSpotLightsMasksSB.close();
        visibleOmniLightsMasksSB.close();
        */

        if (coneSphereVb)
        {
            coneSphereVb->destroy();
        }
        if (coneSphereIb)
        {
            coneSphereIb->destroy();
        }

        closeOmni();
        closeSpot();
        closeLightMaterial();
        shaders::overrides::destroy(depthBiasOverrideId);
    }

    /* TODO: Light shadows
    void ClusteredLights::setShadowBias(float z_bias, float slope_z_bias, float shader_z_bias, float shader_slope_z_bias)
    {
        // todo: move depth bias depends to shader.
        // as, it depends on: wk, resolution, distance
        // however, distance can only be implemented in shader (and it is, but resolution independent)
        depthBiasOverrideState = shaders::OverrideState();
        depthBiasOverrideState.set(shaders::OverrideState::Z_BIAS);
        depthBiasOverrideState.zBias = z_bias;
        depthBiasOverrideState.slopeZBias = slope_z_bias;

        depthBiasOverrideId.reset(shaders::overrides::create(depthBiasOverrideState));

        if (lightShadows)
            lightShadows->setOverrideState(depthBiasOverrideState);

        shaderShadowZBias = shader_z_bias;
        shaderShadowSlopeZBias = shader_slope_z_bias;
    }

    void ClusteredLights::getShadowBias(float& z_bias, float& slope_z_bias, float& shader_z_bias, float& shader_slope_z_bias) const
    {
        z_bias = depthBiasOverrideState.zBias;
        slope_z_bias = depthBiasOverrideState.slopeZBias;
        shader_z_bias = shaderShadowZBias;
        shader_slope_z_bias = shaderShadowSlopeZBias;
    }
    */

    void ClusteredLights::renderOtherLights()  // render regular deferred way (currently - with no shadows)
    {
        if (!hasDeferredLights())
            return;
        // TIME_D3D_PROFILE(deferred_lights);
        setBuffers();

        if (!renderFarOmniLights.empty())
        {
            renderPrims(lightsMat, "Omnilight", visibleFarOmniLightsCB.get(), renderFarOmniLights.size(), 0, v_count, 0, f_count);
        }
        if (!renderFarSpotLights.empty())
        {
            renderPrims(lightsMat, "Spotlight", visibleFarSpotLightsCB.get(), renderFarSpotLights.size(), v_count, 5, f_count * 3,
                        6);
        }
        resetBuffers();
    }

    void ClusteredLights::cullOutOfFrustumLights(const math::Matrix4& globtm,
                                                 SpotLightsManager::mask_type_t spot_light_mask,
                                                 OmniLightsManager::mask_type_t omni_light_mask)
    {
        NAU_ASSERT(lightsInitialized);
        NauFrustum frustum(globtm);
        BBox3 far_box, near_box;
        Vector4 unreachablePlane = Vector4(0, 0, 0, FLT_MAX);

        eastl::vector<uint16_t> visibleFarOmniLightsId, cVisibleOmniLightsId;  // TODO: Use frame allocator
        omniLights.prepare(frustum,
                           visibleFarOmniLightsId,
                           cVisibleOmniLightsId,
                           nullptr,
                           far_box,
                           near_box,
                           unreachablePlane,
                           dynamicOmniLightsShadows,
                           0,
                           Point3(0),
                           omni_light_mask);
        NAU_ASSERT(visibleFarOmniLightsId.size() == 0);
        cVisibleOmniLightsId.resize(std::min<int>(cVisibleOmniLightsId.size(), MAX_OMNI_LIGHTS));

        eastl::vector<uint16_t> visibleFarSpotLightsId, cVisibleSpotLightsId;  // TODO: Use frame allocator
        spotLights.prepare(frustum,
                           visibleFarSpotLightsId,
                           cVisibleSpotLightsId,
                           nullptr,
                           nullptr,
                           far_box,
                           near_box,
                           unreachablePlane,
                           dynamicSpotLightsShadows,
                           spot_light_mask);
        NAU_ASSERT(visibleFarSpotLightsId.size() == 0);
        cVisibleSpotLightsId.resize(std::min<int>(cVisibleSpotLightsId.size(), MAX_SPOT_LIGHTS));

        outOfFrustumCommonLightsShadowsCB.reallocate(1 + cVisibleSpotLightsId.size() * 4 + cVisibleOmniLightsId.size(),
                                                     1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS, "out_of_frustum_common_lights_shadow_data");

        dag::RelocatableFixedVector<Vector4, 1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS> commonShadowData;
        commonShadowData.resize(1 + cVisibleSpotLightsId.size() * 4 + cVisibleOmniLightsId.size());
        commonShadowData[0] = Vector4(cVisibleSpotLightsId.size(), cVisibleOmniLightsId.size(), 4 * cVisibleSpotLightsId.size(), 0);

        outOfFrustumVisibleSpotLightsCB.reallocate(cVisibleSpotLightsId.size(), MAX_SPOT_LIGHTS, "out_of_frustum_spot_lights");
        int baseIndex = 1;
        if (cVisibleSpotLightsId.size())
        {
            eastl::vector<RenderSpotLight> outRenderSpotLights;  // TODO: Use frame allocator
            outRenderSpotLights.resize(cVisibleSpotLightsId.size());
            for (int i = 0; i < cVisibleSpotLightsId.size(); ++i)
                outRenderSpotLights[i] = spotLights.getRenderLight(cVisibleSpotLightsId[i]);
            outOfFrustumVisibleSpotLightsCB.update(outRenderSpotLights.data(), data_size(outRenderSpotLights));
            /* TODO: Light shadows
            for (int i = 0; i < cVisibleSpotLightsId.size(); ++i)
            {
                uint16_t shadowId = dynamicSpotLightsShadows[cVisibleSpotLightsId[i]];
                if (shadowId != INVALID_VOLUME)
                    memcpy(&commonShadowData[baseIndex + i * 4], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Vector4));
                else
                    memset(&commonShadowData[baseIndex + i * 4], 0, 4 * sizeof(Vector4));
            }
            */
        }
        else
        {
            outOfFrustumVisibleSpotLightsCB.update(nullptr, 0);
        }

        outOfFrustumOmniLightsCB.reallocate(cVisibleOmniLightsId.size(), MAX_OMNI_LIGHTS, "out_of_frustum_omni_lights");
        baseIndex += cVisibleSpotLightsId.size() * 4;
        if (cVisibleOmniLightsId.size())
        {
            eastl::vector<OmniLightsManager::RawLight> outRenderOmniLights;  // TODO: Use frame allocator
            outRenderOmniLights.resize(cVisibleOmniLightsId.size());
            /* TODO: Light shadows
            for (int i = 0; i < cVisibleOmniLightsId.size(); ++i)
            {
                outRenderOmniLights[i] = omniLights.getLight(cVisibleOmniLightsId[i]);
                uint16_t shadowId = dynamicOmniLightsShadows[cVisibleOmniLightsId[i]];
                if (shadowId != INVALID_VOLUME)
                    commonShadowData[baseIndex + i] = lightShadows->getOctahedralVolumeTexData(shadowId);
                else
                    memset(&commonShadowData[baseIndex + i], 0, sizeof(Vector4));
            }
            */
            outOfFrustumOmniLightsCB.update(outRenderOmniLights.data(), data_size(outRenderOmniLights));
        }
        else
        {
            outOfFrustumOmniLightsCB.update(nullptr, 0);
        }

        if (!cVisibleSpotLightsId.empty() || !cVisibleOmniLightsId.empty())
        {
            outOfFrustumCommonLightsShadowsCB.update(commonShadowData.data(), data_size(commonShadowData));
        }
        else
        {
            outOfFrustumCommonLightsShadowsCB.update(nullptr, 0);
        }
    }

    // zfar_plane should be normalized and faced towards camera origin. camPlanes[4] in Frustum is of that kind
    // v_plane_dist(zfar_plane, cur_view_pos) - should be positive!
    inline float plane_dist(Vector4 plane, Point3 point)
    {
        return float(dot(plane.getXYZ(), Vector3(point))) + point.getW();
    }

    Vector4 v_perm_xyzd(Vector4 xyzw, Vector4 abcd)
    {
        __m128 zzdd = _mm_shuffle_ps(xyzw.get128(), abcd.get128(), _MM_SHUFFLE(3, 3, 2, 2));
        return Vector4(_mm_shuffle_ps(xyzw.get128(), zzdd, _MM_SHUFFLE(3, 0, 1, 0)));
    }

    inline Vector4 shrink_zfar_plane(Vector4 zfar_plane, Point3 cur_view_pos, Vector4 max_z_far_dist)
    {
        float zfarDist = plane_dist(zfar_plane, cur_view_pos);
        float newZFarDist = std::min<float>(max_z_far_dist.getW(), zfarDist);
        float ofsDist = newZFarDist - zfarDist;
        return v_perm_xyzd(zfar_plane, zfar_plane + Vector4(ofsDist));
    }

    void ClusteredLights::cullFrustumLights(Point3 cur_view_pos,
                                            const Matrix4& globtm,
                                            const Matrix4& view,
                                            const Matrix4& proj,
                                            float znear,
                                            Occlusion* occlusion,
                                            SpotLightsManager::mask_type_t spot_light_mask,
                                            OmniLightsManager::mask_type_t omni_light_mask)
    {
        debugOmniLights.clear();
        debugSpotLights.clear();

        // TIME_PROFILE(cullFrustumLights);
        buffersFilled = false;
        NauFrustum frustum(globtm);
        Vector4 clusteredLastPlane = shrink_zfar_plane(frustum.camPlanes[4], cur_view_pos, Vector4(maxClusteredDist));

        // separate to closer than maxClusteredDist and farther to render others deferred way
        BBox3 far_box, near_box;

        visibleOmniLightsIdSet.reset();
        visibleOmniLightsId.clear();
        eastl::vector<uint16_t> visibleFarOmniLightsId;  // TODO: Use frame allocator
        omniLights.prepare(frustum,
                           visibleOmniLightsId,
                           visibleFarOmniLightsId,
                           &visibleOmniLightsIdSet,
                           occlusion,
                           far_box,
                           near_box,
                           clusteredLastPlane,
                           dynamicOmniLightsShadows,
                           MARK_SMALL_LIGHT_AS_FAR_LIMIT,
                           cur_view_pos,
                           omni_light_mask);

        if (visibleOmniLightsId.size() > MAX_OMNI_LIGHTS)
        {
            // Spotlights were always sorted, this is only here to move the farthests ones into the far buffer.
            stlsort::sort(visibleOmniLightsId.begin(), visibleOmniLightsId.end(), [this, &cur_view_pos](uint16_t i, uint16_t j)
            {
                const float distI = lengthSqr(cur_view_pos - Point3(omniLights.getBoundingSphere(i).getXYZ()));
                const float distJ = lengthSqr(cur_view_pos - Point3(omniLights.getBoundingSphere(j).getXYZ()));
                return distI < distJ;
            });
            auto oldFarSize = visibleFarOmniLightsId.size();
            auto excessSize = visibleOmniLightsId.size() - MAX_OMNI_LIGHTS;
            for (int k = MAX_OMNI_LIGHTS; k < MAX_OMNI_LIGHTS + excessSize; ++k)
            {
                visibleFarOmniLightsId.push_back(visibleOmniLightsId[k]);
            }
            NAU_LOG_WARNING("too many omni lights {}, moved {} to Far buffer (before {}, after {})", visibleOmniLightsId.size(), excessSize,
                            oldFarSize, visibleFarOmniLightsId.size());
            G_UNUSED(oldFarSize);
        }
        visibleOmniLightsId.resize(std::min(int(visibleOmniLightsId.size()), int(MAX_OMNI_LIGHTS)));

        visibleSpotLightsIdSet.reset();
        visibleSpotLightsId.clear();
        // spotLights.prepare(frustum, visibleSpotLightsId, occlusion);

        eastl::vector<uint16_t> visibleFarSpotLightsId;  // TODO: Use frame allocator
        spotLights.prepare(frustum,
                           visibleSpotLightsId,
                           visibleFarSpotLightsId,
                           &visibleSpotLightsIdSet,
                           occlusion,
                           far_box,
                           near_box,
                           clusteredLastPlane,
                           dynamicSpotLightsShadows,
                           MARK_SMALL_LIGHT_AS_FAR_LIMIT,
                           cur_view_pos,
                           spot_light_mask);

        stlsort::sort(visibleSpotLightsId.begin(), visibleSpotLightsId.end(), [this, &cur_view_pos](uint16_t i, uint16_t j)
        {
            const float distI = lengthSqr(cur_view_pos - Point3(spotLights.getBoundingSphere(i).getXYZ()));
            const float distJ = lengthSqr(cur_view_pos - Point3(spotLights.getBoundingSphere(j).getXYZ()));
            return distI < distJ;
        });
        // separate close and far lights cb (so we can render more far lights easier)
        if (visibleSpotLightsId.size() > MAX_SPOT_LIGHTS)
        {
            auto oldFarSize = visibleFarSpotLightsId.size();
            auto excessSize = visibleSpotLightsId.size() - MAX_SPOT_LIGHTS;
            for (int k = MAX_OMNI_LIGHTS; k < MAX_OMNI_LIGHTS + excessSize; ++k)
            {
                visibleFarSpotLightsId.push_back(visibleSpotLightsId[k]);
            }
            NAU_LOG_WARNING("too many spot lights {}, moved {} to Far buffer (before {}, after {})", visibleSpotLightsId.size(), excessSize,
                            oldFarSize, visibleFarSpotLightsId.size());
            G_UNUSED(oldFarSize);
        }
        visibleSpotLightsId.resize(std::min<int>(visibleSpotLightsId.size(), MAX_SPOT_LIGHTS));

        eastl::vector<SpotLightsManager::RawLight> visibleSpotLights;  // TODO: Use frame allocator
        eastl::vector<Vector4> visibleSpotLightsBounds;                // TODO: Use frame allocator
        eastl::vector<Vector4> visibleOmniLightsBounds;                // TODO: Use frame allocator
        visibleSpotLights.resize(visibleSpotLightsId.size());
        renderSpotLights.resize(visibleSpotLightsId.size());
        renderOmniLights.resize(visibleOmniLightsId.size());
        visibleSpotLightsBounds.resize(visibleSpotLightsId.size());
        visibleSpotLightsMasks.resize(visibleSpotLightsId.size());
        visibleOmniLightsMasks.resize(visibleOmniLightsId.size());
        visibleOmniLightsBounds.resize(renderOmniLights.size());
        for (int i = 0, e = visibleSpotLightsId.size(); i < e; ++i)
        {
            uint32_t id = visibleSpotLightsId[i];
            visibleSpotLightsBounds[i] = spotLights.getBoundingSphere(id);
            visibleSpotLights[i] = spotLights.getLight(id);
            renderSpotLights[i] = spotLights.getRenderLight(id);
            visibleSpotLightsMasks[i] = spotLights.getLightMask(id);

            if(spotLights.getLight(id).debugDraw)
            {
                debugSpotLights.push_back(spotLights.getRenderLight(id));
            }
        }
        for (int i = 0; i < visibleOmniLightsId.size(); ++i)
        {
            uint32_t id = visibleOmniLightsId[i];
            renderOmniLights[i] = omniLights.getRenderLight(id);
            visibleOmniLightsBounds[i] = Vector4(_mm_loadu_ps(reinterpret_cast<float*>(&renderOmniLights[i].posRadius)));
            visibleOmniLightsMasks[i] = omniLights.getLightMask(id);

            if(omniLights.getLight(id).debugDraw)
            {
                debugOmniLights.push_back(omniLights.getRenderLight(id));
            }
        }

        visibleFarSpotLightsId.resize(std::min<int>(visibleFarSpotLightsId.size(), MAX_VISIBLE_FAR_LIGHTS));
        renderFarSpotLights.resize(visibleFarSpotLightsId.size());
        for (int i = 0, e = visibleFarSpotLightsId.size(); i < e; ++i)
        {
            renderFarSpotLights[i] = spotLights.getRenderLight(visibleFarSpotLightsId[i]);
            if(spotLights.getLight(visibleFarSpotLightsId[i]).debugDraw)
            {
                debugSpotLights.push_back(spotLights.getRenderLight(visibleFarSpotLightsId[i]));
            }
        }

        visibleFarOmniLightsId.resize(std::min<int>(visibleFarOmniLightsId.size(), MAX_VISIBLE_FAR_LIGHTS));
        renderFarOmniLights.resize(visibleFarOmniLightsId.size());
        for (int i = 0, e = visibleFarOmniLightsId.size(); i < e; ++i)
        {
            renderFarOmniLights[i] = omniLights.getRenderLight(visibleFarOmniLightsId[i]);
            if(omniLights.getLight(visibleFarOmniLightsId[i]).debugDraw)
            {
                debugOmniLights.push_back(omniLights.getRenderLight(visibleFarOmniLightsId[i]));
            }
        }

        // clusteredCullLights(view, proj, znear, 1, 500, (Vector4*)visibleOmniLights.data(),
        //   elem_size(visibleOmniLights)/sizeof(Vector4), visibleOmniLights.size(), 2);
        uint32_t omniWords = (renderOmniLights.size() + 31) / 32, spotWords = (visibleSpotLights.size() + 31) / 32;
        clustersOmniGrid.resize(CLUSTERS_PER_GRID * omniWords);
        clustersSpotGrid.resize(CLUSTERS_PER_GRID * spotWords);
        if (clustersOmniGrid.size() || clustersSpotGrid.size())
        {
            bool nextGridHasOmniLights = clustersOmniGrid.size() != 0, nextGridHasSpotLights = clustersSpotGrid.size() != 0;
            mem_set_0(clustersOmniGrid);
            mem_set_0(clustersSpotGrid);
            uint32_t *omniMask = clustersOmniGrid.data(), *spotMask = clustersSpotGrid.data();
            clusteredCullLights(view,
                                proj,
                                znear,
                                closeSliceDist,
                                maxClusteredDist,
                                renderOmniLights,
                                visibleSpotLights,
                                visibleSpotLightsBounds,
                                occlusion ? true : false,
                                nextGridHasOmniLights,
                                nextGridHasSpotLights,
                                omniMask,
                                omniWords,
                                spotMask,
                                spotWords);
            if (!nextGridHasOmniLights)
            {
                clustersOmniGrid.resize(0);
                renderOmniLights.resize(0);
                visibleOmniLightsBounds.resize(0);
            }
            if (!nextGridHasSpotLights)
            {
                clustersSpotGrid.resize(0);
                renderSpotLights.resize(0);
                visibleSpotLightsBounds.resize(0);
                // visibleSpotLightsMasks.resize(0);
            }
        }

        /* TODO: Support tiledLights
        if (tiledLights)
        {
            mat44f invView;
            v_mat44_orthonormal_inverse43(invView, view);
            Vector4 cur_view_dir = invView.col2;
            tiledLights->prepare(visibleOmniLightsBounds, visibleSpotLightsBounds, cur_view_pos, cur_view_dir);
        }
        */
    }

    template <typename V, typename T = typename V::value_type>
    inline constexpr uint32_t elem_size(const V&)
    {
        return (uint32_t)sizeof(T);
    }

    void ClusteredLights::fillBuffers()
    {
        if (buffersFilled)
            return;
        buffersFilled = true;
        uint32_t omniWords = clustersOmniGrid.size() / CLUSTERS_PER_GRID, spotWords = clustersSpotGrid.size() / CLUSTERS_PER_GRID;
        if ((clustersOmniGrid.size() || clustersSpotGrid.size()) || gridFrameHasLights != NO_CLUSTERED_LIGHTS)  // todo: only update if
                                                                                                                // something changed (which
                                                                                                                // won't happen very often)
        {
            NAU_ASSERT(omniWords == (renderOmniLights.size() + 31) / 32);
            NAU_ASSERT(spotWords == (renderSpotLights.size() + 31) / 32);

            //shader_globals::setVariable<int>(omniLightsCountVarId, renderOmniLights.size()).ignore();
            //shader_globals::setVariable<int>(omniLightsWordCountVarId, omniWords).ignore();
            //shader_globals::setVariable<int>(spotLightsCountVarId, renderSpotLights.size()).ignore();
            //shader_globals::setVariable<int>(spotLightsWordCountVarId, spotWords).ignore();
            //shader_globals::setVariable<float>(depthSliceScaleVarId, clusters.depthSliceScale).ignore();
            //shader_globals::setVariable<float>(depthSliceBiasVarId, clusters.depthSliceBias).ignore();

            /* TODO: Light shadows
            shader_globals::setVariable<Vector4>(shadowAtlasTexelVarId, Vector4(
                                                                            lightShadows ? 1.f / lightShadows->getAtlasWidth() : 1,
                                                                            lightShadows ? 1.f / lightShadows->getAtlasHeight() : 1,
                                                                            0.f,
                                                                            0.f));


            const float maxShadowDistUse = std::min(maxShadowDist, maxClusteredDist * 0.9f);
            const float shadowScale = 1 / (maxShadowDistUse * 0.95 - maxShadowDistUse);  // last 5% of distance are used for disappearing of
                                                                                         // shadows
            const float shadowBias = -shadowScale * maxShadowDistUse;

            shader_globals::setVariable<float>(shadowDistScaleVarId, shadowScale);
            shader_globals::setVariable<float>(shadowDistBiasVarId, shadowBias);
            shader_globals::setVariable<float>(shadowZBiasVarId, shaderShadowZBias);
            shader_globals::setVariable<float>(shadowSlopeZBiasVarId, shaderShadowSlopeZBias);
            */
        }
        gridFrameHasLights = (clustersOmniGrid.size() || clustersSpotGrid.size()) ? HAS_CLUSTERED_LIGHTS : NO_CLUSTERED_LIGHTS;

        NAU_ASSERT(elem_size(renderOmniLights) % sizeof(Vector4) == 0);
        visibleOmniLightsCB.reallocate(renderOmniLights.size(), MAX_OMNI_LIGHTS, "omni_lights");
        visibleOmniLightsCB.update(renderOmniLights.data(), data_size(renderOmniLights));

        /* TODO: global buffers
        shader_globals::set_buffer(omni_lightsVarId, visibleOmniLightsCB.getId());
        */
        /* TODO:  redo Dagor resources
        if (renderOmniLights.size())  // todo: only update if something changed (which won't happen very often)
        {
            NAU_ASSERT(visibleOmniLightsMasks.size() <= MAX_OMNI_LIGHTS);
            visibleOmniLightsMasksSB.getBuf()->updateDataWithLock(0, data_size(visibleOmniLightsMasks), visibleOmniLightsMasks.data(),
                                                                  VBLOCK_DISCARD);
        }
         */
        if (gridFrameHasLights == HAS_CLUSTERED_LIGHTS)
            fillClusteredCB(clustersOmniGrid.data(), omniWords, clustersSpotGrid.data(), spotWords);

        visibleSpotLightsCB.reallocate(renderSpotLights.size(), MAX_SPOT_LIGHTS, "spot_lights");
        visibleSpotLightsCB.update(renderSpotLights.data(), data_size(renderSpotLights));

        /* TODO: global buffers
        shader_globals::set_buffer(spot_lightsVarId, visibleSpotLightsCB.getId());
        */
        /* TODO: redo Dagor resources
        if (renderSpotLights.size())  // todo: only update if something changed (which won't happen very often)
        {
            // do that only when needed
            NAU_ASSERT(visibleSpotLightsMasks.size() <= MAX_SPOT_LIGHTS);
            visibleSpotLightsMasksSB.getBuf()->updateDataWithLock(0, data_size(visibleSpotLightsMasks), visibleSpotLightsMasks.data(),
                                                                  VBLOCK_DISCARD);
        }
        w*/
        // todo: only update if something changed (which won't happen very often)
        visibleFarSpotLightsCB.reallocate(renderFarSpotLights.size(), MAX_VISIBLE_FAR_LIGHTS, "far_spot_lights");
        visibleFarSpotLightsCB.update(renderFarSpotLights.data(), data_size(renderFarSpotLights));
        visibleFarOmniLightsCB.reallocate(renderFarOmniLights.size(), MAX_VISIBLE_FAR_LIGHTS, "far_omni_lights");
        visibleFarOmniLightsCB.update(renderFarOmniLights.data(), data_size(renderFarOmniLights));

        /* TODO: support TiledLight
        if (tiledLights)
            tiledLights->fillBuffers();
        */
    }

    void ClusteredLights::clusteredCullLights(const math::Matrix4& view,
                                              const math::Matrix4& proj,
                                              float znear,
                                              float minDist,
                                              float maxDist,
                                              nau::ConstSpan<RenderOmniLight> omni_lights,
                                              nau::ConstSpan<SpotLightsManager::RawLight> spot_lights,
                                              nau::ConstSpan<math::Vector4> spot_light_bounds,
                                              bool use_occlusion,
                                              bool& has_omni_lights,
                                              bool& has_spot_lights,
                                              uint32_t* omni_mask,
                                              uint32_t omni_words,
                                              uint32_t* spot_mask,
                                              uint32_t spot_words)
    {
        has_spot_lights = spot_lights.size() != 0;
        has_omni_lights = omni_lights.size() != 0;
        if (!omni_lights.size() && !spot_lights.size())
            return;
        // TIME_D3D_PROFILE(clusteredFill);
        clusters.prepareFrustum(view, proj, znear, minDist, maxDist, use_occlusion);
        eastl::unique_ptr<FrustumClusters::ClusterGridItemMasks> tempOmniItemsPtr, tempSpotItemsPtr;

        tempOmniItemsPtr.reset(new FrustumClusters::ClusterGridItemMasks());

        // TIME_PROFILE(clustered);
        uint32_t clusteredOmniLights = clusters.fillItemsSpheres((const Vector4*)omni_lights.data(), elem_size(omni_lights) / sizeof(Vector4),
                                                                 omni_lights.size(), *tempOmniItemsPtr, omni_mask, omni_words);

        tempOmniItemsPtr.reset();

        uint32_t clusteredSpotLights = 0;
        if (spot_lights.size())
        {
            tempSpotItemsPtr.reset(new FrustumClusters::ClusterGridItemMasks());
            clusteredSpotLights = clusters.fillItemsSpheres(spot_light_bounds.data(),
                                                            elem_size(spot_light_bounds) / sizeof(Vector4),
                                                            spot_lights.size(),
                                                            *tempSpotItemsPtr,
                                                            spot_mask,
                                                            spot_words);
        }

        if (clusteredSpotLights)
        {
            // TIME_PROFILE(cullSpots)
            clusteredSpotLights = clusters.cullSpots(
                (const Vector4*)spot_lights.data(),
                elem_size(spot_lights) / sizeof(Vector4),
                (const Vector4*)&spot_lights[0].dir_angle,
                elem_size(spot_lights) / sizeof(Vector4),
                *tempSpotItemsPtr,
                spot_mask,
                spot_words);
        }
        has_spot_lights = clusteredSpotLights != 0;
        has_omni_lights = clusteredOmniLights != 0;
        if (!clusteredSpotLights && !clusteredOmniLights)
            return;
    }

    bool ClusteredLights::fillClusteredCB(uint32_t* source_omni, uint32_t omni_words, uint32_t* source_spot, uint32_t spot_words)
    {
        validateDensity(spot_words + omni_words);  // ensure there is enough space with size

        lightsGridFrame = (lightsGridFrame + 1) % lightsFullGridCB.size();

        /* TODO: support Shadows
        shader_globals::set_buffer(lights_full_gridVarId, lightsFullGridCB[lightsGridFrame]);
        */

        LockedBuffer<uint32_t> masks =
            lock_sbuffer<uint32_t>(lightsFullGridCB[lightsGridFrame].getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

        if (!masks)
            return false;

        masks.updateDataRange(0, source_omni, omni_words * CLUSTERS_PER_GRID);
        masks.updateDataRange(CLUSTERS_PER_GRID * omni_words, source_spot, spot_words * CLUSTERS_PER_GRID);
        return true;
    }

    void ClusteredLights::setResolution(uint32_t width, uint32_t height)
    {
        /* TODO: support TiledLight
        if (tiledLights)
            {tiledLights->setResolution(width, height);}
        */
    }

    void ClusteredLights::changeResolution(uint32_t width, uint32_t height)
    {
        /* TODO: support TiledLight
        if (tiledLights)
        {
            tiledLights->changeResolution(width, height);
        }
        */
    }

    /* TODO: support TiledLight
    void ClusteredLights::changeShadowResolution(uint32_t shadow_quality, bool dynamic_shadow_32bit)
    {
        if (!lightShadows && shadow_quality > 0)
        {
            dstReadbackLights.reset();
            lightShadows.reset();
            lightShadows = eastl::make_unique<ShadowSystem>();
            lightShadows->setOverrideState(depthBiasOverrideState);
            dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights);
        }

        if (lightShadows)
        {
            shadow_quality = min(shadow_quality, MAX_SHADOWS_QUALITY);
            lightShadows->changeResolution(512 << shadow_quality, 128 << shadow_quality, 16 << shadow_quality, 16 << shadow_quality,
                                           dynamic_shadow_32bit);
            invalidateAllShadows();
        }
    }
    */

    void ClusteredLights::toggleTiledLights(bool use_tiled)
    {
        /* TODO: support TiledLight
        if (!use_tiled)
            tiledLights.reset();
        else if (!tiledLights)
            tiledLights = eastl::make_unique<TiledLights>(maxClusteredDist);
        */
    }

    async::Task<> ClusteredLights::init(int frame_initial_lights_count, uint32_t shadow_quality, bool use_tiled_lights)
    {
        lightsInitialized = true;

        /* TODO: support Shadows
        if (shadow_quality)
        {
            lightShadows = eastl::make_unique<ShadowSystem>();
            lightShadows->setOverrideState(depthBiasOverrideState);
            shadow_quality = min(shadow_quality, MAX_SHADOWS_QUALITY);
            lightShadows->changeResolution(512 << shadow_quality, 128 << shadow_quality, 16 << shadow_quality, 16 << shadow_quality, false);
        }
        setShadowBias(-0.00006f, -0.1f, 0.001f, 0.005f);
        */
        initClustered(frame_initial_lights_count);
        initConeSphere();
        co_await initLightMaterial();

        visibleOmniLightsCB.reallocate(0, MAX_OMNI_LIGHTS, "omni_lights");
        visibleOmniLightsCB.update(nullptr, 0);
        visibleSpotLightsCB.reallocate(0, MAX_SPOT_LIGHTS, "spot_lights");
        visibleSpotLightsCB.update(nullptr, 0);
        debugOmniLightsCB.reallocate(0, MAX_OMNI_LIGHTS, "omni_lights");
        debugOmniLightsCB.update(nullptr, 0);
        debugSpotLightsCB.reallocate(0, MAX_SPOT_LIGHTS, "spot_lights");
        debugSpotLightsCB.update(nullptr, 0);

        /* TODO: support Shadows
        if (lightShadows)
            dstReadbackLights = eastl::make_unique<DistanceReadbackLights>(lightShadows.get(), &spotLights);
        */

        lights_full_gridVarId = "lights_full_grid";
        omni_lightsVarId = "omni_lights";
        spot_lightsVarId = "spot_lights";
        common_lights_shadowsVarId = "common_lights_shadows";

        /* TODO: support tiledLights
        tiledLights.reset();
        if (use_tiled_lights)
            tiledLights = eastl::make_unique<TiledLights>(maxClusteredDist);
        */
        co_return;
    }

    void ClusteredLights::setMaxClusteredDist(const float max_clustered_dist)
    {
        maxClusteredDist = max_clustered_dist;
        /* TODO: support tiledLights
        if (tiledLights)
            tiledLights->setMaxLightsDist(maxClusteredDist);
        */
    }

    void ClusteredLights::closeOmni()
    {
        visibleOmniLightsCB.close();
        visibleFarOmniLightsCB.close();
        //pointLightsElem = NULL;
        // del_it(pointLightsMat);
    }

    /* TODO: support Shadows
    void ClusteredLights::closeOmniShadows()
    {
        if (!lightShadows)
            return;
        for (uint16_t& shadowIdx : dynamicOmniLightsShadows)
        {
            if (shadowIdx != INVALID_VOLUME)
            {
                lightShadows->destroyVolume(shadowIdx);
                shadowIdx = INVALID_VOLUME;
            }
        }
    }
    */

    /* TODO: support Shadows
    void ClusteredLights::closeSpotShadows()
    {
        if (!lightShadows)
            return;
        for (uint16_t& shadowIdx : dynamicSpotLightsShadows)
        {
            if (shadowIdx != INVALID_VOLUME)
            {
                lightShadows->destroyVolume(shadowIdx);
                shadowIdx = INVALID_VOLUME;
            }
        }
    }
    */

    void ClusteredLights::closeSpot()
    {
        visibleSpotLightsCB.close();
        visibleFarSpotLightsCB.close();
        commonLightShadowsBufferCB.close();
        //spotLightsElem = NULL;
        // del_it(spotLightsMat);
    }

    void calc_sphere_vertex_face_count(uint32_t slices, uint32_t stacks, bool /*hemisphere*/, uint32_t& out_vertex_count, uint32_t& out_face_count)
    {
        out_face_count = 2 * (stacks) * (slices);
        out_vertex_count = (stacks + 1) * (slices + 1);
    }

#define MAX_SPHERE_SLICES 64
#define MAX_SPHERE_STACKS 64

    void sincos(float rad, float& s, float& c)
    {
        Vector4 vs, vc;
        sseSinfCosf(Vector4(rad).get128(), &vs.get128(), &vc.get128());
        s = vs.getX();
        c = vc.getX();
    }

    void create_sphere_mesh(nau::Span<uint8_t> pVertex,
                            nau::Span<uint8_t> pwFace,
                            float radius,
                            uint32_t slices,
                            uint32_t stacks,
                            uint32_t stride,
                            bool norm,
                            bool tex,
                            bool use_32_instead_of_16_indices,
                            bool hemisphere)
    {
        NAU_ASSERT(stacks >= 2 && stacks <= MAX_SPHERE_STACKS);
        NAU_ASSERT(slices >= 2 && slices <= MAX_SPHERE_SLICES);

        uint32_t i, j;

        // Sin/Cos caches
        eastl::array<float, MAX_SPHERE_SLICES + 1> sinI, cosI;
        eastl::array<float, MAX_SPHERE_STACKS + 1> sinJ, cosJ;

        for (i = 0; i <= slices; i++)
        {
            sincos(2.0f * PI * i / slices, sinI[i], cosI[i]);
        }

        for (j = 0; j <= stacks; j++)
        {
            sincos((hemisphere ? PI / 2 : PI) * j / (hemisphere ? stacks - 1 : stacks), sinJ[j], cosJ[j]);
        }

        // Generate vertices
        uint32_t vert = 0;
        uint32_t texOffset = norm ? 24 : 12;

#define SET_POS(a, b)                            \
    do                                           \
    {                                            \
        (*((Point3*)(&a[vert * stride])) = (b)); \
    } while (0)
#define SET_NORM(a, b)                                                \
    do                                                                \
    {                                                                 \
        if (norm)                                                     \
            (*((Point3*)(&a[vert * stride] + sizeof(Point3))) = (b)); \
    } while (0)
#define SET_TEX_U(a, b)                                         \
    do                                                          \
    {                                                           \
        if (tex)                                                \
            (*((float*)(&a[vert * stride] + texOffset)) = (b)); \
    } while (0)
#define SET_TEX_V(a, b)                                             \
    do                                                              \
    {                                                               \
        if (tex)                                                    \
            (*((float*)(&a[vert * stride] + texOffset + 4)) = (b)); \
    } while (0)

        // Stacks
        for (j = 0; j <= stacks; j++)
        {
            for (i = 0; i <= slices; i++)
            {
                Point3 curNorm = Point3(sinJ[j] * cosI[i], cosJ[j], sinJ[j] * sinI[i]);

                SET_POS(pVertex, scale(curNorm, radius));
                SET_NORM(pVertex, curNorm);
                SET_TEX_U(pVertex, i / (float)slices);
                SET_TEX_V(pVertex, j / (float)stacks);

                vert++;
            }
        }
        // Generate indices
        uint32_t ind = 0;
        uint32_t indSize = use_32_instead_of_16_indices ? sizeof(uint32_t) : sizeof(uint16_t);

#define SET_IND(a, b, c)                                 \
    if (use_32_instead_of_16_indices)                    \
        (*((uint32_t*)(&a[(ind + b) * indSize])) = (c)); \
    else                                                 \
        (*((uint16_t*)(&a[(ind + b) * indSize])) = (c));

        for (int v = 0; v < stacks; ++v)
        {
            for (int h = 0; h < slices; ++h)
            {
                short lt = (short)(h + v * (slices + 1));
                short rt = (short)((h + 1) + v * (slices + 1));

                short lb = (short)(h + (v + 1) * (slices + 1));
                short rb = (short)((h + 1) + (v + 1) * (slices + 1));

                SET_IND(pwFace, 0, lt);
                SET_IND(pwFace, 1, rt);
                SET_IND(pwFace, 2, lb);

                SET_IND(pwFace, 3, rt);
                SET_IND(pwFace, 4, rb);
                SET_IND(pwFace, 5, lb);

                ind += 6;
            }
        }
#undef SET_IND
    }

    void ClusteredLights::initConeSphere()
    {
        static constexpr uint32_t SLICES = 5;
        calc_sphere_vertex_face_count(SLICES, SLICES, false, v_count, f_count);
        if (coneSphereVb)
        {
            coneSphereVb->destroy();
        }
        coneSphereVb = d3d::create_vb((v_count + 5) * sizeof(Point3), 0, u8"coneSphereVb");
        d3d_err((bool)coneSphereVb);
        if (coneSphereIb)
        {
            coneSphereIb->destroy();
        }
        coneSphereIb = d3d::create_ib((f_count + 6) * 6, 0, u8"coneSphereIb");
        d3d_err((bool)coneSphereIb);

        LockedBuffer<uint16_t> indicesLocked = lock_sbuffer<uint16_t>(coneSphereIb, 0, 0, VBLOCK_WRITEONLY);
        if (!indicesLocked)
            return;
        uint16_t* indices = indicesLocked.get();
        LockedBuffer<Point3> verticesLocked = lock_sbuffer<Point3>(coneSphereVb, 0, 0, VBLOCK_WRITEONLY);
        if (!verticesLocked)
            return;
        Point3* vertices = verticesLocked.get();

        create_sphere_mesh(nau::Span<uint8_t>((uint8_t*)vertices, v_count * sizeof(Point3)),
                           nau::Span<uint8_t>((uint8_t*)indices, f_count * 6),
                           1.0f,
                           SLICES, SLICES,
                           sizeof(Point3),
                           false, false, false, false);
        vertices += v_count;
        vertices[0] = Point3(0, 0, 0);
        vertices[1] = Point3(-1, -1, 1);
        vertices[2] = Point3(+1, -1, 1);
        vertices[3] = Point3(-1, +1, 1);
        vertices[4] = Point3(+1, +1, 1);

        indices += f_count * 3;
        indices[0] = v_count + 0;
        indices[1] = v_count + 2;
        indices[2] = v_count + 1;
        indices += 3;
        indices[0] = v_count + 0;
        indices[1] = v_count + 3;
        indices[2] = v_count + 4;
        indices += 3;
        indices[0] = v_count + 0;
        indices[1] = v_count + 1;
        indices[2] = v_count + 3;
        indices += 3;
        indices[0] = v_count + 0;
        indices[1] = v_count + 4;
        indices[2] = v_count + 2;
        indices += 3;
        indices[0] = v_count + 1;
        indices[1] = v_count + 2;
        indices[2] = v_count + 3;
        indices += 3;
        indices[0] = v_count + 3;
        indices[1] = v_count + 2;
        indices[2] = v_count + 4;
    }

    async::Task<> ClusteredLights::initLightMaterial()
    {
        closeSpot();
        MaterialAssetRef spotLightsMatRef = AssetPath{"file:/res/materials/deffered_light.nmat_json"};
        lightsMat = co_await spotLightsMatRef.getAssetViewTyped<MaterialAssetView>();
        NAU_ASSERT(lightsMat);
    }

    void ClusteredLights::closeLightMaterial()
    {
        lightsMat = nullptr;
    }

    void ClusteredLights::setBuffers()
    {
        fillBuffers();
        d3d::setind(coneSphereIb);
        d3d::setvsrc(0, coneSphereVb, sizeof(Point3));
    }

    void ClusteredLights::resetBuffers()
    {
    }

    void ClusteredLights::renderPrims(MaterialAssetView::Ptr material, const char* pipeline, Sbuffer* replaced_buffer, int inst_count, int, int, int index_start, int fcount)
    {
        if (!inst_count)
            return;
        fillBuffers();

        /* TODO shader buffers
        D3DRESID old_buffer = shader_globals::get_buf(buffer_var_id);
        shader_globals::set_buffer(buffer_var_id, replaced_buffer);
        */
        material->bindPipeline(pipeline);

        d3d::set_const_buffer(STAGE_VS, 1, replaced_buffer);

        d3d::drawind_instanced(PRIM_TRILIST, index_start, fcount, 0, inst_count);
    }

    void ClusteredLights::renderDebugLights()
    {
        if (!lightsMat)
        {
            return;
        }
        if((debugSpotLights.size() + debugOmniLights.size()) == 0)
        {
            return;
        }

        setBuffers();

        if (debugSpotLights.size())
        {
            debugSpotLightsCB.reallocate(debugSpotLights.size(), MAX_VISIBLE_FAR_LIGHTS, "far_spot_lights");
            debugSpotLightsCB.update(debugSpotLights.data(), data_size(debugSpotLights));
            renderPrims(lightsMat, "SpotlightDebug", debugSpotLightsCB.get(), debugSpotLights.size(), v_count, 5, f_count * 3, 6);
        }
        if (debugOmniLights.size())
        {
            debugOmniLightsCB.reallocate(debugOmniLights.size(), MAX_VISIBLE_FAR_LIGHTS, "far_omni_lights");
            debugOmniLightsCB.update(debugOmniLights.data(), data_size(debugOmniLights));
            renderPrims(lightsMat, "OmnilightDebug", debugOmniLightsCB.get(), debugOmniLights.size(), 0, v_count, 0, f_count);
        }

        resetBuffers();
    }

    void ClusteredLights::renderDebugLightsBboxes()
    {
        spotLights.renderDebugBboxes();
        omniLights.renderDebugBboxes();
    }

    void ClusteredLights::destroyLight(uint32_t id)
    {
        DecodedLightId typeId = decode_light_id(id);
        switch (typeId.type)
        {
            case LightType::Spot:
                spotLights.destroyLight(typeId.id);
                break;
            case LightType::Omni:
                omniLights.destroyLight(typeId.id);
                break;
            case LightType::Invalid:
                return;
            default:
                NAU_FATAL_FAILURE("unknown light type");
        }
        /* TODO: Light shadows
        uint16_t& lightShadow = typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];

        if (lightShadows && lightShadow != INVALID_VOLUME)
            lightShadows->destroyVolume(lightShadow);
        lightShadow = INVALID_VOLUME;
        */
    }

    uint32_t ClusteredLights::addOmniLight(const OmniLight& light, OmniLightsManager::mask_type_t mask)
    {
        int id = omniLights.addLight(0, light);
        if (id < 0)
            return INVALID_LIGHT;
        omniLights.setLightMask(id, mask);
        if (dynamicOmniLightsShadows.size() <= id)
        {
            int start = dynamicOmniLightsShadows.size();
            dynamicOmniLightsShadows.resize(id + 1);
            memset(dynamicOmniLightsShadows.data() + start, 0xFF,
                   (dynamicOmniLightsShadows.size() - start) * elem_size(dynamicOmniLightsShadows));
        }
        return id;
    }

    // keep mask
    void ClusteredLights::setLight(uint32_t id, const OmniLight& light, bool invalidate_shadow)
    {
        DecodedLightId typeId = decode_light_id(id);
        NAU_ASSERT_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
                          "omni light {} is invalid (maxIndex= {})", typeId.id, omniLights.maxIndex());
        /* TODO: Light shadows
        if (lightShadows != nullptr && dynamicOmniLightsShadows[typeId.id] != INVALID_VOLUME)
        {
            uint32_t shadowId = dynamicOmniLightsShadows[typeId.id];
            invalidate_shadow &= isInvaliatingShadowsNeeded(omniLights.getLight(typeId.id), light);
            if (invalidate_shadow && lightShadows)
            {
                dynamicLightsShadowsVolumeSet.reset(shadowId);
                lightShadows->invalidateVolumeShadow(shadowId);
            }
        }
        */
        omniLights.setLight(typeId.id, light);
    }
    void ClusteredLights::setLightWithMask(uint32_t id, const OmniLight& light, OmniLightsManager::mask_type_t mask, bool invalidate_shadow)
    {
        DecodedLightId typeId = decode_light_id(id);
        NAU_ASSERT_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return,
                          "omni light {} is invalid (maxIndex= {})", typeId.id, omniLights.maxIndex());
        /* TODO: Light shadows
        if (lightShadows != nullptr && dynamicOmniLightsShadows[typeId.id] != INVALID_VOLUME)
        {
            uint32_t shadowId = dynamicOmniLightsShadows[typeId.id];
            invalidate_shadow &= isInvaliatingShadowsNeeded(omniLights.getLight(typeId.id), light);
            if (invalidate_shadow && lightShadows)
            {
                dynamicLightsShadowsVolumeSet.reset(shadowId);
                lightShadows->invalidateVolumeShadow(shadowId);
            }
        }
        */
        omniLights.setLight(typeId.id, light);
        omniLights.setLightMask(typeId.id, mask);
    }

    ClusteredLights::OmniLight ClusteredLights::getOmniLight(uint32_t id) const
    {
        DecodedLightId typeId = decode_light_id(id);
        NAU_ASSERT_AND_DO(typeId.type == LightType::Omni && typeId.id <= omniLights.maxIndex(), return OmniLight(),
                          "omni light {} is invalid (maxIndex= {})", typeId.id, omniLights.maxIndex());
        return omniLights.getLight(id);
    }

    void ClusteredLights::setLight(uint32_t id, const SpotLight& light, SpotLightsManager::mask_type_t mask, bool invalidate_shadow)
    {
        setLight(id, light, invalidate_shadow);
        DecodedLightId typeId = decode_light_id(id);
        spotLights.setLightMask(typeId.id, mask);
    }

    void ClusteredLights::setLight(uint32_t id, const SpotLight& light, bool invalidate_shadow)
    {
        DecodedLightId typeId = decode_light_id(id);
        NAU_ASSERT_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return,
                          "({}) light {} is invalid (maxIndex= {})", typeId.type == LightType::Spot ? "spot" : "omni", typeId.id, spotLights.maxIndex());
        /* TODO: Light shadows
        if (lightShadows != nullptr && dynamicSpotLightsShadows[typeId.id] != INVALID_VOLUME)
        {
            uint32_t shadowId = dynamicSpotLightsShadows[typeId.id];
            invalidate_shadow &= isInvaliatingShadowsNeeded(spotLights.getLight(typeId.id), light);
            if (invalidate_shadow && lightShadows)
            {
                dynamicLightsShadowsVolumeSet.reset(shadowId);
                lightShadows->invalidateVolumeShadow(shadowId);
            }
        }
        */
        spotLights.setLight(typeId.id, light);
    }

    ClusteredLights::SpotLight ClusteredLights::getSpotLight(uint32_t id) const
    {
        DecodedLightId typeId = decode_light_id(id);
        NAU_ASSERT_AND_DO(typeId.type == LightType::Spot && typeId.id <= spotLights.maxIndex(), return SpotLight(),
                          "({}) light {} is invalid (maxIndex= {})", typeId.type == LightType::Spot ? "spot" : "omni", id, spotLights.maxIndex());
        return spotLights.getLight(typeId.id);
    }

    bool ClusteredLights::isLightVisible(uint32_t id) const
    {
        DecodedLightId typeId = decode_light_id(id);
        switch (typeId.type)
        {
            case LightType::Spot:
                NAU_ASSERT_RETURN(typeId.id <= spotLights.maxIndex(), false);
                return visibleSpotLightsIdSet.test(typeId.id);
            case LightType::Omni:
                NAU_ASSERT_RETURN(typeId.id <= omniLights.maxIndex(), false);
                return visibleOmniLightsIdSet.test(typeId.id);
            case LightType::Invalid:
                return false;
            default:
                NAU_FATAL_FAILURE("unknown light type");
        }
        return false;
    }

    uint32_t ClusteredLights::addSpotLight(const SpotLight& light, SpotLightsManager::mask_type_t mask)
    {
        int id = spotLights.addLight(light);
        if (id < 0)
            return INVALID_LIGHT;
        spotLights.setLightMask(id, mask);
        if (dynamicSpotLightsShadows.size() <= id)
        {
            int start = dynamicSpotLightsShadows.size();
            dynamicSpotLightsShadows.resize(id + 1);
            memset(dynamicSpotLightsShadows.data() + start, 0xFF,
                   (dynamicSpotLightsShadows.size() - start) * elem_size(dynamicSpotLightsShadows));
        }
        return id | SPOT_LIGHT_FLAG;
    }

    /* TODO: Light shadows
    bool ClusteredLights::addShadowToLight(uint32_t id, bool only_static_casters, bool hint_dynamic, uint16_t quality, uint8_t priority, uint8_t max_size_srl, DynamicShadowRenderGPUObjects render_gpu_objects)
    {
        if (!lightShadows)
            return false;

        DecodedLightId typeId = decode_light_id(id);
        switch (typeId.type)
        {
            case LightType::Spot:
            {
                NAU_ASSERTF_RETURN(dynamicSpotLightsShadows[typeId.id] == INVALID_VOLUME, false, "spot light {} already has shadow", typeId.id);
                int shadowId =
                    lightShadows->allocateVolume(only_static_casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
                if (shadowId < 0)
                    return false;
                dynamicSpotLightsShadows[typeId.id] = shadowId;
                dynamicLightsShadowsVolumeSet.reset(shadowId);
            }
            break;
            case LightType::Omni:
            {
                NAU_ASSERTF_RETURN(dynamicOmniLightsShadows[typeId.id] == INVALID_VOLUME, false, "omni light {} already has shadow", typeId.id);
                int shadowId =
                    lightShadows->allocateVolume(only_static_casters, hint_dynamic, quality, priority, max_size_srl, render_gpu_objects);
                if (shadowId < 0)
                    return false;
                dynamicOmniLightsShadows[typeId.id] = shadowId;
                dynamicLightsShadowsVolumeSet.reset(shadowId);
            }
            break;
            case LightType::Invalid:
                return false;
            default:
                NAU_ASSERT_FAIL("unknown light type");
        }
        return true;

    }
     */

    /* TODO: Light shadows
    bool ClusteredLights::getShadowProperties(uint32_t id, bool& only_static_casters, bool& hint_dynamic, uint16_t& quality, uint8_t& priority, uint8_t& shadow_size_srl, DynamicShadowRenderGPUObjects& render_gpu_objects) const
    {
        if (!lightShadows)
            return false;

        DecodedLightId typeId = decode_light_id(id);
        if (typeId.type == LightType::Invalid)
            return false;

        const uint16_t& lightShadow =
            typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];
        if (lightShadow == INVALID_VOLUME)
            return false;

        return lightShadows->getShadowProperties(lightShadow, only_static_casters, hint_dynamic, quality, priority, shadow_size_srl,

    }                                          render_gpu_objects);
    */

    /* TODO: Light shadows
    void ClusteredLights::removeShadow(uint32_t id)
    {
        if (!lightShadows)
            return;

        DecodedLightId typeId = decode_light_id(id);
        if (typeId.type == LightType::Invalid)
            return;

        uint16_t& lightShadow = typeId.type == LightType::Spot ? dynamicSpotLightsShadows[typeId.id] : dynamicOmniLightsShadows[typeId.id];
        if (lightShadow == INVALID_VOLUME)
        {
            NAU_ASSERTF(0, "light {} has no shadow", id);
            return;
        }
        lightShadows->destroyVolume(lightShadow);
        lightShadow = INVALID_VOLUME;

    }

    void ClusteredLights::invalidateAllShadows()
    {
        if (lightShadows)
            lightShadows->invalidateAllVolumes();
    }
    */

    void ClusteredLights::invalidateStaticObjects(const BBox3& box)
    {
        /* TODO: Light shadows
        if (lightShadows)
            lightShadows->invalidateStaticObjects(box);
        */
    }

    /* TODO: Light shadows
    void ClusteredLights::prepareShadows(const Point3& viewPos,
                                         const math::Matrix4& globtm,
                                         float hk,
                                         nau::ConstSpan<BBox3> dynamicBoxes,
                                         eastl::fixed_function<sizeof(void*) * 2, StaticRenderCallback> render_static,
                                         eastl::fixed_function<sizeof(void*) * 2, DynamicRenderCallback> render_dynamic)
    {
        if ((visibleSpotLightsId.empty() && visibleOmniLightsId.empty()) || !lightShadows)
            return;
        Vector4 vposMaxShadow = Vector4(viewPos.getX(), viewPos.getY(), viewPos.getZ(), -maxShadowDist);
        Vector4 mulFactor = Vector4(1, 1, 1, -1);
        TIME_D3D_PROFILE(spotAndOmniShadows);
        SCOPE_VIEW_PROJ_MATRIX;
        SCOPE_RENDER_TARGET;

        lightShadows->startPrepareShadows();

        for (auto spotId : visibleSpotLightsId)
        {
            uint32_t shadowId = dynamicSpotLightsShadows[spotId];
            if (shadowId != INVALID_VOLUME)
            {
                setSpotLightShadowVolume(spotId);
                // only if distance to light is closer than max shadow see distance we need update shadow
                Vector4 bounding = spotLights.getBoundingSphere(spotId);
                bounding = v_sub(bounding, vposMaxShadow);
                bounding = v_mul(bounding, bounding);
                bounding = v_dot4_x(bounding, mulFactor);
                if (v_test_vec_x_lt_0(bounding))
                    lightShadows->useShadowOnFrame(shadowId);
            }
        }

        for (auto omniId : visibleOmniLightsId)
        {
            uint32_t shadowId = dynamicOmniLightsShadows[omniId];
            if (shadowId != INVALID_VOLUME)
            {
                setOmniLightShadowVolume(omniId);
                // only if distance to light is closer than max shadow see distance we need update shadow
                Vector4 bounding = omniLights.getBoundingSphere(omniId);
                bounding = v_sub(bounding, vposMaxShadow);
                bounding = v_mul(bounding, bounding);
                bounding = v_dot4_x(bounding, mulFactor);
                if (v_test_vec_x_lt_0(bounding))
                    lightShadows->useShadowOnFrame(shadowId);
            }
        }

        lightShadows->setDynamicObjectsContent(dynamicBoxes.data(), dynamicBoxes.size());  // dynamic content within those boxes

        lightShadows->endPrepareShadows(maxShadowsToUpdateOnFrame, 0.25, viewPos, hk, globtm);
        if (lightShadows->getShadowVolumesToRender().size())
        {
            // debug("render {} / {}", lightShadows->getShadowVolumesToRender().size(), visibleSpotLightsId.size());
            dag::ConstSpan<uint16_t> volumesToRender = lightShadows->getShadowVolumesToRender();
            lightShadows->startRenderVolumes();
            bool staticOverrideState = false;
            shaders::OverrideStateId originalState = shaders::overrides::get_current();
            for (int i = volumesToRender.size() - 1; i >= 0; --i)
            {
                shaders::overrides::set(depthBiasOverrideId);
                mat44f view, proj, viewItm;
                const int id = volumesToRender[i];
                ShadowSystem::RenderFlags renderFlags;
                uint32_t numViews = lightShadows->startRenderVolume(id, proj, renderFlags);
                if (renderFlags & ShadowSystem::RENDER_STATIC)
                {
                    TIME_D3D_PROFILE(staticShadow);
                    if (!staticOverrideState)
                    {
                        shaders::overrides::reset();
                        shaders::overrides::set(depthBiasOverrideId);
                        staticOverrideState = true;
                    }
                    for (uint32_t viewId = 0; viewId < numViews; ++viewId)
                    {
                        lightShadows->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_STATIC);
                        alignas(16) TMatrix viewItmS;
                        v_mat_43ca_from_mat44(viewItmS[0], viewItm);

                        d3d::settm(TM_VIEW, view);
                        d3d::settm(TM_PROJ, proj);
                        mat44f globTm;
                        v_mat44_mul(globTm, proj, view);

                        bool only_static_casters, hint_dynamic;
                        uint8_t priority, shadow_size_srl;
                        uint16_t quality;
                        DynamicShadowRenderGPUObjects render_gpu_objects;
                        lightShadows->getShadowProperties(id, only_static_casters, hint_dynamic, quality, priority, shadow_size_srl,
                                                          render_gpu_objects);

                        render_static(globTm, viewItmS, render_gpu_objects);
                        lightShadows->endRenderVolumeView(id, viewId);
                    }
                    lightShadows->endRenderStatic(id);
                }
                if (renderFlags & ShadowSystem::RENDER_DYNAMIC)
                {
                    TIME_D3D_PROFILE(dynamicShadow);
                    staticOverrideState = false;
                    shaders::overrides::reset();  // startRenderDynamic uses an other state
                    lightShadows->startRenderDynamic(id);
                    shaders::overrides::set(depthBiasOverrideId);
                    for (uint32_t viewId = 0; viewId < numViews; ++viewId)
                    {
                        lightShadows->startRenderVolumeView(id, viewId, viewItm, view, renderFlags, ShadowSystem::RENDER_DYNAMIC);
                        alignas(16) TMatrix viewItmS;
                        v_mat_43ca_from_mat44(viewItmS[0], viewItm);

                        d3d::settm(TM_VIEW, view);
                        d3d::settm(TM_PROJ, proj);

                        render_dynamic(viewItmS, view, proj);
                        shaders::overrides::reset();
                        lightShadows->endRenderVolumeView(id, viewId);
                    }
                }
                lightShadows->endRenderVolume(id);
                shaders::overrides::reset();
            }
            shaders::overrides::reset();
            shaders::overrides::set(originalState);
            lightShadows->endRenderVolumes();
        }

        updateShadowBuffers();

        shaders::overrides::set(depthBiasOverrideId);
        dstReadbackLights->update(render_static);
        shaders::overrides::reset();

    }
     */

    /* TODO: Light shadows
    void ClusteredLights::updateShadowBuffers()
    {
        Staticeastl::vector<Vector4, 1 + MAX_SPOT_LIGHTS * 4 + MAX_OMNI_LIGHTS> commonLightShadowData;
        int numSpotShadows = min<int>(visibleSpotLightsId.size(), MAX_SPOT_LIGHTS);
        int numOmniShadows = min<int>(visibleOmniLightsId.size(), MAX_OMNI_LIGHTS);
        commonLightShadowData.resize(1 + numSpotShadows * 4 + numOmniShadows);
        commonLightShadowData[0] = Vector4(numSpotShadows, numOmniShadows, 4 * numSpotShadows, 0);
        int baseIndex = 1;
        for (int i = 0; i < visibleSpotLightsId.size(); ++i)
        {
            uint16_t shadowId = dynamicSpotLightsShadows[visibleSpotLightsId[i]];
            if (shadowId != INVALID_VOLUME)
            {
                memcpy(&commonLightShadowData[baseIndex + i * 4], &lightShadows->getVolumeTexMatrix(shadowId), 4 * sizeof(Vector4));
            }
            else
            {
                memset(&commonLightShadowData[baseIndex + i * 4], 0, 4 * sizeof(Vector4));
            }
        }
        baseIndex += visibleSpotLightsId.size() * 4;
        for (int i = 0; i < visibleOmniLightsId.size(); ++i)
        {
            uint16_t shadowId = dynamicOmniLightsShadows[visibleOmniLightsId[i]];
            if (shadowId != INVALID_VOLUME)
            {
                commonLightShadowData[baseIndex + i] = lightShadows->getOctahedralVolumeTexData(shadowId);
            }
            else
            {
                memset(&commonLightShadowData[baseIndex + i], 0, sizeof(Vector4));
            }
        }

        if (numSpotShadows > 0 || numOmniShadows > 0)
        {
            commonLightShadowsBufferCB.reallocate(1 + visibleSpotLightsId.size() * 4 + numOmniShadows,
                                                  1 + MAX_SPOT_LIGHTS * 4 + MAX_SPOT_LIGHTS, "common_lights_shadows");
            shader_globals::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());

            commonLightShadowsBufferCB.update(commonLightShadowData.data(), data_size(commonLightShadowData));
        }

        if (spotLightSsssShadowDescBuffer && numSpotShadows > 0)
        {
            Staticeastl::vector<SpotlightShadowDescriptor, MAX_SPOT_LIGHTS> spotLightSsssShadowDesc;
            spotLightSsssShadowDesc.resize(numSpotShadows);
            for (int i = 0; i < visibleSpotLightsId.size(); ++i)
            {
                uint16_t shadowId = dynamicSpotLightsShadows[visibleSpotLightsId[i]];
                if (shadowId != INVALID_VOLUME)
                {
                    SpotlightShadowDescriptor& shadowDesc = spotLightSsssShadowDesc[i];

                    float wk;
                    Point2 zn_zfar;
                    lightShadows->getVolumeInfo(shadowId, wk, zn_zfar.getX(), zn_zfar.getY());
                    shadowDesc.decodeDepth = get_decode_depth(zn_zfar);

                    Point2 shadowUvSize = lightShadows->getShadowUvSize(shadowId);
                    shadowDesc.meterToUvAtZfar = max(shadowUvSize.getX(), shadowUvSize.getY()) / (2 * wk);
                    Vector4 shadowUvMinMax = lightShadows->getShadowUvMinMax(shadowId);
                    shadowDesc.uvMinMax = shadowUvMinMax;

                    bool onlyStaticCasters, hintDynamic;
                    uint16_t quality;
                    uint8_t priority, size;
                    DynamicShadowRenderGPUObjects renderGPUObjects;
                    lightShadows->getShadowProperties(shadowId, onlyStaticCasters, hintDynamic, quality, priority, size, renderGPUObjects);
                    shadowDesc.hasDynamic = static_cast<float>(!onlyStaticCasters);
                }
                else
                {
                    spotLightSsssShadowDesc[i] = {};
                }
            }

            spotLightSsssShadowDescBuffer.getBuf()->updateData(0, numSpotShadows * sizeof(SpotlightShadowDescriptor),
                                                               static_cast<const void*>(spotLightSsssShadowDesc.data()), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
        }
    }
    */

    /* TODO: Light shadows
    void ClusteredLights::setSpotLightShadowVolume(int spot_light_id)
    {
        uint32_t shadowId = dynamicSpotLightsShadows[spot_light_id];
        if (dynamicLightsShadowsVolumeSet.test(shadowId))
            return;
        const SpotLightsManager::RawLight& l = spotLights.getLight(spot_light_id);
        mat44f viewITM;
        spotLights.getLightView(spot_light_id, viewITM);

        BBox3 box;
        v_bbox3_init_empty(box);
        float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.getW());
        lightShadows->setShadowVolume(shadowId, viewITM, lightZnZfar.getX(), lightZnZfar.getY(), 1. / l.dir_angle.getW(), box);
        dynamicLightsShadowsVolumeSet.set(shadowId);

    }
    */

    /* TODO: Light shadows
    void ClusteredLights::setOmniLightShadowVolume(int omni_light_id)
    {
        uint32_t shadowId = dynamicOmniLightsShadows[omni_light_id];
        if (dynamicLightsShadowsVolumeSet.test(shadowId))
            return;
        const OmniLightsManager::RawLight& l = omniLights.getLight(omni_light_id);

        BBox3 box;
        v_bbox3_init_empty(box);
        float2 lightZnZfar = get_light_shadow_zn_zf(l.pos_radius.getW());
        vec3f vpos = Vector4(l.pos_radius.getX(), l.pos_radius.getY(), l.pos_radius.getZ(), 0);
        lightShadows->setOctahedralShadowVolume(shadowId, vpos, lightZnZfar.getX(), lightZnZfar.getY(), box);
        dynamicLightsShadowsVolumeSet.set(shadowId);

    }
    */

    void ClusteredLights::setOutOfFrustumLightsToShader()
    {
        NAU_ASSERT(lightsInitialized);
        /* TODO global shader buffers
        shader_globals::set_buffer(omni_lightsVarId, outOfFrustumOmniLightsCB.getId());
        shader_globals::set_buffer(spot_lightsVarId, outOfFrustumVisibleSpotLightsCB.getId());
        shader_globals::set_buffer(common_lights_shadowsVarId, outOfFrustumCommonLightsShadowsCB.getId());
        */
    }

    void ClusteredLights::setInsideOfFrustumLightsToShader()
    {
        NAU_ASSERT(lightsInitialized);
        /* TODO global shader buffers
        shader_globals::set_buffer(omni_lightsVarId, visibleOmniLightsCB.getId());
        shader_globals::set_buffer(spot_lightsVarId, visibleSpotLightsCB.getId());
        shader_globals::set_buffer(common_lights_shadowsVarId, commonLightShadowsBufferCB.getId());
        */
    }

    void ClusteredLights::setBuffersToShader()
    {
        fillBuffers();
    }

    /*static void draw_cached_debug_wired_box(Point3 p[8], E3DCOLOR color)
    {
      for (int z = 0; z <= 4; z+=4)
      {
        draw_cached_debug_line ( p[z+0], p[z+1], color );
        draw_cached_debug_line ( p[z+1], p[z+3], color );
        draw_cached_debug_line ( p[z+3], p[z+2], color );
        draw_cached_debug_line ( p[z+2], p[z+0], color );
      }

      draw_cached_debug_line ( p[0], p[4+0], color );
      draw_cached_debug_line ( p[1], p[4+1], color );
      draw_cached_debug_line ( p[2], p[4+2], color );
      draw_cached_debug_line ( p[3], p[4+3], color );
    }*/

    void ClusteredLights::drawDebugClusters(int slice)
    {
        G_UNREFERENCED(slice);
        /*begin_draw_cached_debug_lines(false, false);
        TMatrix fView;
        v_mat_43cu_from_mat44(fView[0], clusters.view);
        set_cached_debug_lines_wtm(inverse(fView));
        int minZ = slice ? slice-1 : 0;
        int maxZ = slice ? slice : CLUSTERS_D-1;
        for (int z = minZ; z < maxZ; ++z)
          for (int y = 0; y < CLUSTERS_H; ++y)
            for (int x = 0; x < CLUSTERS_W; ++x)
            {
              Point3 p[8];
              for (int cp = 0, k = 0; k<2; ++k)
                for (int j = 0; j<2; ++j)
                  for (int i = 0; i<2; ++i, cp++)
                    p[cp] = (clusters.frustumPoints.data())[(x+i)+(y+j)*(CLUSTERS_W+1)+(z+k)*(CLUSTERS_W+1)*(CLUSTERS_H+1)];
              E3DCOLOR c = tempSpotItemsPtr->gridCount[x+y*CLUSTERS_W+z*CLUSTERS_W*CLUSTERS_H]>0 ? 0xffff00FF : 0x7f7f7F7F;
              draw_cached_debug_wired_box(p, c);
            }
        set_cached_debug_lines_wtm(TMatrix::IDENT);
        end_draw_cached_debug_lines();*/
        /*begin_draw_cached_debug_lines(false, false);
        for (int i = 0; i < clusters.frustumPoints.size(); ++i)
          draw_cached_debug_sphere(Point3::xyz(clusters.frustumPoints[i]), 0.02, 0xFFFFFFFF);
        end_draw_cached_debug_lines();*/

        /*begin_draw_cached_debug_lines(true, false);
        for (int i = 0; i < visibleSpotLightsBounds.size(); ++i)
        {
          const Vector4 &sph = *(Vector4*)&visibleSpotLightsBounds[i];
          draw_cached_debug_sphere(Point3::xyz(sph), sph.getW(),0xFFFFFFFF);
        }
        end_draw_cached_debug_lines();*/
    }

    bool ClusteredLights::reallocate_common(Sbuffer*& buf, uint16_t& size, int target_size, const char* stat_name)
    {
        if (size >= target_size)
            return true;
        Sbuffer* cb2 = d3d::buffers::create_one_frame_cb(target_size, stat_name);
        if (!cb2)
        {
            NAU_LOG_ERROR("can't re-create buffer <{}> for size {} from {}", stat_name, target_size, size);
            return false;
        }
        size = target_size;
        if (buf)
        {
            buf->destroy();
        }
        buf = cb2;
        return true;
    }

    bool ClusteredLights::updateConsts(Sbuffer* buf, void* data, int data_size, int elems_count)
    {
        uint32_t* destData = 0;
        bool ret = buf->lock(0, 0, (void**)&destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
        d3d_err(ret);
        if (!ret || !destData)
            return false;
        if (elems_count >= 0)
        {
            destData[0] = elems_count;
            destData += 4;
        }
        if (data_size)
            memcpy(destData, data, data_size);
        buf->unlock();
        return true;
    }

    void ClusteredLights::afterReset()
    {
        initConeSphere();

        /* TODO: Light shadows
        if (lightShadows)
            lightShadows->afterReset();
        */
    }

    /* TODO: Light shadows
    BBox3 ClusteredLights::getActiveShadowVolume() const
    {
        if (!lightShadows)
        {
            BBox3 ret;
            v_bbox3_init_empty(ret);
            return ret;
        }
        return lightShadows->getActiveShadowVolume();

    }
    void ClusteredLights::setNeedSsss(bool need_ssss)
    {

        spotLightSsssShadowDescBuffer.close();
        if (need_ssss)
            spotLightSsssShadowDescBuffer = dag::create_sbuffer(sizeof(SpotlightShadowDescriptor), MAX_SPOT_LIGHTS,
                                                                SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, "spot_lights_ssss_shadow_desc");
    }
    */
}  // namespace nau::render