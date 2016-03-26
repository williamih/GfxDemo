#include "Model/ModelInstance.h"
#include <stdlib.h>

#include "Core/Macros.h"

#include "Math/Matrix33.h"

#include "GpuDevice/GpuDrawItemWriter.h"
#include "GpuDevice/GpuMathUtils.h"

#include "Texture/TextureAsset.h"

#include "Model/ModelAsset.h"
#include "Model/ModelScene.h"

struct ModelInstanceCBuffer {
    float worldTransform[4][4];
    float normalTransform[3][4];
    float diffuseColor[4];
    float specularColorAndGlossiness[4];
};

static GpuDrawItemPoolIndex InternalCreateDrawItem(
    ModelScene& scene,
    ModelAsset* model,
    GpuDrawItemPoolIndex prev,
    u32 flags,
    u32 submeshIndex,
    GpuBufferID modelCBuffer
)
{
    MDLHeader* header = (MDLHeader*)(model->GetMDLData());
    MDLSubmesh* submeshes = (MDLSubmesh*)(model->GetMDLData() + header->ofsSubmeshes);
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
    writer.SetVertexBuffer(0, model->GetVertexBuf(), 0);
    writer.SetCBuffer(0, scene.GetSceneCBuffer());
    writer.SetCBuffer(1, modelCBuffer);
    writer.SetTexture(0, diffuseTex);
    writer.SetSampler(0, sampler);
    writer.SetIndexBuffer(model->GetIndexBuf());
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

ModelInstance::ModelInstance(ModelScene& scene, ModelAsset* model, u32 flags)
    : m_scene(scene)
    , m_model(model)
    , m_flags(flags)
    , m_cbuffer(0)
    , m_drawItemIndex(0xFFFFFFFF)
{
    ASSERT(model);
    model->AddRef();

    GpuDevice* dev = model->GetGpuDevice();
    m_cbuffer = dev->BufferCreate(GPU_BUFFER_TYPE_CONSTANT,
                                  GPU_BUFFER_ACCESS_DYNAMIC,
                                  NULL,
                                  sizeof(ModelInstanceCBuffer));

    RefreshDrawItems();
}

ModelInstance::~ModelInstance()
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPool& pool = m_scene.GetDrawItemPool();
        GpuDrawItemPoolIndex next = pool.Next(m_drawItemIndex);
        pool.DeleteDrawItem(m_drawItemIndex);
        m_drawItemIndex = next;
    }

    m_model->GetGpuDevice()->BufferDestroy(m_cbuffer);

    m_model->Release();
}

ModelAsset* ModelInstance::GetModelAsset() const
{
    return m_model;
}

GpuBufferID ModelInstance::GetCBuffer() const
{
    return m_cbuffer;
}

void ModelInstance::RefreshDrawItems()
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPool& pool = m_scene.GetDrawItemPool();
        GpuDrawItemPoolIndex next = pool.Next(m_drawItemIndex);
        pool.DeleteDrawItem(m_drawItemIndex);
        m_drawItemIndex = next;
    }

    MDLHeader* header = (MDLHeader*)m_model->GetMDLData();
    u32 nSubmeshes = header->nSubmeshes;
    GpuDrawItemPoolIndex poolIndex(0xFFFFFFFF);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        poolIndex = InternalCreateDrawItem(
            m_scene,
            m_model,
            poolIndex,
            m_flags,
            i,
            m_cbuffer
        );
        if (m_drawItemIndex == 0xFFFFFFFF)
            m_drawItemIndex = poolIndex;
    }
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

u32 ModelInstance::GetFlags() const
{
    return m_flags;
}

void ModelInstance::SetFlags(u32 flags)
{
    if (flags != m_flags) {
        m_flags = flags;
        RefreshDrawItems();
    }
}

void ModelInstance::Update(const Matrix44& worldTransform,
                           const Vector3& diffuseColor,
                           const Vector3& specularColor,
                           float glossiness)
{
    GpuDevice* dev = m_model->GetGpuDevice();
    ModelInstanceCBuffer* buf = (ModelInstanceCBuffer*)dev->BufferGetContents(m_cbuffer);

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

    dev->BufferFlushRange(m_cbuffer, 0, sizeof(ModelInstanceCBuffer));
}
