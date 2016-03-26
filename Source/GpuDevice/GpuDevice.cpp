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
{}

GpuRenderPassDesc::GpuRenderPassDesc()
    : flags(0)
    , clearR(0.0f)
    , clearG(0.0f)
    , clearB(0.0f)
    , clearA(0.0f)
    , clearDepth(0.0f)
{}
