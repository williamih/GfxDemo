#include <metal_stdlib>
#include "ModelTypes.h"

using namespace metal;

struct ProjectedVertex {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float3 dirToViewer;
};

float3 BRDF(
    float3 cDiff,
    float3 cSpec,
    float glossiness,
    float3 n,
    float3 l,
    float3 v
);

// -----------------------------------------------------------------------------
// Vertex shader
// -----------------------------------------------------------------------------
vertex ProjectedVertex VertexMain(
    MDLVertex                 vert         [[stage_in]],
    constant MDLSceneData&    sceneData    [[buffer(0)]],
    constant MDLInstanceData& instanceData [[buffer(1)]]
)
{
    float4 worldPos = instanceData.worldTransform * float4(vert.position, 1);
    ProjectedVertex outVert;
    outVert.position = sceneData.viewProjTransform * worldPos;
    outVert.normal = instanceData.normalTransform * vert.normal;
    outVert.uv = vert.uv;
    outVert.dirToViewer = sceneData.cameraPos.xyz - worldPos.xyz;
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
    float3 n = normalize(input.normal);
    float3 l = sceneData.dirToLight.xyz;
    float3 v = normalize(input.dirToViewer);

    float3 cDiff = instanceData.diffuseColor.rgb;
    cDiff *= diffuseTex.sample(theSampler, input.uv).rgb;

    float3 cSpec = instanceData.specularColorAndGlossiness.rgb;
    float glossiness = instanceData.specularColorAndGlossiness.a;
    float3 EL_over_pi = sceneData.irradiance_over_pi.xyz;

    float cosTheta = saturate(dot(n, l));
    float3 radiance = BRDF(cDiff, cSpec, glossiness, n, l, v) * EL_over_pi * cosTheta;
    radiance += cDiff * sceneData.ambientRadiance.xyz;

    return float4(radiance, 1);
}

float3 BRDF(
    float3 cDiff,
    float3 cSpec,
    float glossiness,
    float3 n,
    float3 l,
    float3 v
)
{
    float3 h = normalize(l + v);
    float NdotH = saturate(dot(n, h));
    return cDiff + ((glossiness + 8) / 8) * cSpec * pow(NdotH, glossiness);
}
