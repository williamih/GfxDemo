#include "GpuDevice/GpuDevice.h"

GpuPipelineStateDesc::GpuPipelineStateDesc()
    : vertexShader(0)
    , pixelShader(0)
    , inputLayout(0)
{}

GpuRenderPassDesc::GpuRenderPassDesc()
    : flags(0)
    , clearR(0.0f)
    , clearG(0.0f)
    , clearB(0.0f)
    , clearA(0.0f)
    , clearDepth(0.0f)
{}
