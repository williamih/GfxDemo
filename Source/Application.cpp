#include "Application.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "OsWindow.h"
#include "File.h"
#include "Math/Vector3.h"
#include "GpuDevice/GpuDrawItemWriter.h"

static void OnWindowResize(const OsEvent& event, void* userdata)
{
    ((GpuDevice*)userdata)->OnWindowResized();
}

static void OnPaint(const OsEvent& event, void* userdata)
{
    ((Application*)userdata)->Frame();
}

Application::Application()
    : m_window(NULL)
    , m_gpuDevice(NULL)
    , m_renderPass(0)
    , m_modelCache(NULL)
    , m_modelRenderQueue(NULL)
    , m_modelInstance(NULL)
    , m_angle(0.0f)
{
    OsWindowPixelFormat pf;
    pf.colorBits = 32;
    pf.depthBits = 32;

    m_window = OsWindow::Create(800.0f, 600.0f, pf);

    GpuDeviceFormat deviceFormat;
    deviceFormat.pixelColorFormat = GPUPIXELCOLORFORMAT_RGBA8888;
    deviceFormat.pixelDepthFormat = GPUPIXELDEPTHFORMAT_FLOAT32;
    deviceFormat.resolutionX = 1600;
    deviceFormat.resolutionY = 1200;
    deviceFormat.flags = GpuDeviceFormat::FLAG_SCALE_RES_WITH_WINDOW_SIZE;
    m_gpuDevice = GpuDevice::Create(deviceFormat, m_window->GetNSView());

    GpuRenderPassDesc renderPass;
    renderPass.flags |= GpuRenderPassDesc::FLAG_PERFORM_CLEAR;
    renderPass.clearR = 0.0f;
    renderPass.clearB = 0.0f;
    renderPass.clearG = 0.0f;
    renderPass.clearA = 0.0f;
    renderPass.clearDepth = 1.0f;
    m_renderPass = m_gpuDevice->CreateRenderPassObject(renderPass);

    m_modelCache = new ModelCache(m_gpuDevice);
    m_modelRenderQueue = new ModelRenderQueue(m_gpuDevice);
    std::shared_ptr<ModelAsset> model(m_modelCache->FindOrLoad("Assets/Models/Torus.mdl"));
    m_modelInstance = m_modelRenderQueue->CreateModelInstance(model);

    m_window->RegisterEvent(OSEVENT_PAINT, OnPaint, (void*)this);
    m_window->RegisterEvent(OSEVENT_WINDOW_RESIZE, OnWindowResize, (void*)m_gpuDevice);
}

Application::~Application()
{
    ModelInstance::Destroy(m_modelInstance);
    delete m_modelRenderQueue;
    delete m_modelCache;
    m_gpuDevice->DestroyRenderPassObject(m_renderPass);
    GpuDevice::Destroy(m_gpuDevice);
    OsWindow::Destroy(m_window);
}

static Matrix44 CreatePerspectiveMatrix(float aspect, float fovY, float zNear, float zFar)
{
    float tanHalfFovY = tanf(fovY * (3.141592654f / 360.0f));
    float top = tanHalfFovY * zNear;
    float bot = -top;
    float right = top * aspect;
    float left = -right;
    return GpuDevice::MakePerspectiveMatrix(left, right, bot, top, zNear, zFar);
}

void Application::Frame()
{
    m_angle += 0.01f;
    float sinAngle = sinf(m_angle);
    float cosAngle = cosf(m_angle);
    Matrix44 matrix(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cosAngle, -sinAngle, 0.0f,
                    0.0f, sinAngle, cosAngle, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);

    const Vector3 diffuseColor(0.0f, 0.5f, 0.5f);
    const Vector3 specularColor(0.3f, 0.3f, 0.3f);
    const float glossiness = 50.0f;
    m_modelInstance->Update(matrix, diffuseColor, specularColor, glossiness);

    GpuViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_gpuDevice->GetFormat().resolutionX;
    viewport.height = m_gpuDevice->GetFormat().resolutionY;
    viewport.zNear = 0.0f;
    viewport.zFar = 1.0f;

    Matrix44 proj = CreatePerspectiveMatrix(4.0f/3.0f, 75.0f, 0.1f, 10.0f);
    Matrix44 view(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, -3.0f,
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

    m_modelRenderQueue->Clear();
    m_modelRenderQueue->Add(m_modelInstance);
    m_modelRenderQueue->Draw(info, viewport, m_renderPass);

    m_gpuDevice->ScenePresent();
}
