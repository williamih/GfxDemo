using namespace metal;

struct MDLVertex {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct MDLSceneData {
    float4x4 viewProjTransform;
    float4   cameraPos;
    float4   dirToLight;
    float4   irradiance_over_pi;
    float4   ambientRadiance;
};

struct MDLInstanceData {
    float4x4 worldTransform;
    float3x3 normalTransform;
    float4   diffuseColor;
    float4   specularColorAndGlossiness;
};
