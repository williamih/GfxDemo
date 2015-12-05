#include "Application.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "OsWindow.h"
#include "File.h"
#include "GpuDevice/GpuDrawItemWriter.h"

static GpuShaderID CreateShader(GpuDevice* device,
                                const char* path,
                                GpuShaderType type)
{
    std::vector<char> data;
    FileReadFile(path, data);
    return device->CreateShader(type, &data[0], data.size() - 1);
}

struct Vertex {
    float position[3];
    float normal[3];
    unsigned char color[4];
};

static void OnWindowResize(const OsEvent& event, void* userdata)
{
    ((GpuDevice*)userdata)->OnWindowResized();
}

static void OnPaint(const OsEvent& event, void* userdata)
{
    ((Application*)userdata)->Frame();
}

static void* Alloc(size_t size, void*) { return malloc(size); }

Application::Application()
    : m_window(NULL)
    , m_gpuDevice(NULL)
    , m_vertexShader(0)
    , m_pixelShader(0)
    , m_vertexBuffer(0)
    , m_cbuffer(0)
    , m_inputLayout(0)
    , m_pipelineStateObj(0)
    , m_renderPass(0)
    , m_drawItem(NULL)
    , m_angle(0)
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

    m_vertexShader = CreateShader(m_gpuDevice, "Assets/Shaders/VertexShader.metallib",
                                  GPUSHADERTYPE_VERTEX);
    m_pixelShader = CreateShader(m_gpuDevice, "Assets/Shaders/PixelShader.metallib",
                                 GPUSHADERTYPE_PIXEL);

    Vertex vertices[] = {
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {255, 0, 0, 255}, },
        { {0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0, 255, 0, 255}, },
        { {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0, 0, 255, 255}, },
    };
    m_vertexBuffer = m_gpuDevice->CreateBuffer(GPUBUFFERTYPE_VERTEX,
                                               GPUBUFFER_ACCESS_STATIC,
                                               vertices,
                                               sizeof vertices);

    m_cbuffer = m_gpuDevice->CreateBuffer(GPUBUFFERTYPE_CONSTANT,
                                          GPUBUFFER_ACCESS_DYNAMIC,
                                          NULL,
                                          16 * sizeof(float));

    GpuVertexAttribute attribs[] = {
        {GPUVERTEXATTRIB_FLOAT3, offsetof(Vertex, position), 0},
        {GPUVERTEXATTRIB_FLOAT3, offsetof(Vertex, normal), 0},
        {GPUVERTEXATTRIB_UBYTE4_NORMALIZED, offsetof(Vertex, color), 0},
    };
    unsigned strides[] = { sizeof(Vertex) };
    m_inputLayout = m_gpuDevice->CreateInputLayout(sizeof attribs / sizeof attribs[0],
                                                   attribs,
                                                   1,
                                                   strides);

    GpuPipelineStateDesc pipelineState;
    pipelineState.vertexShader = m_vertexShader;
    pipelineState.pixelShader = m_pixelShader;
    pipelineState.inputLayout = m_inputLayout;
    m_pipelineStateObj = m_gpuDevice->CreatePipelineStateObject(pipelineState);

    GpuRenderPassDesc renderPass;
    renderPass.flags |= GpuRenderPassDesc::FLAG_PERFORM_CLEAR;
    renderPass.clearR = 0.0f;
    renderPass.clearB = 0.0f;
    renderPass.clearG = 0.0f;
    renderPass.clearA = 0.0f;
    renderPass.clearDepth = 0.0f;
    m_renderPass = m_gpuDevice->CreateRenderPassObject(renderPass);

    m_window->RegisterEvent(OSEVENT_PAINT, OnPaint, (void*)this);
    m_window->RegisterEvent(OSEVENT_WINDOW_RESIZE, OnWindowResize, (void*)m_gpuDevice);

    GpuDrawItemWriterDesc writerDesc;
    writerDesc.SetNumCBuffers(1);
    writerDesc.SetNumVertexBuffers(1);

    GpuDrawItemWriter writer;
    writer.Begin(m_gpuDevice, writerDesc, Alloc, NULL);
    writer.SetPipelineState(m_pipelineStateObj);
    writer.SetVertexBuffer(0, m_vertexBuffer, 0);
    writer.SetCBuffer(0, m_cbuffer);
    writer.SetDrawCall(GPUPRIMITIVE_TRIANGLES, 0, 3);
    m_drawItem = writer.End();
}

Application::~Application()
{
    GPUDEVICE_UNREGISTER_DRAWITEM(m_gpuDevice, m_drawItem);
    free(m_drawItem);
    m_gpuDevice->DestroyPipelineStateObject(m_pipelineStateObj);
    m_gpuDevice->DestroyShader(m_vertexShader);
    m_gpuDevice->DestroyShader(m_pixelShader);
    m_gpuDevice->DestroyBuffer(m_vertexBuffer);
    m_gpuDevice->DestroyBuffer(m_cbuffer);
    m_gpuDevice->DestroyRenderPassObject(m_renderPass);
    m_gpuDevice->DestroyInputLayout(m_inputLayout);
    GpuDevice::Destroy(m_gpuDevice);
    OsWindow::Destroy(m_window);
}

void Application::Frame()
{
    GpuViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_gpuDevice->GetFormat().resolutionX;
    viewport.height = m_gpuDevice->GetFormat().resolutionY;
    viewport.zNear = 0.0f;
    viewport.zFar = 1.0f;

    m_gpuDevice->SceneBegin();

    m_angle += 0.02f;
    float matrix[] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f * cosf(m_angle), 0.0f, 0.0f, 1.0f,
    };
    memcpy(m_gpuDevice->GetBufferContents(m_cbuffer), matrix, sizeof matrix);
    m_gpuDevice->FlushBufferRange(m_cbuffer, 0, sizeof matrix);

    const GpuDrawItem* items[] = {m_drawItem};
    m_gpuDevice->Draw(items, sizeof items / sizeof items[0], m_renderPass, viewport);

    m_gpuDevice->ScenePresent();
}
