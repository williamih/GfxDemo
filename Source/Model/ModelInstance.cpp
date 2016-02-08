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
                                                   ModelAsset* model,
                                                   const ModelInstanceCreateContext& ctx,
                                                   GpuBufferID modelCBuffer)
{
    GpuTextureID diffuseTex = ctx.defaultTexture;
    if (model->GetDiffuseTex())
        diffuseTex = model->GetDiffuseTex()->GetGpuTextureID();

    GpuDrawItemWriter writer;
    GpuDrawItemPoolIndex index = drawItemPool.BeginDrawItem(writer);
    writer.SetPipelineState(ctx.pipelineObject);
    writer.SetVertexBuffer(0, model->GetVertexBuf(), 0);
    writer.SetCBuffer(0, ctx.sceneCBuffer);
    writer.SetCBuffer(1, modelCBuffer);
    writer.SetTexture(0, diffuseTex);
    writer.SetSampler(0, ctx.sampler);
    writer.SetIndexBuffer(model->GetIndexBuf());
    writer.SetDrawCallIndexed(GPU_PRIMITIVE_TRIANGLES,
                              0,
                              model->GetIndexCount(),
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

    m_drawItemIndex = InternalCreateDrawItem(
        *ctx.drawItemPool,
        m_model,
        ctx,
        m_cbuffer
    );
}

ModelInstance::~ModelInstance()
{
    m_drawItemPool.DeleteDrawItem(m_drawItemIndex);
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

void ModelInstance::RefreshDrawItem(const ModelInstanceCreateContext& ctx)
{
    m_drawItemPool.DeleteDrawItem(m_drawItemIndex);
    m_drawItemIndex = InternalCreateDrawItem(
        m_drawItemPool,
        m_model,
        ctx,
        m_cbuffer
    );
}

void ModelInstance::AddDrawItemsToList(std::vector<const GpuDrawItem*>& items)
{
    items.push_back(m_drawItemPool.GetDrawItem(m_drawItemIndex));
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
