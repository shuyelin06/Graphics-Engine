cbuffer CB0_LIGHTING_INFO : register(b0)
{
    float3 view_position;
    float padding;
    
    float3 sun_direction;
    float padding2;
    
    float3 sun_color;
    float padding3;
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
    
    float3 color = float3(0.03f, 0.07f, 0.45f);
    
    // Compute my diffuse constant
    float diffuse_term = max(dot(input.normal, -sun_direction), 0);
    
    // Compute my specular constant 
    float3 view_direction = normalize(view_position - input.world_position);
    float3 light_direction_reflected = sun_direction - 2 * dot(sun_direction, input.normal) * input.normal;
    light_direction_reflected = normalize(light_direction_reflected);
    float specular_term = max(0, dot(view_direction, light_direction_reflected));
    specular_term = pow(specular_term, 55);
    
    // Apply diffuse and specular term
    color = color * diffuse_term + sun_color * specular_term;
    
    return float4(color, 1.f);
}