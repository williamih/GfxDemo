#include "Application.h"
#include <stddef.h>
#include <math.h>
#include "OsWindow.h"
#include "File.h"

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

Application::Application()
    : m_window(NULL)
    , m_gpuDevice(NULL)
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

    GpuPipelineState pipelineState;
    pipelineState.vertexShader = m_vertexShader;
    pipelineState.pixelShader = m_pixelShader;
    pipelineState.inputLayout = m_inputLayout;
    m_pipelineStateObj = m_gpuDevice->CreatePipelineStateObject(pipelineState);

    GpuRenderPass renderPass;
    renderPass.flags |= GpuRenderPass::FLAG_PERFORM_CLEAR;
    renderPass.clearR = 0.0f;
    renderPass.clearB = 0.0f;
    renderPass.clearG = 0.0f;
    renderPass.clearA = 0.0f;
    renderPass.clearDepth = 0.0f;
    m_renderPass = m_gpuDevice->CreateRenderPassObject(renderPass);

    m_window->RegisterEvent(OSEVENT_PAINT, OnPaint, (void*)this);
    m_window->RegisterEvent(OSEVENT_WINDOW_RESIZE, OnWindowResize, (void*)m_gpuDevice);
}

Application::~Application()
{
    GpuDevice::Destroy(m_gpuDevice);
    OsWindow::Destroy(m_window);
}

void Application::Frame()
{
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

    m_gpuDevice->BeginRenderPass(m_renderPass);

    GpuViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_gpuDevice->GetFormat().resolutionX;
    viewport.height = m_gpuDevice->GetFormat().resolutionY;

    GpuDrawItem drawItem;
    memset(&drawItem, 0, sizeof drawItem);
    drawItem.pipelineStateID = m_pipelineStateObj;
    drawItem.inputAssembler.primType = GPUPRIMITIVE_TRIANGLES;
    drawItem.inputAssembler.nVertexBuffers = 1;
    drawItem.inputAssembler.vertexBuffers[0].bufferID = m_vertexBuffer;
    drawItem.inputAssembler.vertexBuffers[0].offset = 0;
    drawItem.resources.cbuffers[0] = m_cbuffer;
    drawItem.resources.nCBuffers = 1;
    drawItem.viewport = viewport;
    drawItem.first = 0;
    drawItem.count = 3;
    drawItem.indexBufferOffset = 0;
    drawItem.indexType = GPUINDEXTYPE_NONE;
    m_gpuDevice->Draw(drawItem);

    m_gpuDevice->EndRenderPass();

    m_gpuDevice->ScenePresent();
}
