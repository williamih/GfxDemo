#include "Model/ModelRenderQueue.h"

#include "Core/Macros.h"

#include "Math/Vector3.h"
#include "Math/Matrix44.h"

#include "GpuDevice/GpuMathUtils.h"
#include "GpuDevice/GpuDeferredDeletionQueue.h"

#include "Shader/ShaderAsset.h"

#include "Model/ModelAsset.h"
#include "Model/ModelInstance.h"

const float PI = 3.141592654f;

struct ModelSceneCBuffer {
    float viewProjTransform[4][4];
    float cameraPos[4];
    float dirToLight[4];
    float irradiance_over_pi[4];
    float ambientRadiance[4];
};

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

static GpuSamplerID CreateSampler(GpuDevice* device, int maxAnisotropy)
{
    GpuSamplerDesc desc;
    desc.uAddressMode = GPU_SAMPLER_ADDRESS_REPEAT;
    desc.vAddressMode = GPU_SAMPLER_ADDRESS_REPEAT;
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

ModelRenderQueue::ModelRenderQueue(GpuDevice* device,
                                   AssetCache<ShaderAsset>& shaderCache)
    : m_drawItems()
    , m_modelInstances()
    , m_device(device)
    , m_drawItemPool(m_device, CreateDrawItemWriterDesc())
    , m_shaderAsset()
    , m_sceneCBuffer(0)
    , m_defaultTexture(0)
    , m_sampler(0)
    , m_inputLayout(0)
    , m_pipelineStateObj(0)
{
    m_shaderAsset = shaderCache.FindOrLoad("Assets/Shaders/Model_MTL.shd");
    m_shaderAsset->AddRef();

    m_sceneCBuffer = device->BufferCreate(GPU_BUFFER_TYPE_CONSTANT,
                                          GPU_BUFFER_ACCESS_DYNAMIC,
                                          NULL,
                                          sizeof(ModelSceneCBuffer));

    m_defaultTexture = CreateDefaultWhiteTexture(device);
    m_sampler = CreateSampler(device, 1);
    m_inputLayout = CreateInputLayout(device);

    RefreshPipelineStateObject();
}

ModelRenderQueue::~ModelRenderQueue()
{
    m_device->PipelineStateDestroy(m_pipelineStateObj);
    m_device->SamplerDestroy(m_sampler);
    m_device->TextureDestroy(m_defaultTexture);
    m_device->InputLayoutDestroy(m_inputLayout);
    m_device->BufferDestroy(m_sceneCBuffer);

    m_shaderAsset->Release();
}

void ModelRenderQueue::RefreshPipelineStateObject()
{
    GpuPipelineStateDesc pipelineState;
    pipelineState.shaderProgram = m_shaderAsset->GetGpuShaderProgramID();
    pipelineState.shaderStateBitfield = 0;
    pipelineState.inputLayout = m_inputLayout;
    pipelineState.depthCompare = GPU_COMPARE_LESS_EQUAL;
    pipelineState.depthWritesEnabled = true;
    pipelineState.cullMode = GPU_CULL_BACK;
    pipelineState.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;

    GpuPipelineStateID newPipelineState = m_device->PipelineStateCreate(pipelineState);

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        ModelInstanceCreateContext ctx;
        ctx.drawItemPool = &m_drawItemPool;
        ctx.pipelineObject = newPipelineState;
        ctx.sceneCBuffer = m_sceneCBuffer;
        ctx.defaultTexture = m_defaultTexture;
        ctx.sampler = m_sampler;
        m_modelInstances[i]->RefreshDrawItems(ctx);
    }

    if (m_pipelineStateObj != 0)
        m_device->PipelineStateDestroy(m_pipelineStateObj);
    m_pipelineStateObj = newPipelineState;
}

ModelInstance* ModelRenderQueue::CreateModelInstance(ModelAsset* model)
{
    ModelInstanceCreateContext ctx;
    ctx.drawItemPool = &m_drawItemPool;
    ctx.pipelineObject = m_pipelineStateObj;
    ctx.sceneCBuffer = m_sceneCBuffer;
    ctx.defaultTexture = m_defaultTexture;
    ctx.sampler = m_sampler;
    ModelInstance* instance = new ModelInstance(model, ctx);
    m_modelInstances.push_back(instance);
    return instance;
}

void ModelRenderQueue::SetMaxAnisotropy(int maxAnisotropy)
{
    GpuSamplerID sampler = CreateSampler(m_device, maxAnisotropy);
    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        ModelInstanceCreateContext ctx;
        ctx.drawItemPool = &m_drawItemPool;
        ctx.pipelineObject = m_pipelineStateObj;
        ctx.sceneCBuffer = m_sceneCBuffer;
        ctx.defaultTexture = m_defaultTexture;
        ctx.sampler = sampler;
        m_modelInstances[i]->RefreshDrawItems(ctx);
    }
    m_device->SamplerDestroy(m_sampler);
    m_sampler = sampler;
}

void ModelRenderQueue::Clear()
{
    m_drawItems.clear();

#ifdef ASSET_REFRESH
    if (m_shaderAsset->WasJustRefreshed())
        RefreshPipelineStateObject();
#endif
}

void ModelRenderQueue::Add(ModelInstance* instance)
{
    instance->AddDrawItemsToList(m_drawItems);
}

void ModelRenderQueue::Draw(const SceneInfo& sceneInfo,
                            const GpuViewport& viewport,
                            GpuRenderPassID renderPass)
{
    ModelSceneCBuffer* sceneCBuf = (ModelSceneCBuffer*)m_device->BufferGetContents(m_sceneCBuffer);
    GpuMathUtils::FillArrayColumnMajor(sceneInfo.viewProjTransform,
                                       sceneCBuf->viewProjTransform);
    sceneCBuf->cameraPos[0] = sceneInfo.cameraPos.x;
    sceneCBuf->cameraPos[1] = sceneInfo.cameraPos.y;
    sceneCBuf->cameraPos[2] = sceneInfo.cameraPos.z;
    sceneCBuf->cameraPos[3] = 0.0f;
    sceneCBuf->dirToLight[0] = sceneInfo.dirToLight.x;
    sceneCBuf->dirToLight[1] = sceneInfo.dirToLight.y;
    sceneCBuf->dirToLight[2] = sceneInfo.dirToLight.z;
    sceneCBuf->dirToLight[3] = 0.0f;
    sceneCBuf->irradiance_over_pi[0] = sceneInfo.irradiance.x / PI;
    sceneCBuf->irradiance_over_pi[1] = sceneInfo.irradiance.y / PI;
    sceneCBuf->irradiance_over_pi[2] = sceneInfo.irradiance.z / PI;
    sceneCBuf->irradiance_over_pi[3] = 0.0f;
    sceneCBuf->ambientRadiance[0] = sceneInfo.ambientRadiance.x;
    sceneCBuf->ambientRadiance[1] = sceneInfo.ambientRadiance.y;
    sceneCBuf->ambientRadiance[2] = sceneInfo.ambientRadiance.z;
    sceneCBuf->ambientRadiance[3] = 0.0f;
    m_device->BufferFlushRange(m_sceneCBuffer, 0, sizeof(ModelSceneCBuffer));

    m_device->Draw(&m_drawItems[0], (int)m_drawItems.size(), renderPass, viewport);
}
