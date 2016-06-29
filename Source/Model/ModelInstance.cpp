#include "Model/ModelInstance.h"
#include <stdlib.h>

#include "Core/Macros.h"

#include "Math/Matrix33.h"

#include "GpuDevice/GpuDrawItemWriter.h"
#include "GpuDevice/GpuMathUtils.h"

#include "Texture/TextureAsset.h"

#include "Model/ModelShared.h"
#include "Model/ModelScene.h"

const u32 FLAG_LAST_IN_ASSET_GROUP = 1u << 31;

struct ModelInstanceCBuffer {
    float worldTransform[4][4];
    float normalTransform[3][4];
    float diffuseColor[4];
    float specularColorAndGlossiness[4];
};

static GpuDrawItemPoolIndex InternalCreateDrawItem(
    ModelScene& scene,
    ModelShared* shared,
    GpuDrawItemPoolIndex prev,
    u32 flags,
    u32 submeshIndex,
    GpuBufferID modelCBuffer
)
{
    MDLHeader* header = (MDLHeader*)(shared->GetMDLData());
    MDLSubmesh* submeshes = (MDLSubmesh*)(shared->GetMDLData() + header->ofsSubmeshes);
    MDLSubmesh& theSubmesh = submeshes[submeshIndex];
    GpuDrawItemPool& drawItemPool = scene.GetDrawItemPool();

    GpuTextureID diffuseTex = scene.GetDefaultTexture();
    if (theSubmesh.diffuseTexture)
        diffuseTex = theSubmesh.diffuseTexture->GetGpuTextureID();

    GpuSamplerID sampler;
    u32 psoFlags = 0;
    if (flags & ModelInstance::FLAG_SKYBOX) {
        psoFlags |= ModelScene::PSOFLAG_SKYBOX;
        sampler = scene.GetSamplerUVClamp();
    } else {
        sampler = scene.GetSamplerUVRepeat();
    }
    if (flags & ModelInstance::FLAG_WIREFRAME)
        psoFlags |= ModelScene::PSOFLAG_WIREFRAME;

    GpuDrawItemWriter writer;
    GpuDrawItemPoolIndex index = drawItemPool.BeginDrawItem(writer, prev);
    writer.SetPipelineState(scene.RequestPSO(psoFlags));
    writer.SetVertexBuffer(0, shared->GetVertexBuf(), 0);
    writer.SetCBuffer(0, scene.GetSceneCBuffer());
    writer.SetCBuffer(1, modelCBuffer);
    writer.SetTexture(0, diffuseTex);
    writer.SetSampler(0, sampler);
    writer.SetIndexBuffer(shared->GetIndexBuf());
    writer.SetDrawCallIndexed(
        GPU_PRIMITIVE_TRIANGLES,
        theSubmesh.indexStart,
        theSubmesh.indexCount,
        0,
        GPU_INDEX_U32
    );
    writer.End();

    return index;
}

ModelInstance::ModelInstance(ModelScene& scene, ModelShared* shared, u32 flags)
    : m_scene(scene)
    , m_shared(shared)
    , m_flagsAndAssetGroupInfo(flags)
    , m_cbuffer(0)
    , m_drawItemIndex(0xFFFFFFFF)
{
    ASSERT(shared);
    shared->AddRef();

    GpuDevice& dev = shared->GetGpuDevice();
    m_cbuffer = dev.BufferCreate(
        GPU_BUFFER_TYPE_CONSTANT,
        GPU_BUFFER_ACCESS_DYNAMIC,
        NULL,
        sizeof(ModelInstanceCBuffer),
        1 // maxUpdatesPerFrame
    );

    RecreateDrawItems();
}

ModelInstance::~ModelInstance()
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPool& pool = m_scene.GetDrawItemPool();
        GpuDrawItemPoolIndex next = pool.Next(m_drawItemIndex);
        pool.DeleteDrawItem(m_drawItemIndex);
        m_drawItemIndex = next;
    }

    // Update the linked list
    if (m_shared->GetFirstInstance() == this)
        m_shared->SetFirstInstance(m_link.Next());

    m_shared->GetGpuDevice().BufferDestroy(m_cbuffer);

    m_shared->Release();
}

