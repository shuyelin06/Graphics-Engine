#ifndef __P_COMMON_HEADER__
#define __P_COMMON_HEADER__
// P_Common.hlsli:
// Contains common bindings which are to be used
// across all pixel shaders.
// If a binding is not given here, it is assumed to be a slot
// that can be used by any shader for any other purpose.
SamplerState s_point : register(s0);
SamplerState s_shadow : register(s1);

Texture2D color_atlas : register(t0);
Texture2D shadow_atlas : register(t1);

cbuffer CB0_GLOBAL_DATA : register(b0)
{
    // View Information
    float3 view_pos;
    float view_znear;
    
    float3 view_direction;
    float view_zfar;
    
    float4x4 m_screen_to_world;
    float4x4 m_world_to_screen;
    
    // Render Target Information
    float resolution_x;
    float resolution_y;
    float2 cb0_p0; // Padding
    
    // Rendering Config
    
    
}

#endif