#include "P_Common.hlsli"

/* Vertex Shader Output (Pixel Shader Input) */
struct PS_IN
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    
    uint4 materialID : TEXCOORD0;
    float4 materialWeights : TEXCOORD1;
};

DefineTex2DArray(TerrainColormap, 4);

struct TerrainData
{
    float uvScaling;
    float3 pad3;
};
cbuffer CB4_TerrainData : register(b4)
{
    TerrainData CB4;
}

float4 psterrain_main(PS_IN input) : SV_TARGET
{
    float3 position = input.world_position;
    // UV is taken from by projecting the XZ plane and scaling it
    float2 uv = position.xz * CB(CB4.uvScaling);
    
    float3 color = SampleTex2DArray(TerrainColormap, float3(uv, 0));
    return float4(color, 1.f);
}
