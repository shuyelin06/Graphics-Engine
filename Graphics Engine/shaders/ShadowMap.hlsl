// Vertex CB0:
// Stores the light view and projection matrices,
// so that we can transform a world point into the light's view
cbuffer LIGHT_TRANSFORM : register(b2)
{
    float4x4 m_view;
    float4x4 m_projection;
}

// Vertex CB1:
// Contains the LocalToWorld matrix for the vertices of the 
// input.
cbuffer MESH_TRANSFORM : register(b1)
{
    float4x4 m_world;
}

// Vertex / Pixel Input.
// For writing to a depth map, we just need to properly transform
// the vertex from local space --> the light's view space.
// The GPU will automatically write to the depth buffer.
struct VS_IN
{
    float3 position_local : POSITION;
};

struct VS_OUT
{
    float4 position_clip : SV_POSITION;
};

// Vertex Shader:
// Transforms vertices into the light's view
VS_OUT vs_main(VS_IN input)
{
    // Zero the Memory
    VS_OUT output = (VS_OUT) 0;
    
    float4 position = float4(input.position_local, 1.0f);
    position = mul(m_world, position); // Local -> World
    position = mul(m_view, position); // World -> Light View
    position = mul(m_projection, position); // Light View -> Clipping
    
    output.position_clip = position;
    
    return output;
}

// Pixel Shader
// Does nothing; the vertex shader output is automatically
// written to the depth map. 
float4 ps_main(VS_OUT input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}