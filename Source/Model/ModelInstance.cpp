#include "Model/ModelInstance.h"
#include <stdlib.h>

#include "Math/Matrix33.h"

#include "GpuDevice/GpuDrawItemWriter.h"
#include "GpuDevice/GpuMathUtils.h"

#include "Model/ModelAsset.h"

struct ModelInstanceCBuffer {
    float worldTransform[4][4];
    float normalTransform[3][4];
    float diffuseColor[4];
    float specularColorAndGlossiness[4];
};

static GpuDrawItemWriterDesc CreateDrawItemWriterDesc()
{
    GpuDrawItemWriterDesc desc;
    desc.SetNumCBuffers(2);
    desc.SetNumVertexBuffers(1);
    desc.SetNumTextures(1);
    desc.SetNumSamplers(1);
    return desc;
}

static void* ModelDrawItemAlloc(size_t size, void* userdata)
{
    return userdata;
}

static void InternalCreateDrawItem(void* location,
                                   const ModelInstanceCreateContext& ctx,
                                   GpuBufferID modelCBuffer)
{
    GpuDevice* dev = ctx.model->GetGpuDevice();
    GpuDrawItemWriterDesc desc = CreateDrawItemWriterDesc();

    GpuDrawItemWriter writer;
    writer.Begin(dev, desc, ModelDrawItemAlloc, location);
    writer.SetPipelineState(ctx.pipelineObject);
    writer.SetVertexBuffer(0, ctx.model->GetVertexBuf(), 0);
    writer.SetCBuffer(0, ctx.sceneCBuffer);
    writer.SetCBuffer(1, modelCBuffer);
    writer.SetTexture(0, ctx.defaultTexture);
    writer.SetSampler(0, ctx.samplerNonMipmapped);
    writer.SetIndexBuffer(ctx.model->GetIndexBuf());
    writer.SetDrawCallIndexed(GPU_PRIMITIVE_TRIANGLES,
                              0,
                              ctx.model->GetIndexCount(),
                              0,
                              GPU_INDEX_U32);
    writer.End();
}

ModelInstance* ModelInstance::Create(const ModelInstanceCreateContext& ctx)
{
    GpuDrawItemWriterDesc desc = CreateDrawItemWriterDesc();
    size_t size = sizeof(ModelInstance) + GpuDrawItemWriter::SizeInBytes(desc);
    u8* buf = (u8*)malloc(size);
    ModelInstance* instance = new (buf) ModelInstance(ctx);
    return instance;
}

void ModelInstance::Destroy(ModelInstance* instance)
{
    instance->~ModelInstance();
    free((void*)instance);
}

ModelInstance::ModelInstance(const ModelInstanceCreateContext& ctx)
    : m_model(ctx.model)
    , m_cbuffer(0)
{
    GpuDevice* dev = ctx.model->GetGpuDevice();
    m_cbuffer = dev->BufferCreate(GPU_BUFFER_TYPE_CONSTANT,
                                  GPU_BUFFER_ACCESS_DYNAMIC,
                                  NULL,
                                  sizeof(ModelInstanceCBuffer));
    InternalCreateDrawItem((void*)GetDrawItem(), ctx, m_cbuffer);
}

ModelInstance::~ModelInstance()
{
    GPUDEVICE_UNREGISTER_DRAWITEM(m_model->GetGpuDevice(), GetDrawItem());
    m_model->GetGpuDevice()->BufferDestroy(m_cbuffer);
}

const GpuDrawItem* ModelInstance::GetDrawItem() const
{
    return (const GpuDrawItem*)(this + 1);
}

ModelAsset* ModelInstance::GetModelAsset() const
{
    return m_model.get();
}

GpuBufferID ModelInstance::GetCBuffer() const
{
    return m_cbuffer;
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
