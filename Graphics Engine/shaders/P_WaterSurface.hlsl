#include "Utility.hlsli"

struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
    
    float3 normal : NORMAL;
    
    float3x3 m_tbn : TEXCOORD1;
};

SamplerState tex_sampler : register(s0);
Texture2D depth_map : register(t0);
Texture2D bump_map : register(t1);

cbuffer CB0_LIGHTING_INFO : register(b0)
{
    float3 view_position;
    float padding;
    
    float3 sun_direction;
    float padding2;
    
    float3 sun_color;
    float padding3;
    
    float2 resolution;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float2 uv = clip_to_uv(input.position_clip, resolution);
    
    
    input.normal = bump_to_normal(bump_map.Sample(tex_sampler, input.world_position.xz / 300.f).rgb, input.m_tbn);
    
    float3 ambient_color = float3(0.3f, 0.27f, 0.75f);
    float3 color = float3(0.03f, 0.07f, 0.75f);
    
    // Compute my diffuse constant
    float diffuse_term = max(dot(input.normal, -sun_direction), 0);
    
    // Compute my specular constant 
    float3 view_direction = normalize(view_position - input.world_position);
    float3 light_direction_reflected = reflect(sun_direction, input.normal);
    
    float fresnel = pow(1.0f - saturate(dot(view_direction, input.normal)), 5.0f);
    
    float specular_term = max(0, dot(view_direction, light_direction_reflected));
    specular_term = pow(specular_term, 55);
    float3 specular = sun_color * specular_term;
    
    // Apply diffuse and specular term
    // color = color * diffuse_term + sun_color * specular_term;
    
    // Apply fresnel reflective term
    
    color = float3(0.0f, 0.0f, 0.12f) + fresnel * (ambient_color + specular);
    // lerp(color, ambient_color, fresnel);
    
    // My alpha will depend on the sampled depth
    
    float depth = depth_map.Sample(tex_sampler, uv);
    float alpha = 1.f;
    
    if (depth < 1.f - 0.001f && input.position_clip.z < depth)
    {
        float min_alpha = 0.75f;
        float water_depth = depth - input.position_clip.z;
        
        alpha = min_alpha + exp(-water_depth) * (1 - min_alpha);
    }
    
    return float4(color, alpha);
}