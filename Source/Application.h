#ifndef APPLICATION_H
#define APPLICATION_H

#include "GpuDevice/GpuDevice.h"

class OsWindow;

class Application {
public:
    Application();
    ~Application();
    void Frame();
private:
    Application(const Application&);
    Application& operator=(const Application&);

    OsWindow* m_window;
    GpuDevice* m_gpuDevice;
    GpuShaderID m_vertexShader;
    GpuShaderID m_pixelShader;
    GpuBufferID m_vertexBuffer;
    GpuBufferID m_cbuffer;
    GpuInputLayoutID m_inputLayout;
    GpuPipelineStateID m_pipelineStateObj;
    GpuRenderPassID m_renderPass;
    float m_angle;
};

#endif // APPLICATION_H
