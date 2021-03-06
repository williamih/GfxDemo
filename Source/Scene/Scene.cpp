#include "Scene/Scene.h"

#include <math.h>

#include "Model/ModelInstance.h"

static const Vector3 s_dirToLight(0.0f, 0.0f, 1.0f);
static const Vector3 s_irradiance(1.0f, 1.0f, 1.0f);
static const Vector3 s_ambientRadiance(0.3f, 0.3f, 0.3f);

Scene::Scene(
    GpuDevice& device,
    FileLoader& loader,
    GpuSamplerCache& samplerCache,
    ShaderCache& shaderCache,
    TextureCache& textureCache
)
    : m_device(device)
    , m_renderTargetDisplay(device, samplerCache, shaderCache)

    , m_modelScene(device, loader, samplerCache, shaderCache, textureCache)
    , m_modelRenderQueue()
    , m_modelInstances(NULL)
    , m_skybox(NULL)

    , m_colorRenderTarget()
    , m_depthRenderTarget()
    , m_renderPass()

    , m_cameraPos()
    , m_forward()
    , m_right()
    , m_up()
    , m_zNear(0.0f)
    , m_zFar(0.0f)
    , m_fovY(0.0f)
{
    m_colorRenderTarget = device.TextureCreate(
        GPU_TEXTURE_2D,
        GPU_PIXEL_FORMAT_BGRA8888,
        GPU_TEXTURE_FLAG_RENDER_TARGET,
        device.GetFormat().resolutionX, // width
        device.GetFormat().resolutionY, // height
        1, // depthOrArrayLength
        1 // nMipmapLevels
    );
    m_depthRenderTarget = device.TextureCreate(
        GPU_TEXTURE_2D,
        GPU_PIXEL_FORMAT_DEPTH_32,
        GPU_TEXTURE_FLAG_RENDER_TARGET,
        device.GetFormat().resolutionX, // width
        device.GetFormat().resolutionY, // height
        1, // depthOrArrayLength
        1 // nMipmapLevels
    );

    GpuRenderLoadAction colorLoadAction = GPU_RENDER_LOAD_ACTION_DISCARD;
    GpuRenderStoreAction colorStoreAction = GPU_RENDER_STORE_ACTION_STORE;
    GpuColor clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
    GpuRenderPassDesc renderPassDesc;
    renderPassDesc.clearColors = &clearColor;
    renderPassDesc.colorLoadActions = &colorLoadAction;
    renderPassDesc.colorStoreActions = &colorStoreAction;
    renderPassDesc.clearDepth = 1.0f;
    renderPassDesc.numRenderTargets = 1;
    renderPassDesc.renderTargets = &m_colorRenderTarget;
    renderPassDesc.depthStencilTarget = m_depthRenderTarget;
    renderPassDesc.depthStencilLoadAction = GPU_RENDER_LOAD_ACTION_CLEAR;
    renderPassDesc.depthStencilStoreAction = GPU_RENDER_STORE_ACTION_STORE;
    m_renderPass = device.RenderPassCreate(renderPassDesc);
}

Scene::~Scene()
{
    m_device.TextureDestroy(m_colorRenderTarget);
    m_device.TextureDestroy(m_depthRenderTarget);
    m_device.RenderPassDestroy(m_renderPass);

    for (size_t i = 0; i < m_modelInstances.size(); ++i) {
        m_modelScene.DestroyModelInstance(m_modelInstances[i]);
    }
    if (m_skybox)
        m_modelScene.DestroyModelInstance(m_skybox);
}

void Scene::SetSkybox(const char* path)
{
    if (m_skybox)
        m_modelScene.DestroyModelInstance(m_skybox);
    m_skybox = m_modelScene.CreateModelInstance("Models\\Skybox.mdl",
                                                ModelInstance::FLAG_SKYBOX);
    m_skybox->Update(Matrix44(), Vector3(1.0f, 1.0f, 1.0f), Vector3(), 1.0f);
}

ModelInstance* Scene::AddModelInstance(const char* path)
{
    m_modelInstances.push_back(m_modelScene.CreateModelInstance(path, 0));
    return m_modelInstances.back();
}

void Scene::RefreshModel(const char* path)
{
    m_modelScene.Reload(path);
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

    m_renderTargetDisplay.CopyToBackbuffer(
        viewport, m_colorRenderTarget, m_depthRenderTarget
    );
}
