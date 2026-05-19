cbuffer PER_VIEW_DATA : register(b0)
{
    float4x4 m_world_to_screen;
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

    float3 position = input.position_local;
    float3 norm = input.normal;
        
    output.world_position = position;
    output.normal = norm;
    
    float4 pos = float4(position, 1);
    pos = mul(m_world_to_screen, pos);
    output.position_clip = pos;    
    
    return output;
}