#include "Model/ModelScene.h"

#include "GpuDevice/GpuSamplerCache.h"

#include "Shader/ShaderAsset.h"
#include "Model/ModelShared.h"
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
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelShared::Vertex, position), 0},
        {GPU_VERTEX_ATTRIB_FLOAT3, offsetof(ModelShared::Vertex, normal), 0},
        {GPU_VERTEX_ATTRIB_FLOAT2, offsetof(ModelShared::Vertex, uv), 0},
    };
    unsigned stride = sizeof(ModelShared::Vertex);
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

    for (ModelInstance* instance = self->m_modelInstances.Head(); instance;
         instance = instance->m_link.Next()) {
        instance->RecreateDrawItems();
    }

    cache.Release(oldSamplerUVClamp);
    cache.Release(oldSamplerUVRepeat);
}

ModelScene::ModelScene(
    GpuDevice& device,
    FileLoader& loader,
    GpuSamplerCache& samplerCache,
    ShaderCache& shaderCache,
    TextureCache& textureCache
)
    : m_device(device)
    , m_fileLoader(loader)
    , m_samplerCache(samplerCache)
    , m_textureCache(textureCache)

    , m_modelInstances()
    , m_modelCache()

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
    m_modelShader = shaderCache.FindOrLoad("Shaders\\Model");
    m_modelShader->AddRef();

    m_skyboxShader = shaderCache.FindOrLoad("Shaders\\Skybox");
    m_skyboxShader->AddRef();

    m_sceneCBuffer = device.BufferCreate(
        GPU_BUFFER_TYPE_CONSTANT,
        GPU_BUFFER_ACCESS_STREAM,
        NULL,
        sizeof(SceneCBuffer),
        0 // maxUpdatesPerFrame (unused)
    );

    m_defaultTexture = CreateDefaultWhiteTexture(device);
    m_inputLayout = CreateInputLayout(device);

    samplerCache.RegisterCallback(&ModelScene::SamplerCacheCallback, (void*)this);
}

ModelScene::~ModelScene()
{
    ASSERT(m_modelInstances.Empty() && "ModelInstance not destroyed");

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

    for (ModelInstance* instance = m_modelInstances.Head(); instance;
         instance = instance->m_link.Next()) {
        instance->RecreateDrawItems();
    }

    for (int i = 0; i < toDestroyCount; ++i) {
        m_device.PipelineStateDestroy(toDestroy[i]);
    }
}

ModelInstance* ModelScene::CreateModelInstance(const char* path, u32 flags)
{
    ModelShared* shared = m_modelCache.Get(
        m_device,
        m_textureCache,
        m_fileLoader,
        path
    );
    ModelInstance* instance = new ModelInstance(*this, shared, flags);
    ModelInstance* firstInGroup = shared->GetFirstInstance();
    if (firstInGroup != NULL) {
        m_modelInstances.InsertBefore(instance, firstInGroup);
    } else {
        m_modelInstances.InsertHead(instance);
        instance->MarkLastInAssetGroup();
    }
    shared->SetFirstInstance(instance);

    return instance;
}

void ModelScene::DestroyModelInstance(ModelInstance* instance)
{
    delete instance;
}

void ModelScene::Reload(const char* path)
{
    m_modelCache.Reload(m_device, m_textureCache, m_fileLoader, path);
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
    if (m_modelShader->PollRefreshed())
        RefreshPSOsMatching(PSOFLAG_SKYBOX, 0);
    if (m_skyboxShader->PollRefreshed())
        RefreshPSOsMatching(PSOFLAG_SKYBOX, PSOFLAG_SKYBOX);
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
