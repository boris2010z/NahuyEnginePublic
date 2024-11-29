// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "graphics_assets/static_meshes/static_mesh.h"

#include "nau/async/task.h"

nau::StaticMesh::StaticMesh()
{
}

eastl::span<nau::math::float4> getTangents(const eastl::span<uint16_t>& indices,
                                           const eastl::span<nau::math::float3>& positions,
                                           const eastl::span<nau::math::float3>& normals,
                                           const eastl::span<nau::math::float2>& uvs)
{
    if (indices.empty() || positions.empty() || uvs.empty())
    {
        return {};
    }

    nau::math::float4* tangents = new nau::math::float4[normals.size()];

    for (int i = 0; i < indices.size(); i += 3)
    {
        uint16_t index0 = indices[i + 0];
        uint16_t index1 = indices[i + 1];
        uint16_t index2 = indices[i + 2];

        const nau::math::Vector3 v0 = nau::math::Vector3(positions[index0].x, positions[index0].y, positions[index0].z);
        const nau::math::Vector3 v1 = nau::math::Vector3(positions[index1].x, positions[index1].y, positions[index1].z);
        const nau::math::Vector3 v2 = nau::math::Vector3(positions[index2].x, positions[index2].y, positions[index2].z);

        const nau::math::Vector3 n0 = nau::math::Vector3(normals[index0].x, normals[index0].y, normals[index0].z);
        const nau::math::Vector3 n1 = nau::math::Vector3(normals[index1].x, normals[index1].y, normals[index1].z);
        const nau::math::Vector3 n2 = nau::math::Vector3(normals[index2].x, normals[index2].y, normals[index2].z);

        const nau::math::Vector2 uv0 = nau::math::Vector2(uvs[index0].x, uvs[index0].y);
        const nau::math::Vector2 uv1 = nau::math::Vector2(uvs[index1].x, uvs[index1].y);
        const nau::math::Vector2 uv2 = nau::math::Vector2(uvs[index2].x, uvs[index2].y);

        const nau::math::Vector3 dv1 = v1 - v0;
        const nau::math::Vector3 dv2 = v2 - v0;

        const nau::math::Vector2 duv1 = uv1 - uv0;
        const nau::math::Vector2 duv2 = uv2 - uv0;

        float r = 1.0f / (duv1[0] * duv2[1] - duv1[1] * duv2[0]);
        nau::math::Vector3 tangent = (dv1 * duv2[1] - dv2 * duv1[1]) * r;

        nau::math::Vector3 tangent0 = tangent - Vectormath::SSE::dot(tangent, n0) * n0;
        nau::math::Vector3 tangent1 = tangent - Vectormath::SSE::dot(tangent, n1) * n1;
        nau::math::Vector3 tangent2 = tangent - Vectormath::SSE::dot(tangent, n2) * n2;

        tangent0 = Vectormath::SSE::normalize(tangent0);
        tangent1 = Vectormath::SSE::normalize(tangent1);
        tangent2 = Vectormath::SSE::normalize(tangent2);

        tangents[index0] = nau::math::float4(tangent0[0], tangent0[1], tangent0[2], 1.0f);
        tangents[index1] = nau::math::float4(tangent1[0], tangent1[1], tangent1[2], 1.0f);
        tangents[index2] = nau::math::float4(tangent2[0], tangent2[1], tangent2[2], 1.0f);
    }
    eastl::span span = {tangents, normals.size()};
    return span;
}

