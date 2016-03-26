#include "Scene/Scene.h"

#include <math.h>

#include "Asset/AssetCache.h"

#include "Model/ModelAsset.h"
#include "Model/ModelInstance.h"

const int MAX_ANISOTROPY = 16;
static const Vector3 s_dirToLight(0.0f, 0.0f, 1.0f);
static const Vector3 s_irradiance(1.0f, 1.0f, 1.0f);
static const Vector3 s_ambientRadiance(0.3f, 0.3f, 0.3f);

Scene::Scene(
    GpuDevice* device,
    AssetCache<ShaderAsset>& shaderCache,
    AssetCache<ModelAsset>& modelCache
)
    : m_device(device)
    , m_modelCache(modelCache)

    , m_modelScene(device, shaderCache)
    , m_modelRenderQueue()
    , m_modelInstances(NULL)
    , m_skybox(NULL)

    , m_renderPass()

    , m_cameraPos()
    , m_forward()
    , m_right()
    , m_up()
    , m_zNear(0.0f)
    , m_zFar(0.0f)
    , m_fovY(0.0f)
{
    m_modelScene.SetMaxAnisotropy(MAX_ANISOTROPY);

    GpuRenderPassDesc renderPassDesc;
    renderPassDesc.flags |= GpuRenderPassDesc::FLAG_PERFORM_CLEAR;
    renderPassDesc.clearR = 0.0f;
    renderPassDesc.clearB = 0.0f;
    renderPassDesc.clearG = 0.0f;
    renderPassDesc.clearA = 0.0f;
    renderPassDesc.clearDepth = 1.0f;
    m_renderPass = device->RenderPassCreate(renderPassDesc);
}

Scene::~Scene()
{
    m_device->RenderPassDestroy(m_renderPass);

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        m_modelScene.DestroyModelInstance(m_modelInstances[i]);
    }
    if (m_skybox)
        m_modelScene.DestroyModelInstance(m_skybox);
}

ModelInstance* Scene::LoadModel(const char* path, u32 flags)
{
    return m_modelScene.CreateModelInstance(m_modelCache.FindOrLoad(path), flags);
}

void Scene::SetSkybox(const char* path)
{
    if (m_skybox)
        m_modelScene.DestroyModelInstance(m_skybox);
    m_skybox = LoadModel("Assets/Models/Skybox.mdl", ModelInstance::FLAG_SKYBOX);
    m_skybox->Update(Matrix44(), Vector3(1.0f, 1.0f, 1.0f), Vector3(), 1.0f);
}

ModelInstance* Scene::AddModelInstance(const char* path)
{
    m_modelInstances.push_back(LoadModel(path, 0));
    return m_modelInstances.back();
}

void Scene::Update(const SceneUpdateInfo& info)
{
    m_cameraPos = info.cameraPos;
    m_forward = info.forward;
    m_right = info.right;
    m_up = info.up;
    m_zNear = info.zNear;
    m_zFar = info.zFar;
    m_fovY = info.fovY;
}

static Matrix44 ZUpToYUpMatrix()
{
    return Matrix44(1.0f,  0.0f, 0.0f, 0.0f,
                    0.0f,  0.0f, 1.0f, 0.0f,
                    0.0f, -1.0f, 0.0f, 0.0f,
                    0.0f,  0.0f, 0.0f, 1.0f);
}

static Matrix44 CreatePerspectiveMatrix(float aspect, float fovY, float zNear, float zFar)
{
    float tanHalfFovY = tanf(fovY * 0.5f);
    float top = tanHalfFovY * zNear;
    float bot = -top;
    float right = top * aspect;
    float left = -right;
    return GpuDevice::TransformCreatePerspective(left, right, bot, top, zNear, zFar);
}

void Scene::Render(const GpuViewport& viewport)
{
    float aspect = (float)viewport.width / (float)viewport.height;
    Matrix44 projTransform = CreatePerspectiveMatrix(aspect, m_fovY, m_zNear, m_zFar);

    Matrix44 cameraToWorld(
        m_right.x, m_forward.x, m_up.x, m_cameraPos.x,
        m_right.y, m_forward.y, m_up.y, m_cameraPos.y,
        m_right.z, m_forward.z, m_up.z, m_cameraPos.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    Matrix44 viewTransform = ZUpToYUpMatrix() * cameraToWorld.AffineInverse();

    ModelRenderQueue::SceneInfo info;
    info.viewProjTransform = projTransform * viewTransform;
    info.cameraPos = m_cameraPos;
    info.dirToLight = s_dirToLight;
    info.irradiance = s_irradiance;
    info.ambientRadiance = s_ambientRadiance;

    m_modelScene.Update();

    m_modelRenderQueue.Clear();
    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        m_modelRenderQueue.Add(m_modelInstances[i]);
    }
    if (m_skybox)
        m_modelRenderQueue.Add(m_skybox);
    m_modelRenderQueue.Draw(m_modelScene, info, viewport, m_renderPass);
}
