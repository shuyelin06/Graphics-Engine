#include "P_Common.hlsli"

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

Texture2D render_target : register(t0);

float4 ps_main(PS_IN input) : SV_TARGET
{
    // Read from the render target texture and write what we read.
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    float3 color = render_target.Sample(s_point, uv);
    
    return float4(color, 1.f);
}
