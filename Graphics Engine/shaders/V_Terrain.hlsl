#include "V_Common.hlsli"

struct ChunkData
{
    float2 positionXZ;
    float2 extentsXZ;
};
cbuffer CB5_TERRAIN_DATA : register(b5)
{
    float2 heightMapWorldPosition;
    float2 heightMapWorldExtents;

    ChunkData chunkData[2000];
}

Texture2D g_heightmap : register(t0);
SamplerState g_heightmapSampler : register(s0);

struct VS_IN
{
    float3 position_local : POSITION;
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

VS_OUT vsterrain_main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    // Based on my ChunkData, determine my (x,z) world position
    ChunkData data = chunkData[input.instanceID];
    float x = data.positionXZ.x + input.position_local.x * data.extentsXZ.x;
    float z = data.positionXZ.y + input.position_local.z * data.extentsXZ.y;
    
    // Convert world position to UV coordinates in the heightmap
    float u = (x - heightMapWorldPosition.x) / heightMapWorldExtents.x;
    float v = (z - heightMapWorldPosition.y) / heightMapWorldExtents.y;
    float height = g_heightmap.SampleLevel(g_heightmapSampler, float2(u, v), 0).r;
    
    // Generate my (x,y,z) world position
    float3 position = float3(x, height, z);
    
    float4 pos = float4(position, 1);
    pos = mul(m_world_to_screen, pos);
    
    output.world_position = position;
    output.normal = float3(0, 1, 0);
    output.position_clip = pos;
    
    return output;
}