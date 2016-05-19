#include <metal_stdlib>

using namespace metal;

struct InputVertex {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
};

struct ProjectedVertex {
    float4 position [[position]];
    float2 uv;
};

struct PixelOutput {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

// -----------------------------------------------------------------------------
// Vertex shader
// -----------------------------------------------------------------------------
vertex ProjectedVertex VertexMain(
    InputVertex vert [[stage_in]]
)
{
    ProjectedVertex outVert;

    outVert.position = float4(vert.position, 1);
    outVert.uv = vert.uv;

    return outVert;
}

// -----------------------------------------------------------------------------
// Pixel shader
// -----------------------------------------------------------------------------
fragment PixelOutput PixelMain(
    ProjectedVertex  input      [[stage_in]],
    sampler          theSampler [[sampler(0)]],
    texture2d<float> colorBuf   [[texture(0)]],
    texture2d<float> depthBuf   [[texture(1)]]
)
{
    PixelOutput out;
    out.color = colorBuf.sample(theSampler, input.uv);
    out.depth = depthBuf.sample(theSampler, input.uv).r;
    return out;
}
