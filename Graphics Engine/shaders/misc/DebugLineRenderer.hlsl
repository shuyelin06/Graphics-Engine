#include "V_Common.hlsli"

struct VS_INPUT
{
    float4 PosXYZ_TexU : PosXYZ_TexU;
    float4 ColorRGBA : ColorRGBA;
};

struct PS_INPUT
{
    float4 position_clip : SV_POSITION;
    float3 color : COLOR;
};

// Vertex Shader:
// Based on the instance ID, modifies the cube mesh
PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    float3 position = input.PosXYZ_TexU.xyz;
    float3 color = input.ColorRGBA.rgb;

    float4 pos = float4(position, 1.0f);
    pos = mul(m_world_to_screen, pos);
    output.position_clip = pos;
    
    output.color = color;
    
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color = float4(input.color, 1.0f);
    return color;
}