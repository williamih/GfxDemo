#include <metal_stdlib>

using namespace metal;

struct FragmentInput {
    float3 normal [[user(normal)]];
    float3 dirToViewer [[user(dirToViewer)]];
};

struct SceneData {
    float4x4 viewProjTransform;
    float4 cameraPos;
    float4 dirToLight;
    float4 irradiance_over_pi;
    float4 ambientRadiance;
};

struct InstanceData {
    float4x4 worldTransform;
    float3x3 normalTransform;
    float4 diffuseColor;
    float4 specularColorAndGlossiness;
};

float3 BRDF(float3 cDiff,
            float3 cSpec,
            float glossiness,
            float3 n,
            float3 l,
            float3 v);

fragment float4 PixelMain(FragmentInput input [[stage_in]],
                          constant SceneData& sceneData [[buffer(0)]],
                          constant InstanceData& instanceData [[buffer(1)]])
{
    float3 n = normalize(input.normal);
    float3 l = sceneData.dirToLight.xyz;
    float3 v = normalize(input.dirToViewer);

    float3 cDiff = instanceData.diffuseColor.rgb;
    float3 cSpec = instanceData.specularColorAndGlossiness.rgb;
    float glossiness = instanceData.specularColorAndGlossiness.a;
    float3 EL_over_pi = sceneData.irradiance_over_pi.xyz;

    float cosTheta = saturate(dot(n, l));
    float3 radiance = BRDF(cDiff, cSpec, glossiness, n, l, v) * EL_over_pi * cosTheta;
    radiance += cDiff * sceneData.ambientRadiance.xyz;

    return float4(radiance, 1);
}

float3 BRDF(float3 cDiff,
            float3 cSpec,
            float glossiness,
            float3 n,
            float3 l,
            float3 v)
{
    float3 h = normalize(l + v);
    float NdotH = saturate(dot(n, h));
    return cDiff + ((glossiness + 8) / 8) * cSpec * pow(NdotH, glossiness);
}
