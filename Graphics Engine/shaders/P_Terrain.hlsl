#include "P_Common.hlsli"

/* Vertex Shader Output (Pixel Shader Input) */
struct PS_IN
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    
    nointerpolation uint4 materialID : TEXCOORD0;
    float4 materialWeights : TEXCOORD1;
};

DefineTex2DArray(TerrainColormap, 4);
DefineTex2DArray(TerrainDisplacement, 5);

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
    // Weights of 0 are null materials (contribute nothing)
    uint4 materialIDs = input.materialID;
    float4 materialWeights = input.materialWeights;

    // UV is taken from by projecting the XZ plane and scaling it
    float2 uv = position.xz * CB(CB4.uvScaling);
    
    float3 color = float3(0, 0, 0);
    

    float3 colorA = SampleTex2DArray(TerrainColormap, float3(uv, materialIDs.x)).rgb;
    float3 colorB = SampleTex2DArray(TerrainColormap, float3(uv, materialIDs.y)).rgb;
    float heightA = SampleTex2DArray(TerrainDisplacement, float3(uv, materialIDs.x)).r;
    float heightB = SampleTex2DArray(TerrainDisplacement, float3(uv, materialIDs.y)).r;
    
    heightA = heightA * materialWeights.x;
    heightB = heightB * materialWeights.y;
    
    if (heightA > heightB)
    {
        color = colorA;
    }
    else
    {
        color = colorB;
    }
        
    /*
    float3 colorA = SampleTex2DArray(TerrainColormap, float3(uv, materialIDs.x)).rgb;
    float3 colorB = SampleTex2DArray(TerrainColormap, float3(uv, materialIDs.y)).rgb;
    color = colorA * materialWeights.x + colorB * materialWeights.y;
    */

    // color += sampleMaterial(uv, materialIDs.z) * materialWeights.z;
    // color += sampleMaterial(uv, materialIDs.w) * materialWeights.w;
    
    return float4(color, 1.f);
}
