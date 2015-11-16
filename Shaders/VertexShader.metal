#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct ProjectedVertex {
    float4 position [[position]];
    float3 color [[user(color)]];
};

struct CBufferData {
    float4x4 mvpTransform;
};

vertex ProjectedVertex VertexMain(Vertex vert [[stage_in]],
                                  constant CBufferData& cbuf [[buffer(0)]])
{
    ProjectedVertex outVert;
    outVert.position = cbuf.mvpTransform * float4(vert.position, 1);
    outVert.color = vert.color.rgb;
    return outVert;
}
