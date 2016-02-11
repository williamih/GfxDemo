#include <metal_stdlib>
#include "ModelTypes.h"

using namespace metal;

struct ProjectedVertex {
    float4 position [[position]];
    float2 uv;
};

// -----------------------------------------------------------------------------
// Vertex shader
// -----------------------------------------------------------------------------
vertex ProjectedVertex VertexMain(
    MDLVertex                 vert         [[stage_in]],
    constant MDLSceneData&    sceneData    [[buffer(0)]],
    constant MDLInstanceData& instanceData [[buffer(1)]]
)
{
    // Ensure that the skybox is always centered at the origin.
    float4x4 vpTransform = sceneData.viewProjTransform;
    vpTransform[3] = float4(0, 0, 0, 1);

    float4 worldPos = instanceData.worldTransform * float4(vert.position, 1);

    ProjectedVertex outVert;
    outVert.position = vpTransform * worldPos;

    // Ensure that the skybox is always at the far plane.
    outVert.position.z = outVert.position.w;

    outVert.uv = vert.uv;

    return outVert;
}

// -----------------------------------------------------------------------------
// Pixel shader
// -----------------------------------------------------------------------------
fragment float4 PixelMain(
    ProjectedVertex           input        [[stage_in]],
    constant MDLSceneData&    sceneData    [[buffer(0)]],
    constant MDLInstanceData& instanceData [[buffer(1)]],
    sampler                   theSampler   [[sampler(0)]],
    texture2d<float>          diffuseTex   [[texture(0)]]
)
{
    float3 cDiff = instanceData.diffuseColor.rgb;
    cDiff *= diffuseTex.sample(theSampler, input.uv).rgb;

    return float4(cDiff, 1);
}
