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

DefineTex2D(heightmap, 0);

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
    float height = SampleTex2DLevel(heightmap, float2(u, v), 0, float2(0,0)).r;
    
    // Generate my (x,y,z) world position
    float3 position = float3(x, height, z);
    
    float4 pos = float4(position, 1);
    pos = mul(m_world_to_screen, pos);
    
    // Generate my normal
    float normX1 = SampleTex2DLevel(heightmap, float2(u, v), 0, float2(-1, 0)).r;
    float normX2 = SampleTex2DLevel(heightmap, float2(u, v), 0, float2(1, 0)).r;
    float normZ1 = SampleTex2DLevel(heightmap, float2(u, v), 0, float2(0, -1)).r;
    float normZ2 = SampleTex2DLevel(heightmap, float2(u, v), 0, float2(0, 1)).r;
    float3 tangentX = float3(0, (normX2 - normX1) * 0.5f, 1);
    float3 tangentZ = float3(1, (normZ2 - normZ1) * 0.5f, 1);
    
    float3 normal = normalize(cross(tangentX, tangentZ));
    
    output.world_position = position;
    output.normal = normal;
    output.position_clip = pos;
    
    return output;
}