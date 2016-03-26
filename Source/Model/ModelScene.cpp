#include "Model/ModelScene.h"

#include "Asset/AssetCache.h"
#include "Shader/ShaderAsset.h"
#include "Model/ModelAsset.h"
#include "Model/ModelInstance.h"

static GpuTextureID CreateDefaultWhiteTexture(GpuDevice* device)
{
    GpuTextureID texture = device->TextureCreate(
        GPU_TEXTURE_2D,
        GPU_PIXEL_FORMAT_BGRA8888,
        1, // width
        1, // height
        1, // depthOrArrayLength
        1 // nMipmapLevels
    );
    const u8 texturePixels[] = {0xFF, 0xFF, 0xFF, 0xFF};
    GpuRegion region = {0, 0, 1, 1};
    device->TextureUpload(texture, region, 0, 4, texturePixels);
    return texture;
}

static GpuSamplerID CreateSampler(
    GpuDevice* device,
    GpuSamplerAddressMode uvAddressMode,
    int maxAnisotropy
)
{
    GpuSamplerDesc desc;
    desc.uAddressMode = uvAddressMode;
    desc.vAddressMode = uvAddressMode;
    desc.wAddressMode = GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE;
    desc.minFilter = GPU_SAMPLER_FILTER_LINEAR;
    desc.magFilter = GPU_SAMPLER_FILTER_LINEAR;
    desc.mipFilter = GPU_SAMPLER_MIPFILTER_LINEAR;
    desc.maxAnisotropy = maxAnisotropy;
    return device->SamplerCreate(desc);
}

static GpuInputLayoutID CreateInputLayout(GpuDevice* device)
{
    GpuVertexAttribute attribs[] = {
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, position), 0},
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, normal), 0},
        {GPU_VERTEX_ATTRIB_FLOAT2, offsetof(ModelAsset::Vertex, uv), 0},
    };
    unsigned stride = sizeof(ModelAsset::Vertex);
    return device->InputLayoutCreate(
        sizeof attribs / sizeof attribs[0],
        attribs,
        1,
        &stride
    );
}

static GpuDrawItemWriterDesc CreateDrawItemWriterDesc()
{
    GpuDrawItemWriterDesc desc;
    desc.SetNumCBuffers(2);
    desc.SetNumVertexBuffers(1);
    desc.SetNumTextures(1);
    desc.SetNumSamplers(1);
    return desc;
}

ModelScene::ModelScene(GpuDevice* device,
                       AssetCache<ShaderAsset>& shaderCache)
    : m_modelInstances()
    , m_device(device)
    , m_drawItemPool(m_device, CreateDrawItemWriterDesc())
    , m_modelShader(NULL)
    , m_skyboxShader(NULL)
    , m_sceneCBuffer(0)
    , m_defaultTexture(0)
    , m_samplerUVClamp(0)
    , m_samplerUVRepeat(0)
    , m_inputLayout(0)
    , m_modelPSO(0)
    , m_skyboxPSO(0)
{
    m_modelShader = shaderCache.FindOrLoad("Assets/Shaders/Model_MTL.shd");
    m_modelShader->AddRef();

    m_skyboxShader = shaderCache.FindOrLoad("Assets/Shaders/Skybox_MTL.shd");
    m_skyboxShader->AddRef();

    m_sceneCBuffer = device->BufferCreate(
        GPU_BUFFER_TYPE_CONSTANT,
        GPU_BUFFER_ACCESS_DYNAMIC,
        NULL,
        sizeof(SceneCBuffer)
    );

    m_defaultTexture = CreateDefaultWhiteTexture(device);
    m_samplerUVClamp = CreateSampler(device, GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE, 1);
    m_samplerUVRepeat = CreateSampler(device, GPU_SAMPLER_ADDRESS_REPEAT, 1);
    m_inputLayout = CreateInputLayout(device);

    m_modelPSO = CreateModelPSO();
    m_skyboxPSO = CreateSkyboxPSO();
}

ModelScene::~ModelScene()
{
    ASSERT(m_modelInstances.empty() && "ModelInstance not destroyed");

    m_device->PipelineStateDestroy(m_modelPSO);
    m_device->PipelineStateDestroy(m_skyboxPSO);
    m_device->SamplerDestroy(m_samplerUVClamp);
    m_device->SamplerDestroy(m_samplerUVRepeat);
    m_device->TextureDestroy(m_defaultTexture);
    m_device->InputLayoutDestroy(m_inputLayout);
    m_device->BufferDestroy(m_sceneCBuffer);

    m_modelShader->Release();
    m_skyboxShader->Release();
}

GpuPipelineStateID ModelScene::CreateModelPSO()
{
    GpuPipelineStateDesc pipelineState;
    pipelineState.shaderProgram = m_modelShader->GetGpuShaderProgramID();
    pipelineState.shaderStateBitfield = 0;
    pipelineState.inputLayout = m_inputLayout;
    pipelineState.depthCompare = GPU_COMPARE_LESS_EQUAL;
    pipelineState.depthWritesEnabled = true;
    pipelineState.cullMode = GPU_CULL_BACK;
    pipelineState.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;

    return m_device->PipelineStateCreate(pipelineState);
}

