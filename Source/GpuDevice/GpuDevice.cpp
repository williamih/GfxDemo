#include "GpuDevice/GpuDevice.h"

GpuSamplerDesc::GpuSamplerDesc()
    : uAddressMode(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE)
    , vAddressMode(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE)
    , wAddressMode(GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE)
    , minFilter(GPU_SAMPLER_FILTER_NEAREST)
    , magFilter(GPU_SAMPLER_FILTER_NEAREST)
    , mipFilter(GPU_SAMPLER_MIPFILTER_NOT_MIPMAPPED)
    , maxAnisotropy(1)
{}

GpuPipelineStateDesc::GpuPipelineStateDesc()
    : shaderProgram(0)
    , shaderStateBitfield(0)
    , inputLayout(0)
    , depthCompare(GPU_COMPARE_ALWAYS)
    , depthWritesEnabled(false)
    , fillMode(GPU_FILL_MODE_SOLID)
    , cullMode(GPU_CULL_NONE)
    , frontFaceWinding(GPU_WINDING_CLOCKWISE)
    , blendingEnabled(false)
    , blendSrcFactor(GPU_BLEND_ONE)
    , blendDstFactor(GPU_BLEND_ZERO)
{}

GpuRenderPassDesc::GpuRenderPassDesc()
    : numRenderTargets(0)

    , renderTargets(NULL)

    , clearColors(NULL)
    , colorLoadActions(NULL)
    , colorStoreActions(NULL)

    , depthStencilTarget(0)
    , clearDepth(0.0f)
    , depthStencilLoadAction(DEFAULT_LOAD_ACTION)
    , depthStencilStoreAction(DEFAULT_STORE_ACTION)
{}
