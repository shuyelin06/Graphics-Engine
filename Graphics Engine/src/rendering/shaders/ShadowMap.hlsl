// VS Constant Buffer 1:
// Stores the transformation matrices needed to
// transform the 
cbuffer TRANSFORM_MATRICES : register(b0)
{
    float4x4 m_modelToLight;
}

// Vertex Shader Input
struct VS_IN
{
    // Position of vertex in space
    float3 position : POSITION;
};

// Pixel Shader Input
// (Vertex Shader Output)
struct PS_IN
{
    // (Light)View-position (depth buffer position) of pixel
    float4 position_clip : SV_POSITION;   
};

// Vertex Shader:
// Transforms vertices into the light's view
PS_IN vs_main(VS_IN input)
{
    // Generate output
    PS_IN output = (PS_IN) 0;
    
    // Create 4D position vector with w = 1 value
    float4 vertex_pos = float4(input.position, 1);
    output.position_clip = mul(vertex_pos, m_modelToLight);
    
    return output;
}

// Pixel Shader
// Does nothing; the vertex shader output is automatically
// written to the depth map. 
void ps_main(PS_IN input) { }