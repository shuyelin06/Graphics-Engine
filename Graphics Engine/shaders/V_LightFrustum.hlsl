// Each frustum is defined by a frustum to world matrix.
// We use instancing, and pass a normalized unit cube
// to the shader to be transformed into our frustum
cbuffer CB0_FRUSTUM_MATRICES : register(b0)
{
    float4x4 m_world_to_screen;
    float4x4 m_frustum_to_world[1000];
}

struct VS_INPUT
{
    // Position of the frustum corners
    float3 frustum_point : POSITION;
    
    // Unique identifier for the frustum 
    uint instance : SV_InstanceID;
};

struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // Compute world position
    float4 clip_point = float4(input.frustum_point, 1.f);
    clip_point = mul(m_frustum_to_world[input.instance], clip_point);
    output.world_position = clip_point.xyz;
    
    // Compute screen position 
    clip_point = mul(m_world_to_screen, clip_point);
    output.position_clip = clip_point;
    
    return output;
}