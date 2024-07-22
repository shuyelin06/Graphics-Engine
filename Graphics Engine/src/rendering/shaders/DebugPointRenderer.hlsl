// 8 Floats / Point * 4 Bytes / Float 
// = 32 Bytes / Point
struct PointData
{
    float3 position;
    float scale;
    float3 color;
    float padding;
};

// Maximum Allowable Points:
// 4096 Vectors * 4 Components per Vector / 8 Components per Point
// = ~2048 Points
cbuffer CB0 : register(b0)
{
    // Stores data for each point
    PointData points[2048];
}

cbuffer CB1 : register(b1)
{
    row_major float4x4 m_view;
    row_major float4x4 m_projection;
}

struct VS_INPUT
{
    // Position of one of the cube's vertices
    float3 vertex_position : POSITION;
    
    // UNUSED
    float2 tex_UNUSED : TEXTURE;
    float3 norm_UNUSED : NORMAL;
    
    // Unique identifier for the cube (point)
    uint instance : SV_InstanceID;
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
    
    // Obtain data for this instance
    PointData data = points[input.instance];
    
    // Transform vertex position based on per-point data
    input.vertex_position = input.vertex_position * data.scale;
    input.vertex_position = input.vertex_position + data.position;
    
    // Transform to camera space
    float4 pos = float4(input.vertex_position, 1.0f);
    pos = mul(pos, m_view);
    pos = mul(pos, m_projection);
    
    output.position_clip = pos;
    
    // Pass color to pixel shader
    output.color = data.color;
    
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color = float4(input.color, 1.0f);
    return color;
}