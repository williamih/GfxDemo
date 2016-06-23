#include "Scene/RenderTargetDisplay.h"

#include "GpuDevice/GpuDrawItemWriter.h"

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

RenderTargetDisplay::RenderTargetDisplay(GpuDevice& device,
                                         AssetCache<ShaderAsset>& shaderCache)
    : m_device(device)
    , m_shader(shaderCache.FindOrLoad("Assets/Shaders/BlitRT_MTL.shd"))
    , m_renderPass()
    , m_vertexBuf()
    , m_inputLayout()
    , m_sampler()
    , m_pipelineStateObj()
    , m_drawItem(NULL)
{
    m_drawItem = malloc(GpuDrawItemWriter::SizeInBytes(GetDrawItemDesc()));

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

    GpuSamplerDesc samplerDesc;
    samplerDesc.minFilter = GPU_SAMPLER_FILTER_LINEAR;
    samplerDesc.magFilter = GPU_SAMPLER_FILTER_LINEAR;
    samplerDesc.mipFilter = GPU_SAMPLER_MIPFILTER_NOT_MIPMAPPED;
    samplerDesc.uAddressMode = GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.vAddressMode = GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.wAddressMode = GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE;
    m_sampler = m_device.SamplerCreate(samplerDesc);

    CreatePSO();
}

RenderTargetDisplay::~RenderTargetDisplay()
{
    free(m_drawItem);
    m_device.PipelineStateDestroy(m_pipelineStateObj);
    m_device.SamplerDestroy(m_sampler);
    m_device.InputLayoutDestroy(m_inputLayout);
    m_device.BufferDestroy(m_vertexBuf);
    m_device.RenderPassDestroy(m_renderPass);
}

static void* Alloc(size_t, void* userdata)
{
    return userdata;
}

void RenderTargetDisplay::CopyToBackbuffer(
    const GpuViewport& viewport,
    GpuTextureID colorBuf,
    GpuTextureID depthBuf
)
{
#ifdef ASSET_REFRESH
    if (m_shader->WasJustRefreshed())
        CreatePSO();
#endif

    GpuDrawItemWriter writer;
    writer.Begin(&m_device, GetDrawItemDesc(), Alloc, m_drawItem);
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
