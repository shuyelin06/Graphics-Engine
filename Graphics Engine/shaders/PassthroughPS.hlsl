#include "Common.hlsli"

struct PostFXParams
{
    float resolutionX;
    float resolutionY;
    float2 padding;
};

cbuffer PostFXParamsCB : register(b0)
{
    PostFXParams CB0;
};

DefineTex2D(render_target, 0);

struct PS_IN
{
    float4 position_clip : SV_Position;
};

float4 ps_main(PS_IN input) : SV_TARGET
{
    // Read from the render target texture and write what we read.
    float2 uv = float2(input.position_clip.x / CB0.resolutionX, input.position_clip.y / CB0.resolutionY);
    float4 color = SampleTex2D(render_target, uv).rgba;
    return color;
}
