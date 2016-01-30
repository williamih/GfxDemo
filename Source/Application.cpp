#include "Application.h"

#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include "Core/File.h"

#include "Math/Vector3.h"

#include "GpuDevice/GpuDrawItemWriter.h"
#include "GpuDevice/GpuDeferredDeletionQueue.h"

#include "Shader/ShaderAsset.h"

#include "Texture/TextureAsset.h"

#include "Model/ModelAsset.h"

#include "OsWindow.h"

static void OnWindowResize(const OsEvent& event, void* userdata)
{
    ((GpuDevice*)userdata)->OnWindowResized();
}

static void OnPaint(const OsEvent& event, void* userdata)
{
    ((Application*)userdata)->Frame();
}

static void OnKeyDown(const OsEvent& event, void* userdata)
{
    if (event.key.code == OSKEY_R)
        ((Application*)userdata)->RefreshModelShader();
}

OsWindow* Application::CreateWindow()
{
    OsWindowPixelFormat pf;
    pf.colorBits = 32;
    pf.depthBits = 32;

    return OsWindow::Create(800.0f, 600.0f, pf);
}

GpuDevice* Application::CreateGpuDevice(OsWindow& window)
{
    GpuDeviceFormat deviceFormat;
    deviceFormat.pixelColorFormat = GPU_PIXEL_COLOR_FORMAT_RGBA8888;
    deviceFormat.pixelDepthFormat = GPU_PIXEL_DEPTH_FORMAT_FLOAT32;
    deviceFormat.resolutionX = 1600;
    deviceFormat.resolutionY = 1200;
    deviceFormat.flags = GpuDeviceFormat::FLAG_SCALE_RES_WITH_WINDOW_SIZE;
    return GpuDevice::Create(deviceFormat, window.GetNSView());
}

ModelInstance* Application::CreateModelInstance(ModelRenderQueue& queue,
                                                AssetCache<ModelAsset>& cache,
                                                const char* path)
{
    std::shared_ptr<ModelAsset> model(cache.FindOrLoad(path));
    return queue.CreateModelInstance(model);
}

Application::Application()
    : m_window(CreateWindow(), &OsWindow::Destroy)
    , m_gpuDevice(CreateGpuDevice(*m_window), &GpuDevice::Destroy)
    , m_renderPass(0)

#ifdef ASSET_REFRESH
    , m_gpuDeferredDeletionQueue()
#endif

    , m_shaderAssetFactory(m_gpuDevice.get()
#ifdef ASSET_REFRESH
                           , m_gpuDeferredDeletionQueue
#endif
                           )
    , m_shaderCache(m_shaderAssetFactory)

    , m_textureAssetFactory(m_gpuDevice.get()
#ifdef ASSET_REFRESH
                            , m_gpuDeferredDeletionQueue
#endif
                            )
    , m_textureCache(m_textureAssetFactory)

    , m_modelAssetFactory(m_gpuDevice.get(), m_textureCache)
    , m_modelCache(m_modelAssetFactory)

    , m_modelRenderQueue(m_gpuDevice.get(), m_shaderCache)
    , m_teapot(CreateModelInstance(m_modelRenderQueue, m_modelCache, "Assets/Models/Teapot.mdl"),
               &ModelInstance::Destroy)
    , m_floor(CreateModelInstance(m_modelRenderQueue, m_modelCache, "Assets/Models/Floor.mdl"),
              &ModelInstance::Destroy)
    , m_angle(0.0f)
{
    GpuRenderPassDesc renderPass;
    renderPass.flags |= GpuRenderPassDesc::FLAG_PERFORM_CLEAR;
    renderPass.clearR = 0.0f;
    renderPass.clearB = 0.0f;
    renderPass.clearG = 0.0f;
    renderPass.clearA = 0.0f;
    renderPass.clearDepth = 1.0f;
    m_renderPass = m_gpuDevice->RenderPassCreate(renderPass);

    m_modelRenderQueue.SetMaxAnisotropy(16);

    Matrix44 floorTransform(1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, -1.5f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f);
    m_floor->Update(
        floorTransform,
        Vector3(1.0f, 1.0f, 1.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        1.0f
    );

    m_window->RegisterEvent(OSEVENT_PAINT, OnPaint, (void*)this);
    m_window->RegisterEvent(OSEVENT_WINDOW_RESIZE, OnWindowResize, (void*)m_gpuDevice.get());
    m_window->RegisterEvent(OSEVENT_KEY_DOWN, OnKeyDown, (void*)this);
}

Application::~Application()
{
    m_gpuDevice->RenderPassDestroy(m_renderPass);
}

static Matrix44 CreatePerspectiveMatrix(float aspect, float fovY, float zNear, float zFar)
{
    float tanHalfFovY = tanf(fovY * (3.141592654f / 360.0f));
    float top = tanHalfFovY * zNear;
    float bot = -top;
    float right = top * aspect;
    float left = -right;
    return GpuDevice::TransformCreatePerspective(left, right, bot, top, zNear, zFar);
}

const float VIEW_ANGLE = 30.0f * (3.141592654f / 180.0f);

void Application::Frame()
{
    m_angle += 0.01f;
    float sinAngle = sinf(m_angle);
    float cosAngle = cosf(m_angle);
    Matrix44 matrix(cosAngle,  0.0f, sinAngle, 0.0f,
                    0.0f,      1.0f, 0.0f, 0.0f,
                    -sinAngle, 0.0f, cosAngle, 0.0f,
                    0.0f,      0.0f, 0.0f, 1.0f);

    const Vector3 diffuseColor(0.0f, 0.5f, 0.5f);
    const Vector3 specularColor(0.3f, 0.3f, 0.3f);
    const float glossiness = 50.0f;
    m_teapot->Update(matrix, diffuseColor, specularColor, glossiness);

    GpuViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_gpuDevice->GetFormat().resolutionX;
    viewport.height = m_gpuDevice->GetFormat().resolutionY;
    viewport.zNear = 0.0f;
    viewport.zFar = 1.0f;

    Matrix44 proj = CreatePerspectiveMatrix(4.0f/3.0f, 75.0f, 0.1f, 20.0f);
    Matrix44 view(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, cosf(VIEW_ANGLE), -sinf(VIEW_ANGLE), 0.0f,
                  0.0f, sinf(VIEW_ANGLE), cosf(VIEW_ANGLE), -3.7f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    Matrix44 viewProj = proj * view;
    Vector3 cameraPos(0.0f, 0.0f, 3.0f);

    m_gpuDevice->SceneBegin();

    ModelRenderQueue::SceneInfo info;
    info.viewProjTransform = viewProj;
    info.cameraPos = cameraPos;
    info.dirToLight = Vector3(0.0f, 1.0f, 0.0f);
    info.irradiance = Vector3(1.0f, 1.0f, 1.0f);
    info.ambientRadiance = Vector3(0.3f, 0.3f, 0.3f);

    m_modelRenderQueue.Clear();
    m_modelRenderQueue.Add(m_teapot.get());
    m_modelRenderQueue.Add(m_floor.get());
    m_modelRenderQueue.Draw(info, viewport, m_renderPass);

#ifdef ASSET_REFRESH
    m_gpuDeferredDeletionQueue.Update(m_gpuDevice.get());
    m_shaderCache.UpdateRefreshSystem();
#endif
    m_gpuDevice->ScenePresent();
}

void Application::RefreshModelShader()
{
#ifdef ASSET_REFRESH
    m_shaderCache.Refresh("Assets/Shaders/Model_MTL.shd");
#else
    printf("Asset refresh system is disabled\n");
#endif
}
