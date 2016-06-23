#include "Model/ModelScene.h"

#include "GpuDevice/GpuSamplerCache.h"

#include "Asset/AssetCache.h"
#include "Shader/ShaderAsset.h"
#include "Model/ModelAsset.h"
#include "Model/ModelInstance.h"

static GpuTextureID CreateDefaultWhiteTexture(GpuDevice& device)
{
    GpuTextureID texture = device.TextureCreate(
        GPU_TEXTURE_2D,
        GPU_PIXEL_FORMAT_BGRA8888,
        0, // flags
        1, // width
        1, // height
        1, // depthOrArrayLength
        1 // nMipmapLevels
    );
    const u8 texturePixels[] = {0xFF, 0xFF, 0xFF, 0xFF};
    GpuRegion region = {0, 0, 1, 1};
    device.TextureUpload(texture, region, 0, 4, texturePixels);
    return texture;
}

static GpuInputLayoutID CreateInputLayout(GpuDevice& device)
{
    GpuVertexAttribute attribs[] = {
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, position), 0},
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, normal), 0},
        {GPU_VERTEX_ATTRIB_FLOAT2, offsetof(ModelAsset::Vertex, uv), 0},
    };
    unsigned stride = sizeof(ModelAsset::Vertex);
    return device.InputLayoutCreate(
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

void ModelScene::SamplerCacheCallback(GpuSamplerCache& cache, void* userdata)
{
    ModelScene* self = (ModelScene*)userdata;

    GpuSamplerID oldSamplerUVClamp = self->m_samplerUVClamp;
    GpuSamplerID oldSamplerUVRepeat = self->m_samplerUVRepeat;

    self->m_samplerUVClamp = cache.Acquire(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE);
    self->m_samplerUVRepeat = cache.Acquire(GPU_SAMPLER_ADDRESS_REPEAT);

    for (size_t i = 0; i < self->m_modelInstances.size(); ++i) {
        self->m_modelInstances[i]->RefreshDrawItems();
    }

    cache.Release(oldSamplerUVClamp);
    cache.Release(oldSamplerUVRepeat);
}

ModelScene::ModelScene(
    GpuDevice& device,
    GpuSamplerCache& samplerCache,
    AssetCache<ShaderAsset>& shaderCache
)
    : m_modelInstances()
    , m_device(device)
    , m_samplerCache(samplerCache)
    , m_drawItemPool(m_device, CreateDrawItemWriterDesc())
    , m_modelShader(NULL)
    , m_skyboxShader(NULL)
    , m_sceneCBuffer(0)
    , m_defaultTexture(0)
    , m_samplerUVClamp(samplerCache.Acquire(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE))
    , m_samplerUVRepeat(samplerCache.Acquire(GPU_SAMPLER_ADDRESS_REPEAT))
    , m_inputLayout(0)
    , m_PSOs()
{
    m_modelShader = shaderCache.FindOrLoad("Assets/Shaders/Model_MTL.shd");
    m_modelShader->AddRef();

    m_skyboxShader = shaderCache.FindOrLoad("Assets/Shaders/Skybox_MTL.shd");
    m_skyboxShader->AddRef();

    m_sceneCBuffer = device.BufferCreate(
        GPU_BUFFER_TYPE_CONSTANT,
        GPU_BUFFER_ACCESS_DYNAMIC,
        NULL,
        sizeof(SceneCBuffer)
    );

    m_defaultTexture = CreateDefaultWhiteTexture(device);
    m_inputLayout = CreateInputLayout(device);

    samplerCache.RegisterCallback(&ModelScene::SamplerCacheCallback, (void*)this);
}

ModelScene::~ModelScene()
{
    ASSERT(m_modelInstances.empty() && "ModelInstance not destroyed");

    m_samplerCache.UnregisterCallback(&ModelScene::SamplerCacheCallback, (void*)this);
    m_samplerCache.Release(m_samplerUVClamp);
    m_samplerCache.Release(m_samplerUVRepeat);

    for (int i = 0; i < sizeof m_PSOs / sizeof m_PSOs[0]; ++i) {
        if (m_PSOs[i])
            m_device.PipelineStateDestroy(m_PSOs[i]);
    }
    m_device.TextureDestroy(m_defaultTexture);
    m_device.InputLayoutDestroy(m_inputLayout);
    m_device.BufferDestroy(m_sceneCBuffer);

    m_modelShader->Release();
    m_skyboxShader->Release();
}

void ModelScene::RefreshPSOsMatching(u32 bits, u32 enabled)
{
    GpuPipelineStateID toDestroy[sizeof m_PSOs / sizeof m_PSOs[0]];
    int toDestroyCount = 0;

    for (u32 i = 0; i < sizeof m_PSOs / sizeof m_PSOs[0]; ++i) {
        if ((m_PSOs[i] != 0) && ((i & bits) == enabled)) {
            toDestroy[toDestroyCount] = m_PSOs[i];
            ++toDestroyCount;
            m_PSOs[i] = GpuPipelineStateID(0);
        }
    }

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        m_modelInstances[i]->RefreshDrawItems();
    }

    for (int i = 0; i < toDestroyCount; ++i) {
        m_device.PipelineStateDestroy(toDestroy[i]);
    }
}

ModelInstance* ModelScene::CreateModelInstance(ModelAsset* model, u32 flags)
{
    ModelInstance* instance = new ModelInstance(*this, model, flags);
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

GpuPipelineStateID ModelScene::RequestPSO(u32 flags)
{
    ASSERT(flags < sizeof m_PSOs / sizeof m_PSOs[0]);
    if (!m_PSOs[flags]) {
        GpuPipelineStateDesc desc;
        if (flags & PSOFLAG_SKYBOX)
            desc.shaderProgram = m_skyboxShader->GetGpuShaderProgramID();
        else
            desc.shaderProgram = m_modelShader->GetGpuShaderProgramID();
        desc.shaderStateBitfield = 0;
        desc.inputLayout = m_inputLayout;
        desc.depthCompare = GPU_COMPARE_LESS_EQUAL;
        desc.depthWritesEnabled = true;
        if (flags & PSOFLAG_WIREFRAME)
            desc.fillMode = GPU_FILL_MODE_WIREFRAME;
        else
            desc.fillMode = GPU_FILL_MODE_SOLID;
        desc.cullMode = GPU_CULL_BACK;
        desc.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;
        m_PSOs[flags] = m_device.PipelineStateCreate(desc);
    }
    return m_PSOs[flags];
}

void ModelScene::Update()
{
#ifdef ASSET_REFRESH
    if (m_modelShader->WasJustRefreshed())
        RefreshPSOsMatching(PSOFLAG_SKYBOX, 0);
    if (m_skyboxShader->WasJustRefreshed())
        RefreshPSOsMatching(PSOFLAG_SKYBOX, PSOFLAG_SKYBOX);
#endif
}

GpuSamplerID ModelScene::GetSamplerUVClamp() const
{
    return m_samplerUVClamp;
}

GpuSamplerID ModelScene::GetSamplerUVRepeat() const
{
    return m_samplerUVRepeat;
}

GpuTextureID ModelScene::GetDefaultTexture() const
{
    return m_defaultTexture;
}

GpuDrawItemPool& ModelScene::GetDrawItemPool()
{
    return m_drawItemPool;
}

GpuBufferID ModelScene::GetSceneCBuffer() const
{
    return m_sceneCBuffer;
}

GpuDevice& ModelScene::GetGpuDevice() const
{
    return m_device;
}
