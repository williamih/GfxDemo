#include "Scene/RenderTargetDisplay.h"

#include "GpuDevice/GpuSamplerCache.h"

#include "Asset/AssetCache.h"

#include "Shader/ShaderAsset.h"

static GpuDrawItemWriterDesc GetDrawItemDesc()
{
    GpuDrawItemWriterDesc desc;
    desc.SetNumTextures(2);
    desc.SetNumSamplers(1);
    desc.SetNumVertexBuffers(1);
    return desc;
}

void RenderTargetDisplay::SamplerCacheCallback(GpuSamplerCache& cache,
                                               void* userdata)
{
    RenderTargetDisplay* self = (RenderTargetDisplay*)userdata;
    cache.Release(self->m_sampler);
    self->m_sampler = cache.Acquire(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE);
}

RenderTargetDisplay::RenderTargetDisplay(
    GpuDevice& device,
    GpuSamplerCache& samplerCache,
    ShaderCache& shaderCache
)
    : m_device(device)
    , m_samplerCache(samplerCache)
    , m_shader(shaderCache.FindOrLoad("Assets/Shaders/BlitRT"))
    , m_renderPass()
    , m_vertexBuf()
    , m_inputLayout()
    , m_sampler(samplerCache.Acquire(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE))
    , m_pipelineStateObj()
    , m_drawItem()
{
    m_shader->AddRef();

    m_samplerCache.RegisterCallback(&RenderTargetDisplay::SamplerCacheCallback,
                                    (void*)this);

    GpuRenderLoadAction loadAction = GPU_RENDER_LOAD_ACTION_DISCARD;
    GpuRenderStoreAction storeAction = GPU_RENDER_STORE_ACTION_STORE;

    GpuRenderPassDesc renderPassDesc;
    renderPassDesc.numRenderTargets = 0;
    renderPassDesc.colorLoadActions = &loadAction;
    renderPassDesc.colorStoreActions = &storeAction;
    renderPassDesc.depthStencilLoadAction = GPU_RENDER_LOAD_ACTION_DISCARD;
    renderPassDesc.depthStencilStoreAction = GPU_RENDER_STORE_ACTION_STORE;
    m_renderPass = m_device.RenderPassCreate(renderPassDesc);

    float vertices[] = {
        // Top right
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        // Top left
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        // Bottom right
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        // Bottom left
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
    };
    m_vertexBuf = m_device.BufferCreate(
        GPU_BUFFER_TYPE_VERTEX,
        GPU_BUFFER_ACCESS_STATIC,
        vertices,
        sizeof vertices
    );

    GpuVertexAttribute attribs[] = {
        {GPU_VERTEX_ATTRIB_FLOAT3, 0, 0},
        {GPU_VERTEX_ATTRIB_FLOAT2, 3 * sizeof(float), 0},
    };
    u32 stride = 5 * sizeof(float);
    m_inputLayout = m_device.InputLayoutCreate(
        sizeof attribs / sizeof attribs[0],
        attribs,
        1, // nVertexBuffers
        &stride
    );

    CreatePSO();
}

RenderTargetDisplay::~RenderTargetDisplay()
{
    m_device.PipelineStateDestroy(m_pipelineStateObj);
    m_device.InputLayoutDestroy(m_inputLayout);
    m_device.BufferDestroy(m_vertexBuf);
    m_device.RenderPassDestroy(m_renderPass);
    m_samplerCache.Release(m_sampler);

    m_samplerCache.UnregisterCallback(&RenderTargetDisplay::SamplerCacheCallback,
                                      (void*)this);

    m_shader->Release();
}

void RenderTargetDisplay::CopyToBackbuffer(
    const GpuViewport& viewport,
    GpuTextureID colorBuf,
    GpuTextureID depthBuf
)
{
    if (m_shader->PollRefreshed())
        CreatePSO();

    ASSERT(GpuDrawItemWriter::SizeInBytes(GetDrawItemDesc()) == sizeof m_drawItem);
    GpuDrawItemWriter writer;
    writer.Begin(&m_device, GetDrawItemDesc(), m_drawItem);
    writer.SetTexture(0, colorBuf);
    writer.SetTexture(1, depthBuf);
    writer.SetSampler(0, m_sampler);
    writer.SetVertexBuffer(0, m_vertexBuf, 0);
    writer.SetPipelineState(m_pipelineStateObj);
    writer.SetDrawCall(GPU_PRIMITIVE_TRIANGLE_STRIP, 0, 4);
    GpuDrawItem* drawItem = writer.End();

    m_device.Draw(&drawItem, 1, m_renderPass, viewport);

    GPUDEVICE_UNREGISTER_DRAWITEM(m_device, drawItem);
}

void RenderTargetDisplay::CreatePSO()
{
    if (m_pipelineStateObj != 0)
        m_device.PipelineStateDestroy(m_pipelineStateObj);

    GpuPipelineStateDesc pipelineStateDesc;
    pipelineStateDesc.depthCompare = GPU_COMPARE_ALWAYS;
    pipelineStateDesc.depthWritesEnabled = true;
    pipelineStateDesc.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;
    pipelineStateDesc.cullMode = GPU_CULL_BACK;
    pipelineStateDesc.shaderProgram = m_shader->GetGpuShaderProgramID();
    pipelineStateDesc.inputLayout = m_inputLayout;

    m_pipelineStateObj = m_device.PipelineStateCreate(pipelineStateDesc);
}
