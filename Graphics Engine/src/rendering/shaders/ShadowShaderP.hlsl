// Light ShadowMap & Sampler
Texture2D depthMap : register(t0);
SamplerState depthSampler : register(s0);

// Per-View Data
cbuffer CB1 : register(b1)
{
    float3 light_position;
    float padding;
    row_major float4x4 m_view;
    row_major float4x4 m_projection;
}

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET
{
    // Check if the point is shadowed. First, get the depth of the
    // point to the light by converting it to the light projection
    // space
    float4 view_position = float4(input.world_position, 1);
    view_position = mul(view_position, m_view);
    view_position = mul(view_position, m_projection);
    view_position = view_position / view_position.w; // Manual W-Divide 
    float depth = view_position.z;
    
    // Now, sample the light's shadow map to see its depth at that location, i.e.
    // how "far" it can see.
    
    float2 shadowmap_coords = float2((view_position.x + 1) / 2.f, (-view_position.y + 1) / 2.f);
    // float2 shadowmap_coords = float2((view_position.x - 1) * 2.f, (view_position.y - 1) * 2.f);
    float4 sampledValue = depthMap.Sample(depthSampler, shadowmap_coords);
    float sampledDepth = sampledValue.r;
    
    // If sampled depth is < depth, that means the light cannot see the
    // point, so its in shadow.
    // To represent this,
    float inShadow = step(depth, sampledDepth + 0.005f);
    
    // Calculate attenuation of light
    float3 light_direction = light_position - input.world_position;
    // float angle = dot(normalize(light_direction), input.normal);
    float angle = 1;
    
    float4 color = float4(0, 0, 0, 1.0f);
    color.x = inShadow * 10 * angle / (length(light_direction) + 1);
    
    return color;
}
