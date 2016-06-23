#ifndef SCENE_RENDERTARGETDISPLAY_H
#define SCENE_RENDERTARGETDISPLAY_H

#include "GpuDevice/GpuDevice.h"

template<class T> class AssetCache;
class ShaderAsset;

class RenderTargetDisplay {
public:
    RenderTargetDisplay(GpuDevice& device, AssetCache<ShaderAsset>& shaderCache);
    ~RenderTargetDisplay();

    void CopyToBackbuffer(
        const GpuViewport& viewport,
        GpuTextureID colorBuf,
        GpuTextureID depthBuf
    );

private:
    RenderTargetDisplay(const RenderTargetDisplay&);
    RenderTargetDisplay& operator=(const RenderTargetDisplay&);

    void CreatePSO();

    GpuDevice& m_device;
    ShaderAsset* m_shader;
    GpuRenderPassID m_renderPass;
    GpuBufferID m_vertexBuf;
    GpuInputLayoutID m_inputLayout;
    GpuSamplerID m_sampler;
    GpuPipelineStateID m_pipelineStateObj;
    void* m_drawItem;
};

#endif // SCENE_RENDERTARGETDISPLAY_H
