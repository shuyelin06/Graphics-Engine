cbuffer CB1 : register(b1)
{
    float4x4 m_view;
    float4x4 m_projection;
}

struct VS_INPUT
{
    // Represents one point of a line
    float3 position : POSITION;
    float3 color : COLOR;
    
    uint vertex_id : SV_VertexID;
    uint instance_id : SV_InstanceID;
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
    
    float4 pos = float4(input.position, 1.0f);
    pos = mul(m_view, pos);
    pos = mul(m_projection, pos);
    output.position_clip = pos;
    
    output.color = input.color;
    
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color = float4(input.color, 1.0f);
    return color;
}