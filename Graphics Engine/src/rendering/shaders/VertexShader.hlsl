cbuffer CB2 : register(b1)
{
    float4x4 m_view;
    float4x4 m_projection;
}

cbuffer CB3 : register(b2)
{
    float4x4 m_world;
    float4x4 m_normals;
}

/* Vertex Shader Input */
struct VS_IN
{
    float3 position_local : POSITION; // POSITION Semantic
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
    float4 norm = float4(input.normal, 1.0f);
	
    // Find World Position
    pos = mul(m_world, pos);
    output.world_position = pos.xyz;
    
    // Find Clipping Position
    pos = mul(m_view, pos);
    pos = mul(m_projection, pos);
    output.position_clip = pos;
    
    // Find normal
    norm = mul(m_normals, norm);
    output.normal = norm.xyz;
	
    return output;
}