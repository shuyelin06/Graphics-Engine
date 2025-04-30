cbuffer CB0_LIGHTING_INFO : register(b0)
{
    float3 view_position;
    float padding;
}

#include "PerlinNoise.hlsli"
cbuffer CB1_NOISE_TABLE : register(b1)
{
    PerlinNoiseData noise_data;
}


struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    float3 sun_direction = normalize(float3(3.0f, 1.0f, 0.0));
    
    float3 shallow_color = float3(173.f / 255.f, 216.f / 255.f, 230.f / 255.f);
    float3 deep_color = float3(0, 0, 139.f / 255.f);
    float3 color = lerp(shallow_color, deep_color, pow(input.position_clip.z, 0.001f));
    
    float diffuse_term = max(dot(input.normal, sun_direction), 0);
    
    float3 view_direction = normalize(view_position - input.world_position);
    float3 light_direction_reflected = -sun_direction - 2 * dot(-sun_direction, input.normal) * input.normal;
    light_direction_reflected = normalize(light_direction_reflected);
    float specular_term = max(0, dot(view_direction, light_direction_reflected));
    specular_term = pow(specular_term, 40);
    
    float noise = perlinNoise2D(input.world_position.xz * 0.001f, noise_data);
    color = color * (clamp(diffuse_term + noise, 0.0f, 1.0f) + specular_term);
    return float4(color, 0.95f);
}