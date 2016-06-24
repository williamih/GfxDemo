#ifndef SCENE_RENDERTARGETDISPLAY_H
#define SCENE_RENDERTARGETDISPLAY_H

#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemWriter.h"

class GpuSamplerCache;

class ShaderCache;
class ShaderAsset;

class RenderTargetDisplay {
public:
    RenderTargetDisplay(
        GpuDevice& device,
        GpuSamplerCache& samplerCache,
        ShaderCache& shaderCache
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

    static const int DRAW_ITEM_SIZE =
        GpuDrawItemSize::Base
          + 2 * GpuDrawItemSize::Texture
          + 1 * GpuDrawItemSize::Sampler
          + 1 * GpuDrawItemSize::VertexBuf;

    GpuDevice& m_device;
    GpuSamplerCache& m_samplerCache;
    ShaderAsset* m_shader;
    GpuRenderPassID m_renderPass;
    GpuBufferID m_vertexBuf;
    GpuInputLayoutID m_inputLayout;
    GpuSamplerID m_sampler;
    GpuPipelineStateID m_pipelineStateObj;
    char m_drawItem[DRAW_ITEM_SIZE];
};

#endif // SCENE_RENDERTARGETDISPLAY_H
