#include "Model/ModelRenderQueue.h"

#include "Core/Macros.h"
#include "File.h"

#include "Math/Vector3.h"
#include "Math/Matrix44.h"

#include "GpuDevice/GpuMathUtils.h"

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

static GpuShaderID LoadShader(GpuDevice* dev, GpuShaderType type, const char* path)
{
    std::vector<char> data;
    if (!FileReadFile(path, data))
        FATAL("Failed to read shader %s", path);
    // subtract 1 because we don't want to include the null terminator
    size_t length = data.size() - 1;
    GpuShaderID shader = dev->CreateShader(type, &data[0], length);
    return shader;
}

ModelRenderQueue::ModelRenderQueue(GpuDevice* device)
    : m_modelInstances()
    , m_drawItems()
    , m_device(device)
    , m_sceneCBuffer(0)
    , m_vertexShader(0)
    , m_pixelShader(0)
    , m_inputLayout(0)
    , m_pipelineStateObj(0)
{
    m_sceneCBuffer = device->CreateBuffer(GPUBUFFERTYPE_CONSTANT,
                                          GPUBUFFER_ACCESS_DYNAMIC,
                                          NULL,
                                          sizeof(ModelSceneCBuffer));
    m_vertexShader = LoadShader(device, GPUSHADERTYPE_VERTEX, "Assets/Shaders/ModelVS.metallib");
    m_pixelShader = LoadShader(device, GPUSHADERTYPE_PIXEL, "Assets/Shaders/ModelPS.metallib");

    GpuVertexAttribute attribs[] = {
        {GPUVERTEXATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, position), 0},
        {GPUVERTEXATTRIB_FLOAT3, offsetof(ModelAsset::Vertex, normal), 0},
    };
    unsigned stride = sizeof(ModelAsset::Vertex);
    m_inputLayout = device->CreateInputLayout(sizeof attribs / sizeof attribs[0],
                                              attribs,
                                              1,
                                              &stride);

    GpuPipelineStateDesc pipelineState;
    pipelineState.vertexShader = m_vertexShader;
    pipelineState.pixelShader = m_pixelShader;
    pipelineState.inputLayout = m_inputLayout;
    pipelineState.depthCompare = GPU_COMPARE_LESS_EQUAL;
    pipelineState.depthWritesEnabled = true;
    pipelineState.cullMode = GPU_CULL_BACK;
    pipelineState.frontFaceWinding = GPU_WINDING_COUNTER_CLOCKWISE;
    m_pipelineStateObj = device->CreatePipelineStateObject(pipelineState);
}

ModelRenderQueue::~ModelRenderQueue()
{
    m_device->DestroyPipelineStateObject(m_pipelineStateObj);
    m_device->DestroyInputLayout(m_inputLayout);
    m_device->DestroyBuffer(m_sceneCBuffer);
    m_device->DestroyShader(m_vertexShader);
    m_device->DestroyShader(m_pixelShader);
}

ModelInstance* ModelRenderQueue::CreateModelInstance(std::shared_ptr<ModelAsset> model)
{
    ModelInstanceCreateContext ctx;
    ctx.model = model;
    ctx.pipelineObject = m_pipelineStateObj;
    ctx.sceneCBuffer = m_sceneCBuffer;
    return ModelInstance::Create(ctx);
}

void ModelRenderQueue::Clear()
{
    m_modelInstances.clear();
    m_drawItems.clear();
}

void ModelRenderQueue::Add(ModelInstance* instance)
{
    m_modelInstances.push_back(instance);
    m_drawItems.push_back(instance->GetDrawItem());
}

void ModelRenderQueue::Draw(const SceneInfo& sceneInfo,
                            const GpuViewport& viewport,
                            GpuRenderPassID renderPass)
{
    ModelSceneCBuffer* sceneCBuf = (ModelSceneCBuffer*)m_device->GetBufferContents(m_sceneCBuffer);
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
    m_device->FlushBufferRange(m_sceneCBuffer, 0, sizeof(ModelSceneCBuffer));

    m_device->Draw(&m_drawItems[0], (int)m_drawItems.size(), renderPass, viewport);
}
