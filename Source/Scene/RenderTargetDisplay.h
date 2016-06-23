#ifndef SCENE_RENDERTARGETDISPLAY_H
#define SCENE_RENDERTARGETDISPLAY_H

#include "GpuDevice/GpuDevice.h"

class GpuSamplerCache;

template<class T> class AssetCache;
class ShaderAsset;

class RenderTargetDisplay {
public:
    RenderTargetDisplay(
        GpuDevice& device,
        GpuSamplerCache& samplerCache,
        AssetCache<ShaderAsset>& shaderCache
    );
    ~RenderTargetDisplay();

    void CopyToBackbuffer(
        const GpuViewport& viewport,
        GpuTextureID colorBuf,
        GpuTextureID depthBuf
    );

private:
    RenderTargetDisplay(const RenderTargetDisplay&);
    RenderTargetDisplay& operator=(const RenderTargetDisplay&);

    static void SamplerCacheCallback(GpuSamplerCache& cache, void* userdata);
    void CreatePSO();

    GpuDevice& m_device;
    GpuSamplerCache& m_samplerCache;
    ShaderAsset* m_shader;
    GpuRenderPassID m_renderPass;
    GpuBufferID m_vertexBuf;
    GpuInputLayoutID m_inputLayout;
    GpuSamplerID m_sampler;
    GpuPipelineStateID m_pipelineStateObj;
    void* m_drawItem;
};

#endif // SCENE_RENDERTARGETDISPLAY_H
