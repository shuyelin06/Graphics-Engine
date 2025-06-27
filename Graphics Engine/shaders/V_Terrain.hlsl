cbuffer PER_VIEW_DATA : register(b0)
{
    float4x4 m_world_to_screen;
}

struct TB_Descriptor
{
    uint i_start;
    uint i_count;
    uint v_start;
    uint v_count;
};

StructuredBuffer<TB_Descriptor> sb_descriptors : register(t0);
StructuredBuffer<uint> sb_indices : register(t1);
StructuredBuffer<float3> sb_positions : register(t2);
StructuredBuffer<float3> sb_normals : register(t3);

struct VS_IN
{
    uint vertex_id : SV_VertexID;
    uint instance_id : SV_InstanceID;
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

    // We perform vertex pulling. 
    // 1) Different instance ids represent different chunks.
    // First, we pull the chunk data we need using the instance ID
    TB_Descriptor chunk_desc = sb_descriptors[input.instance_id];
    
    // 2) Now, we use the vertex id to reference the index / vertex buffers, and
    // find our corresponding vertex index.
    // If the chunk does not have enough indices, just return NaN
    if (input.vertex_id >= chunk_desc.i_count)
    {
        float nan = 0.0 / 0.0;
        output.world_position = float3(nan, nan, nan);
        output.normal = float3(nan, nan, nan);
        output.position_clip = float4(nan, nan, nan, nan);
    }
    else
    {
        uint vertex_index = sb_indices[chunk_desc.i_start + input.vertex_id] + chunk_desc.v_start;
        
        float3 position = sb_positions[vertex_index];
        float3 norm = sb_normals[vertex_index];
        
        output.world_position = position;
        output.normal = norm;
    
        float4 pos = float4(position, 1);
        pos = mul(m_world_to_screen, pos);
        output.position_clip = pos;
    }
    
    return output;

}