nau::async::Task<nau::Ptr<nau::StaticMesh>> nau::StaticMesh::createFromStaticMeshAccessor(IMeshAssetAccessor& meshAccessor)
{
    nau::StaticMesh::Ptr mesh = rtti::createInstance<StaticMesh>();

    nau::StaticMeshLod& lod0 = mesh->lods.emplace_back();

    const auto meshDesc = meshAccessor.getDescription();

    d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

    auto indexBufferTask = [](IMeshAssetAccessor& accessor, const MeshDescription& meshDesc) -> nau::async::Task<std::tuple<Sbuffer*, size_t>>
    {
        const size_t bufferSize = meshDesc.indexCount * sizeof(uint16_t);
        if (bufferSize == 0)
        {
            co_return std::tuple{nullptr, bufferSize};
        }

        Sbuffer* ibuf;
        {
            ibuf = d3d::create_ib(bufferSize, SBCF_DYNAMIC, u8"IndexBuf");  // (position_float4 + color_float4) * 3

            std::byte* mem = nullptr;
            ibuf->lock(0, bufferSize, reinterpret_cast<void**>(&mem), VBLOCK_WRITEONLY);  //(0, nullptr, reinterpret_cast<void**>(&mem));
            scope_on_leave
            {
                ibuf->unlock();
            };

            accessor.copyIndices(mem, bufferSize, ElementFormat::Uint16).ignore();
        }

        co_return std::tuple{std::move(ibuf), bufferSize};
    }(meshAccessor, meshDesc);

    auto vertexBufferTask = [&](IMeshAssetAccessor& accessor, const MeshDescription& meshDesc) -> nau::async::Task<std::tuple<Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t, Sbuffer*, size_t>>
    {
        const size_t posBufferSize = meshDesc.vertexCount * sizeof(float[3]);
        const size_t nrmBufferSize = meshDesc.vertexCount * sizeof(float[3]);
        const size_t tangentBufferSize = meshDesc.vertexCount * sizeof(float[4]);
        const size_t texBufferSize = meshDesc.vertexCount * sizeof(float[2]);

        if (meshDesc.vertexCount == 0)
        {
            co_return std::tuple{nullptr, posBufferSize,
                                 nullptr, nrmBufferSize,
                                 nullptr, tangentBufferSize,
                                 nullptr, texBufferSize};
        }

        eastl::array<OutputVertAttribDescription,4> outLayout;

        auto& posDesc = outLayout[0];
        {
            posDesc.semantic = "POSITION";
            posDesc.semanticIndex = 0;
            posDesc.elementFormat = ElementFormat::Float;
            posDesc.attributeType = AttributeType::Vec3;
            posDesc.byteStride = 0;
        }

        auto& nrmDesc = outLayout[1];
        {
            nrmDesc.semantic = "NORMAL";
            nrmDesc.semanticIndex = 0;
            nrmDesc.elementFormat = ElementFormat::Float;
            nrmDesc.attributeType = AttributeType::Vec3;
            nrmDesc.byteStride = 0;
        }

        auto& tangentDesc = outLayout[2];
        {
            tangentDesc.semantic = "TANGENT";
            tangentDesc.semanticIndex = 0;
            tangentDesc.elementFormat = ElementFormat::Float;
            tangentDesc.attributeType = AttributeType::Vec4;
            tangentDesc.byteStride = 0;
        }

        auto& uv0Desc = outLayout[3];
        {
            uv0Desc.semantic = "TEXCOORD";
            uv0Desc.semanticIndex = 0;
            uv0Desc.elementFormat = ElementFormat::Float;
            uv0Desc.attributeType = AttributeType::Vec2;
            uv0Desc.byteStride = 0;
        }

        Sbuffer* pbuf;
        Sbuffer* nbuf;
        Sbuffer* tangentbuf;
        Sbuffer* tbuf;
        {
            pbuf = d3d::create_vb(posBufferSize, SBCF_DYNAMIC, u8"posBuf");
            nbuf = d3d::create_vb(nrmBufferSize, SBCF_DYNAMIC, u8"normBuf");
            tangentbuf = d3d::create_vb(tangentBufferSize, SBCF_DYNAMIC, u8"tangentBuf");
            tbuf = d3d::create_vb(texBufferSize, SBCF_DYNAMIC, u8"texBuf");

            std::byte* posMem = nullptr;
            pbuf->lock(0, posBufferSize, reinterpret_cast<void**>(&posMem), VBLOCK_WRITEONLY);

            std::byte* nrmMem = nullptr;
            nbuf->lock(0, nrmBufferSize, reinterpret_cast<void**>(&nrmMem), VBLOCK_WRITEONLY);

            std::byte* tangentMem = nullptr;
            tangentbuf->lock(0, tangentBufferSize, reinterpret_cast<void**>(&tangentMem), VBLOCK_WRITEONLY);

            std::byte* texMem = nullptr;
            tbuf->lock(0, texBufferSize, reinterpret_cast<void**>(&texMem), VBLOCK_WRITEONLY);

            scope_on_leave
            {
                // Calculate AABB
                nau::math::AABB aabb = nau::math::AABB();
                aabb.InitFromVertsSlow(reinterpret_cast<nau::math::float3*>(posMem), meshDesc.vertexCount);

                mesh->m_localBSphere = nau::math::BSphere3();
                mesh->m_localBSphere += nau::math::BBox3(aabb.minBounds, aabb.maxBounds);

                NAU_ASSERT(mesh->m_localBSphere.r > 0.00001f);

                pbuf->unlock();
                nbuf->unlock();
                tangentbuf->unlock();
                tbuf->unlock();
            };

            posDesc.outputBuffer = posMem;
            posDesc.outputBufferSize = posBufferSize;

            nrmDesc.outputBuffer = nrmMem;
            nrmDesc.outputBufferSize = nrmBufferSize;

            tangentDesc.outputBuffer = tangentMem;
            tangentDesc.outputBufferSize = tangentBufferSize;

            uv0Desc.outputBuffer = texMem;
            uv0Desc.outputBufferSize = texBufferSize;

            accessor.copyVertAttribs(outLayout).ignore();
        }

        co_return std::tuple{std::move(pbuf), posBufferSize, std::move(nbuf), nrmBufferSize, std::move(tangentbuf), tangentBufferSize, std::move(tbuf), texBufferSize};
    }(meshAccessor, meshDesc);

    co_await async::whenAll(Expiration::never(), indexBufferTask, vertexBufferTask);

    auto [indexBuffer, indexBufferSize] = *indexBufferTask;
    auto [posBuffer, posBufferSize, nrmBuffer, nrmBufferSize, tangentBuffer, tangentBufferSize, texBuffer, texBufferSize] = *vertexBufferTask;

    lod0.m_indexCount = meshDesc.indexCount;
    lod0.m_vertexCount = meshDesc.vertexCount;

    lod0.m_indexBuffer = indexBuffer;
    lod0.m_positionsBuffer = posBuffer;
    lod0.m_normalsBuffer = nrmBuffer;
    lod0.m_tangentsBuffer = tangentBuffer;
    lod0.m_texCoordsBuffer = texBuffer;

    if ((lod0.m_indexCount != 0) && (lod0.m_vertexCount != 0))
    {
        std::byte* mem = nullptr;
        indexBuffer->lock(0, indexBufferSize, reinterpret_cast<void**>(&mem), VBLOCK_READONLY);

        std::byte* posMem = nullptr;
        posBuffer->lock(0, posBufferSize, reinterpret_cast<void**>(&posMem), VBLOCK_READONLY);

        std::byte* nrmMem = nullptr;
        nrmBuffer->lock(0, nrmBufferSize, reinterpret_cast<void**>(&nrmMem), VBLOCK_READONLY);

        std::byte* texMem = nullptr;
        texBuffer->lock(0, texBufferSize, reinterpret_cast<void**>(&texMem), VBLOCK_READONLY);

        std::byte* tangentMem = nullptr;
        tangentBuffer->lock(0, tangentBufferSize, reinterpret_cast<void**>(&tangentMem), VBLOCK_WRITEONLY);

        auto tangs = getTangents({(uint16_t*)mem, meshDesc.indexCount},
                                 {(nau::math::float3*)posMem, meshDesc.vertexCount},
                                 {(nau::math::float3*)nrmMem, meshDesc.vertexCount},
                                 {(nau::math::float2*)texMem, meshDesc.vertexCount});
        memcpy(tangentMem, tangs.data(), tangentBufferSize);

        delete[] tangs.data();

        posBuffer->unlock();
        nrmBuffer->unlock();
        tangentBuffer->unlock();
        texBuffer->unlock();
        indexBuffer->unlock();
    }

    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);

    // load material
    nau::MaterialSlot& slot = lod0.m_materialSlots.emplace_back();
    slot.m_startIndex = 0;
    slot.m_endIndex = lod0.m_indexCount;

    static MaterialAssetRef material {AssetPath{"file:/res/materials/embedded/standard_opaque.nmat_json"}};
    slot.m_material = co_await material.getReloadableAssetViewTyped<MaterialAssetView>();

    co_return mesh;
}

bool nau::StaticMesh::createFromGeneratedData()
{
    return false;
}

const nau::StaticMeshLod& nau::StaticMesh::getLod(uint32_t lodInd) const
{
    NAU_ASSERT(lodInd < lods.size());

    return lods[lodInd];
}

uint32_t nau::StaticMesh::getLodsCount() const
{
    return lods.size();
}
