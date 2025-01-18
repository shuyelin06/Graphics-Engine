cbuffer PER_VIEW_DATA : register(b0)
{
    float4x4 m_view; 
    float4x4 m_projection;
}

cbuffer PER_MESH_DATA : register(b1)
{
    float3 terrain_offset;
    float padding;
}

struct VS_IN
{
    float3 position_local : POSITION;
    float3 normal : NORMAL;
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

    float4 pos = float4(input.position_local + terrain_offset, 1);
    output.world_position = pos.xyz;
    
    pos = mul(m_view, pos);
    pos = mul(m_projection, pos);
    output.position_clip = pos;
    
    output.normal = input.normal;
    
    return output;

}