// Per-View Data
cbuffer CB1 : register(b1)
{
    row_major float4x4 m_view;
    row_major float4x4 m_projection;
}

// Per-Mesh Data
cbuffer CB2 : register(b2)
{
    row_major float4x4 m_world;
    row_major float4x4 m_normals;
}

/* Vertex Shader Input */
struct VS_IN
{
    float3 position_local : POSITION;
    float2 tex : TEXTURE;
    float3 normal : NORMAL;
};

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

// Vertex Shader Entry Point - Takes VS_IN and outputs a VS_OUT
// Takes a position in local coordinates, and translates them to clipping
// coordinates
// 
// Vertex Shaders MUST output a float4 XYZW setting a vertex's position in homogeneous clip space
// Must be between -1 and 1 on X and Y axes, and 0 and 1 on Z (positive into the screen). 
// Homogeneous coordiantes are identified by the SV_POSITION semantic
VS_OUT vs_main(VS_IN input)
{
	// Zero the Memory
    VS_OUT output = (VS_OUT) 0;
    
    float4 pos = float4(input.position_local, 1.0f);
    float4 norm = float4(input.normal, 0.0f);
	
    // Find World Position
    pos = mul(pos, m_world);
    output.world_position = pos.xyz;
    
    // Find Clipping Position
    pos = mul(pos, m_view);
    pos = mul(pos, m_projection);
    output.position_clip = pos;
    
    // Compute normal and normalize it for lighting
    norm = mul(norm, m_normals);
    output.normal = normalize(norm.xyz);
	
    return output;
}