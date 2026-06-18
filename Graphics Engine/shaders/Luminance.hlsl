#include "Common.hlsli"
#include "P_Common.hlsli"
#include "ToneMap.hlsli"

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

DefineTex2D(render_target, 0);

float ps_main(PS_IN input) : SV_TARGET
{
    // Read from the render target texture
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    // Sample for our color, and calculate the luminance
    float3 color = SampleTex2D(render_target, uv).rgb;
    float lumens = luminance(color);
    
    return lumens;
}