ModelShared* ModelInstance::GetShared() const
{
    return m_shared;
}

GpuBufferID ModelInstance::GetCBuffer() const
{
    return m_cbuffer;
}

u32 ModelInstance::GetFlags() const
{
    return m_flagsAndAssetGroupInfo & ~(FLAG_LAST_IN_ASSET_GROUP);
}

void ModelInstance::SetFlags(u32 flags)
{
    if (flags != GetFlags()) {
        m_flagsAndAssetGroupInfo = flags |
            (m_flagsAndAssetGroupInfo & FLAG_LAST_IN_ASSET_GROUP);
        RecreateDrawItems();
    }
}

void ModelInstance::Update(const Matrix44& worldTransform,
                           const Vector3& diffuseColor,
                           const Vector3& specularColor,
                           float glossiness)
{
    GpuDevice& dev = m_shared->GetGpuDevice();
    ModelInstanceCBuffer* buf = (ModelInstanceCBuffer*)dev.BufferMap(m_cbuffer);

    Matrix33 normalTransform = worldTransform.UpperLeft3x3().Inverse().Transpose();

    GpuMathUtils::FillArrayColumnMajor(worldTransform, buf->worldTransform);
    GpuMathUtils::FillArrayColumnMajor(normalTransform, buf->normalTransform);
    buf->diffuseColor[0] = diffuseColor.x;
    buf->diffuseColor[1] = diffuseColor.y;
    buf->diffuseColor[2] = diffuseColor.z;
    buf->diffuseColor[3] = 0.0f;
    buf->specularColorAndGlossiness[0] = specularColor.x;
    buf->specularColorAndGlossiness[1] = specularColor.y;
    buf->specularColorAndGlossiness[2] = specularColor.z;
    buf->specularColorAndGlossiness[3] = glossiness;

    dev.BufferUnmap(m_cbuffer);
}

void ModelInstance::RecreateDrawItems()
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPool& pool = m_scene.GetDrawItemPool();
        GpuDrawItemPoolIndex next = pool.Next(m_drawItemIndex);
        pool.DeleteDrawItem(m_drawItemIndex);
        m_drawItemIndex = next;
    }

    MDLHeader* header = (MDLHeader*)m_shared->GetMDLData();
    u32 nSubmeshes = header->nSubmeshes;
    GpuDrawItemPoolIndex poolIndex(0xFFFFFFFF);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        poolIndex = InternalCreateDrawItem(
            m_scene,
            m_shared,
            poolIndex,
            GetFlags(),
            i,
            m_cbuffer
        );
        if (m_drawItemIndex == 0xFFFFFFFF)
            m_drawItemIndex = poolIndex;
    }
}

void ModelInstance::Reload(ModelShared* newShared)
{
    if (m_shared->GetFirstInstance() == this) {
        m_shared->SetFirstInstance(NULL);
        newShared->SetFirstInstance(this);
    }
    m_shared->Release();
    newShared->AddRef();
    m_shared = newShared;
    RecreateDrawItems();
}

void ModelInstance::AddDrawItemsToList(std::vector<const GpuDrawItem*>& items)
{
    GpuDrawItemPoolIndex index = m_drawItemIndex;
    while (index != 0xFFFFFFFF) {
        GpuDrawItemPool& pool = m_scene.GetDrawItemPool();
        items.push_back(pool.GetDrawItem(index));
        index = pool.Next(index);
    }
}

ModelInstance* ModelInstance::NextInAssetGroup()
{
    if (m_flagsAndAssetGroupInfo & FLAG_LAST_IN_ASSET_GROUP)
        return NULL;
    return m_link.Next();
}

void ModelInstance::MarkLastInAssetGroup()
{
    m_flagsAndAssetGroupInfo |= FLAG_LAST_IN_ASSET_GROUP;
}
