#ifndef __V_COMMON_HEADER__
#define __V_COMMON_HEADER__
// V_Common.hlsli
// Contains common bindings which are to be used across all
// vertex shaders. 
cbuffer CB0_TRANSFORM_INFO : register(b0)
{
    float4x4 m_world_to_screen;
    
    float3 view_position;
};

#endif