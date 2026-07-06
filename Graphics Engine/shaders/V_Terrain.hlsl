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

float3 calculateTerrainNormal(float2 uv)
{
    // Sample adjacent texels to my UV coordinate.
    // These generate the gradient vectors at this uv. We can cross them to find the normal.
    float heightX1 = SampleTex2DLevel(heightmap, uv, 0, float2(-1, 0)).r;
    float heightX2 = SampleTex2DLevel(heightmap, uv, 0, float2(1, 0)).r;
    float3 tangentX = float3(1, (heightX2 - heightX1) * 0.5f, 0);
    
    float heightZ1 = SampleTex2DLevel(heightmap, uv, 0, float2(0, -1)).r;
    float heightZ2 = SampleTex2DLevel(heightmap, uv, 0, float2(0, 1)).r;
    float3 tangentZ = float3(0, (heightZ2 - heightZ1) * 0.5f, 1);
    
    return normalize(cross(tangentX, tangentZ));
}

VS_OUT vsterrain_main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    // Based on my ChunkData, determine my (x,z) world position
    ChunkData data = chunkData[input.instanceID];
    float x = data.positionXZ.x + input.position_local.x * data.extentsXZ.x;
    float z = data.positionXZ.y + input.position_local.z * data.extentsXZ.y;
    
    // Convert world position to UV coordinates that we can use to sample the heightmap
    float2 uv = float2((x - heightMapWorldPosition.x) / heightMapWorldExtents.x,
                    (z - heightMapWorldPosition.y) / heightMapWorldExtents.y);
    
    // Generate my (x,y,z) world position using the heightmap
    // Add to the input position. We add because mesh has skirts that have negative y coordinates
    // which we need to factor in (to hide LOD transitions)
    float height = SampleTex2DLevel(heightmap, uv, 0, float2(0,0)).r;
    float3 worldPosition = float3(x, input.position_local.y + height, z);
    float4 clipPosition = mul(CB(m_world_to_screen), float4(worldPosition, 1));

    // Generate my normal
    float3 normal = calculateTerrainNormal(uv);
    
    output.world_position = worldPosition;
    output.normal = normal;
    output.position_clip = clipPosition;
    
    return output;
}