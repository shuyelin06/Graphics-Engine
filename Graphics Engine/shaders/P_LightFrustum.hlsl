#include "P_Common.hlsli"

struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
};

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    // Attenuate alpha blending based on distance to camera
    float distance = length(view_pos - input.world_position);
    
    // Add a small dither to prevent banding
    float dither = frac(sin(distance) * 43758.5453f) * 0.01f;
    float alpha = 0.75f / (0.5f * distance + 1.f);
    alpha = 0.3f;
    
    return float4(1.0f, 1.0f, 0.5f, alpha + dither);
}