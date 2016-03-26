#include "Application.h"

#include <stddef.h>
#include <stdlib.h>
#include <math.h>

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

OsWindow* Application::CreateWindow()
{
    OsWindowPixelFormat pf;
    pf.colorBits = 32;
    pf.depthBits = 32;

    return OsWindow::Create(1280.0f, 720.0f, pf);
}

GpuDevice* Application::CreateGpuDevice(OsWindow& window)
{
    GpuDeviceFormat deviceFormat;
    deviceFormat.pixelColorFormat = GPU_PIXEL_COLOR_FORMAT_RGBA8888;
    deviceFormat.pixelDepthFormat = GPU_PIXEL_DEPTH_FORMAT_FLOAT32;
    deviceFormat.resolutionX = 2560;
    deviceFormat.resolutionY = 1440;
    deviceFormat.flags = GpuDeviceFormat::FLAG_SCALE_RES_WITH_WINDOW_SIZE;
    return GpuDevice::Create(deviceFormat, window.GetNSView());
}

Application::Application()
    : m_window(CreateWindow(), &OsWindow::Destroy)
    , m_gpuDevice(CreateGpuDevice(*m_window), &GpuDevice::Destroy)

#ifdef ASSET_REFRESH
    , m_gpuDeferredDeletionQueue()
#endif

    , m_fileLoader()

    , m_shaderAssetFactory(m_gpuDevice.get()
#ifdef ASSET_REFRESH
                           , m_gpuDeferredDeletionQueue
#endif
                           )
    , m_shaderCache(m_fileLoader, m_shaderAssetFactory)

    , m_textureAssetFactory(m_gpuDevice.get()
#ifdef ASSET_REFRESH
                            , m_gpuDeferredDeletionQueue
#endif
                            )
    , m_textureCache(m_fileLoader, m_textureAssetFactory)

    , m_modelAssetFactory(m_gpuDevice.get(), m_textureCache)
    , m_modelCache(m_fileLoader, m_modelAssetFactory)

    , m_scene(m_gpuDevice.get(), m_shaderCache, m_modelCache)
    , m_camera()
    , m_teapot(NULL)
    , m_floor(NULL)
    , m_angle(0.0f)
{
    m_teapot = m_scene.AddModelInstance("Assets/Models/Teapot.mdl");
    m_floor = m_scene.AddModelInstance("Assets/Models/Floor.mdl");
    m_scene.SetSkybox("Assets/Models/Skybox.mdl");

    Matrix44 floorTransform(1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, -1.5f,
                            0.0f, 0.0f, 0.0f, 1.0f);
    m_floor->Update(
        floorTransform,
        Vector3(1.0f, 1.0f, 1.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        1.0f
    );

    m_camera.SetPositionAndTarget(Vector3(0.0f, -3.87f, 2.5f), Vector3(0.0f, -3.0f, 2.0f));

    m_window->RegisterEvent(OSEVENT_PAINT, OnPaint, (void*)this);
    m_window->RegisterEvent(OSEVENT_WINDOW_RESIZE, OnWindowResize, (void*)m_gpuDevice.get());
    m_window->RegisterEvent(OSEVENT_KEY_DOWN, &Application::OnKeyDown, (void*)this);
    m_window->RegisterEvent(OSEVENT_KEY_UP, &Application::OnKeyUp, (void*)this);
    m_window->RegisterEvent(OSEVENT_MOUSE_DRAG, &Application::OnMouseDragged, (void*)this);
}

Application::~Application()
{}

void Application::Frame()
{
    m_camera.Update(1.0f / 60.0f);

    m_angle += 0.01f;
    float sinAngle = sinf(m_angle);
    float cosAngle = cosf(m_angle);
    Matrix44 matrix(cosAngle, -sinAngle, 0.0f, 0.0f,
                    sinAngle,  cosAngle, 0.0f, 0.0f,
                    0.0f,      0.0f,     1.0f, 0.0f,
                    0.0f,      0.0f,     0.0f, 1.0f);

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

    m_gpuDevice->SceneBegin();

    SceneUpdateInfo updateInfo;
    updateInfo.cameraPos = m_camera.Position();
    updateInfo.forward = m_camera.Forward();
    updateInfo.right = m_camera.Right();
    updateInfo.up = m_camera.Up();
    updateInfo.zNear = m_camera.ZNear();
    updateInfo.zFar = m_camera.ZFar();
    updateInfo.fovY = m_camera.FOV();
    m_scene.Update(updateInfo);
    m_scene.Render(viewport);

#ifdef ASSET_REFRESH
    m_gpuDeferredDeletionQueue.Update(m_gpuDevice.get());
    m_shaderCache.UpdateRefreshSystem();
#endif
    m_gpuDevice->ScenePresent();
}

void Application::OnKeyDown(const OsEvent& event, void* userdata)
{
    if (event.key.code == OSKEY_R)
        ((Application*)userdata)->RefreshModelShader();
    else if (event.key.code == OSKEY_W || event.key.code == OSKEY_UP_ARROW)
        ((Application*)userdata)->m_camera.MoveForwardStart();
    else if (event.key.code == OSKEY_S || event.key.code == OSKEY_DOWN_ARROW)
        ((Application*)userdata)->m_camera.MoveBackwardStart();
    else if (event.key.code == OSKEY_SPACE)
        ((Application*)userdata)->m_camera.AscendStart();
    else if (event.key.code == OSKEY_X)
        ((Application*)userdata)->m_camera.DescendStart();
    else if (event.key.code == OSKEY_A || event.key.code == OSKEY_LEFT_ARROW)
        ((Application*)userdata)->m_camera.TurnLeftStart();
    else if (event.key.code == OSKEY_D || event.key.code == OSKEY_RIGHT_ARROW)
        ((Application*)userdata)->m_camera.TurnRightStart();
}

void Application::OnKeyUp(const OsEvent& event, void* userdata)
{
    if (event.key.code == OSKEY_R)
        ((Application*)userdata)->RefreshModelShader();
    else if (event.key.code == OSKEY_W || event.key.code == OSKEY_UP_ARROW)
        ((Application*)userdata)->m_camera.MoveForwardStop();
    else if (event.key.code == OSKEY_S || event.key.code == OSKEY_DOWN_ARROW)
        ((Application*)userdata)->m_camera.MoveBackwardStop();
    else if (event.key.code == OSKEY_SPACE)
        ((Application*)userdata)->m_camera.AscendStop();
    else if (event.key.code == OSKEY_X)
        ((Application*)userdata)->m_camera.DescendStop();
    else if (event.key.code == OSKEY_A || event.key.code == OSKEY_LEFT_ARROW)
        ((Application*)userdata)->m_camera.TurnLeftStop();
    else if (event.key.code == OSKEY_D || event.key.code == OSKEY_RIGHT_ARROW)
        ((Application*)userdata)->m_camera.TurnRightStop();
}

void Application::OnMouseDragged(const OsEvent& event, void* userdata)
{
    ((Application*)userdata)->m_camera.HandleMouseDrag(
        event.mouseDrag.normalizedDeltaX,
        event.mouseDrag.normalizedDeltaY
    );
}

void Application::RefreshModelShader()
{
#ifdef ASSET_REFRESH
    m_shaderCache.Refresh("Assets/Shaders/Model_MTL.shd");
#else
    printf("Asset refresh system is disabled\n");
#endif
}
