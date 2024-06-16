// Define unique per-point data
struct PointData
{
    // Defines the point's position
    float3 position;
    
    // Defines the point's scale
    float scale;
    
    // Defines the point's RGB color
    float3 color;
    
    // Padding to make sure the struct is 4-byte aligned
    float padding;
};

// Constant buffer storing transformation matrices
cbuffer TRANSFORM_MATRICES : register(b0)
{
    row_major float4x4 m_worldToCamera;
}

// Constant buffer storing per-point data
// Maximum Allowable Points:
// 4096 Vectors * 4 Components per Vector / 8 Components per Point
// = ~2048 Points
cbuffer POINTS : register(b1)
{
    // Stores data for each point
    PointData points[2048];
}

// Vertex Shader Input
struct VS_INPUT
{
    // Stores the position of one of the cube's vertices
    float3 vertex_position : POSITION;
    
    // Unique identifier for the cube
    uint instance : SV_InstanceID;
};

// Pixel Shader Input (Vertex Shader Output)
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
    output.position_clip = mul(pos, m_worldToCamera);
    
    // Pass color to pixel shader
    output.color = data.color;
    
    return output;
}

// Pixel Shader:
// Returns the input color specified
float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color = float4(input.color, 1.0f);
    return color;
}