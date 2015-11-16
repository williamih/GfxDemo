#include <metal_stdlib>

using namespace metal;

struct FragmentInput {
    float3 color [[user(color)]];
};

fragment float4 PixelMain(FragmentInput input [[stage_in]])
{
    return float4(input.color, 1);
}
