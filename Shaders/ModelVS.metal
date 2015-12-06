#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
};

struct ProjectedVertex {
    float4 position [[position]];
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

vertex ProjectedVertex VertexMain(Vertex vert [[stage_in]],
                                  constant SceneData& sceneData [[buffer(0)]],
                                  constant InstanceData& instanceData [[buffer(1)]])
{
    float4 worldPos = instanceData.worldTransform * float4(vert.position, 1);
    ProjectedVertex outVert;
    outVert.position = sceneData.viewProjTransform * worldPos;
    outVert.normal = instanceData.normalTransform * vert.normal;
    outVert.dirToViewer = sceneData.cameraPos.xyz - worldPos.xyz;
    return outVert;
}
