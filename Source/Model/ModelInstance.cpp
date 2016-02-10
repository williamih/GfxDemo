#include "Model/ModelInstance.h"
#include <stdlib.h>

#include "Core/Macros.h"

#include "Math/Matrix33.h"

#include "GpuDevice/GpuDrawItemWriter.h"
#include "GpuDevice/GpuMathUtils.h"

#include "Texture/TextureAsset.h"

#include "Model/ModelAsset.h"

struct ModelInstanceCBuffer {
    float worldTransform[4][4];
    float normalTransform[3][4];
    float diffuseColor[4];
    float specularColorAndGlossiness[4];
};

static GpuDrawItemPoolIndex InternalCreateDrawItem(GpuDrawItemPool& drawItemPool,
                                                   GpuDrawItemPoolIndex prev,
                                                   ModelAsset* model,
                                                   u32 submeshIndex,
                                                   const ModelInstanceCreateContext& ctx,
                                                   GpuBufferID modelCBuffer)
{
    MDLHeader* header = (MDLHeader*)(model->GetMDLData());
    MDLSubmesh* submeshes = (MDLSubmesh*)(model->GetMDLData() + header->ofsSubmeshes);
    MDLSubmesh& theSubmesh = submeshes[submeshIndex];

    GpuTextureID diffuseTex = ctx.defaultTexture;
    if (theSubmesh.diffuseTexture)
        diffuseTex = theSubmesh.diffuseTexture->GetGpuTextureID();

    GpuDrawItemWriter writer;
    GpuDrawItemPoolIndex index = drawItemPool.BeginDrawItem(writer, prev);
    writer.SetPipelineState(ctx.pipelineObject);
    writer.SetVertexBuffer(0, model->GetVertexBuf(), 0);
    writer.SetCBuffer(0, ctx.sceneCBuffer);
    writer.SetCBuffer(1, modelCBuffer);
    writer.SetTexture(0, diffuseTex);
    writer.SetSampler(0, ctx.sampler);
    writer.SetIndexBuffer(model->GetIndexBuf());
    writer.SetDrawCallIndexed(GPU_PRIMITIVE_TRIANGLES,
                              theSubmesh.indexStart,
                              theSubmesh.indexCount,
                              0,
                              GPU_INDEX_U32);
    writer.End();

    return index;
}

ModelInstance::ModelInstance(ModelAsset* model,
                             const ModelInstanceCreateContext& ctx)
    : m_drawItemPool(*ctx.drawItemPool)
    , m_model(model)
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

    RefreshDrawItems(ctx);
}

ModelInstance::~ModelInstance()
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPoolIndex next = m_drawItemPool.Next(m_drawItemIndex);
        m_drawItemPool.DeleteDrawItem(m_drawItemIndex);
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

void ModelInstance::RefreshDrawItems(const ModelInstanceCreateContext& ctx)
{
    while (m_drawItemIndex != 0xFFFFFFFF) {
        GpuDrawItemPoolIndex next = m_drawItemPool.Next(m_drawItemIndex);
        m_drawItemPool.DeleteDrawItem(m_drawItemIndex);
        m_drawItemIndex = next;
    }

    MDLHeader* header = (MDLHeader*)m_model->GetMDLData();
    u32 nSubmeshes = header->nSubmeshes;
    GpuDrawItemPoolIndex poolIndex(0xFFFFFFFF);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        poolIndex = InternalCreateDrawItem(
            *ctx.drawItemPool,
            poolIndex,
            m_model,
            i,
            ctx,
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
        items.push_back(m_drawItemPool.GetDrawItem(index));
        index = m_drawItemPool.Next(index);
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