GpuPipelineStateID ModelScene::CreateSkyboxPSO()
{
    GpuPipelineStateDesc pipelineState;
    pipelineState.shaderProgram = m_skyboxShader->GetGpuShaderProgramID();
    pipelineState.shaderStateBitfield = 0;
    pipelineState.inputLayout = m_inputLayout;
    pipelineState.depthCompare = GPU_COMPARE_LESS_EQUAL;
    pipelineState.depthWritesEnabled = true;
    pipelineState.cullMode = GPU_CULL_BACK;
    pipelineState.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;

    return m_device->PipelineStateCreate(pipelineState);
}

void ModelScene::RefreshModelPSO()
{
    GpuPipelineStateID newPipelineState = CreateModelPSO();

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        ModelInstanceCreateContext ctx;
        ctx.drawItemPool = &m_drawItemPool;
        ctx.sceneCBuffer = m_sceneCBuffer;
        ctx.defaultTexture = m_defaultTexture;
        ctx.samplerUVClamp = m_samplerUVClamp;
        ctx.samplerUVRepeat = m_samplerUVRepeat;
        ctx.modelPSO = newPipelineState;
        ctx.skyboxPSO = m_skyboxPSO;
        m_modelInstances[i]->RefreshDrawItems(ctx);
    }

    if (m_modelPSO != 0)
        m_device->PipelineStateDestroy(m_modelPSO);
    m_modelPSO = newPipelineState;
}

void ModelScene::RefreshSkyboxPSO()
{
    GpuPipelineStateID newPipelineState = CreateSkyboxPSO();

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        ModelInstanceCreateContext ctx;
        ctx.drawItemPool = &m_drawItemPool;
        ctx.sceneCBuffer = m_sceneCBuffer;
        ctx.defaultTexture = m_defaultTexture;
        ctx.samplerUVClamp = m_samplerUVClamp;
        ctx.samplerUVRepeat = m_samplerUVRepeat;
        ctx.modelPSO = m_modelPSO;
        ctx.skyboxPSO = m_skyboxPSO;
        m_modelInstances[i]->RefreshDrawItems(ctx);
    }

    if (m_skyboxPSO != 0)
        m_device->PipelineStateDestroy(m_skyboxPSO);
    m_skyboxPSO = newPipelineState;
}

ModelInstance* ModelScene::CreateModelInstance(ModelAsset* model, u32 flags)
{
    ModelInstanceCreateContext ctx;
    ctx.drawItemPool = &m_drawItemPool;
    ctx.sceneCBuffer = m_sceneCBuffer;
    ctx.defaultTexture = m_defaultTexture;
    ctx.samplerUVClamp = m_samplerUVClamp;
    ctx.samplerUVRepeat = m_samplerUVRepeat;
    ctx.modelPSO = m_modelPSO;
    ctx.skyboxPSO = m_skyboxPSO;
    ModelInstance* instance = new ModelInstance(model, flags, ctx);
    m_modelInstances.push_back(instance);
    return instance;
}

void ModelScene::DestroyModelInstance(ModelInstance* instance)
{
    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        if (m_modelInstances[i] == instance) {
            m_modelInstances[i] = m_modelInstances.back();
            m_modelInstances.pop_back();
            delete instance;
            return;
        }
    }
    ASSERT(!"ModelInstance not found in scene");
}

void ModelScene::SetMaxAnisotropy(int maxAnisotropy)
{
    GpuSamplerID samplerUVClamp = CreateSampler(
        m_device, GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE, maxAnisotropy
    );
    GpuSamplerID samplerUVRepeat = CreateSampler(
        m_device, GPU_SAMPLER_ADDRESS_REPEAT, maxAnisotropy
    );
    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        ModelInstanceCreateContext ctx;
        ctx.drawItemPool = &m_drawItemPool;
        ctx.sceneCBuffer = m_sceneCBuffer;
        ctx.defaultTexture = m_defaultTexture;
        ctx.samplerUVClamp = samplerUVClamp;
        ctx.samplerUVRepeat = samplerUVRepeat;
        ctx.modelPSO = m_modelPSO;
        ctx.skyboxPSO = m_skyboxPSO;
        m_modelInstances[i]->RefreshDrawItems(ctx);
    }
    m_device->SamplerDestroy(m_samplerUVClamp);
    m_device->SamplerDestroy(m_samplerUVRepeat);
    m_samplerUVClamp = samplerUVClamp;
    m_samplerUVRepeat = samplerUVRepeat;
}

GpuDevice* ModelScene::GetGpuDevice() const
{
    return m_device;
}

GpuBufferID ModelScene::GetSceneCBuffer() const
{
    return m_sceneCBuffer;
}

void ModelScene::Update()
{
#ifdef ASSET_REFRESH
    if (m_modelShader->WasJustRefreshed())
        RefreshModelPSO();
    if (m_skyboxShader->WasJustRefreshed())
        RefreshSkyboxPSO();
#endif
}
