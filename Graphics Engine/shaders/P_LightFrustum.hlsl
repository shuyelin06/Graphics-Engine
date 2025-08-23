#include "P_Common.hlsli"
#include "Lighting.hlsli"
#include "Utility.hlsli"
#include "Noise.hlsli"

struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
    uint instance : SV_InstanceID;
};

Texture2D depth_map : register(t3);
Texture2D depth_map_forward : register(t4);

cbuffer CB1_PARAMS : register(b2)
{
    float3 sun_direction_;
    float water_surface_height;
    
    float sky_multiplier;
    float scattering_multiplier;
    float attenuation_multiplier;
    float fog_multiplier;
    
    float3 rgb_attenuation;
    int num_steps;
    
    float max_distance;
    float multiplier;
    float multiplier2;
}

float ray_surface_distance(float3 ray_position, float3 ray_direction, float surface_height)
{
    float t = ray_plane_intersection(ray_position, ray_direction, float3(0, surface_height, 0), float3(0, -1, 0));
    t = min(max_distance, t);
    t = lerp(t, max_distance, t < 0);
    
    return t;
}

// Transmittance:
// Determines the amount of light transmitted (in range [0,1]) based on the optical depth.
// This is the integration of density over an entire viewing ray.
float3 transmittance(float depth)
{
    return exp(-rgb_attenuation * depth * attenuation_multiplier);
}

// Rayleigh Phase Function:
// Given an input vector and output vector, returns the amount of light scattered
// from the input to the output vector through Rayleigh scattering.
float phase_rayleigh(float3 in_direc, float3 out_direc)
{
    // First, find cos(theta) of the two vectors using the dot product.
    float cos_theta = dot(in_direc, out_direc) / (length(in_direc) * length(out_direc));

    // Now, use approximation given here:
    // https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering
    // Where g = 0
    return (3.f / (4.f * 3.14159f)) * (1 + cos_theta * cos_theta);
}

float4 forward_pass(PS_INPUT input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    // We will raymarch through the light volume, and attenuate the light color.
    // This will give us a volumetric lighting effect.
    // First, determine my ray viewing vector, and how far it goes.
    float volume_start = depth_map_forward.Sample(s_point, uv);
    if (volume_start == 1.f)
        volume_start = 0.f;
    float volume_end = input.position_clip.z;
    float depth = depth_map.Sample(s_point, uv);
    
    float4 world_pos = float4(uv.x * 2.f - 1, 1.f - 2.f * uv.y, volume_start, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos /= world_pos.w;
    // Bias value needed for shadow test to not flicker
    volume_start = length(world_pos.xyz - view_pos) + 0.1f;
    
    world_pos = float4(uv.x * 2.f - 1, 1.f - 2.f * uv.y, volume_end, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos /= world_pos.w;
    volume_end = length(world_pos.xyz - view_pos) - 0.1f;
    
    world_pos = float4(uv.x * 2.f - 1, 1.f - 2.f * uv.y, depth, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos /= world_pos.w;
    depth = length(world_pos.xyz - view_pos);
    
    float3 view_vector = normalize(world_pos.xyz - view_pos);
    volume_end = min(depth, volume_end);
    
    // Begin my ray march.
    float3 ray_position = view_pos + volume_start * view_vector;
    float3 ray_direction = view_vector;
    
    float ray_length = (volume_end - volume_start);
    float ray_step = ray_length / (num_steps - 1);
    
    float3 light_contribution = float3(0, 0, 0);
    LightData light = light_instances[input.instance];
    
    for (int i = 0; i < num_steps; i++)
    {
        float d_light_to_point = length(ray_position - light.position);
        float shadow_factor = shadowValue(ray_position, light, 0.001f);
        
        float3 step_contribution = transmittance(d_light_to_point) * ray_step * scattering_multiplier;
        step_contribution *= shadow_factor;
        step_contribution *= multiplier2 / (1 + multiplier * d_light_to_point * d_light_to_point);
        // step_contribution *= phase_rayleigh(normalize(ray_position - light.position), -ray_direction);
        step_contribution *= transmittance(i * ray_step) * scattering_multiplier;
        
        light_contribution += step_contribution;
        
        ray_position += ray_direction * ray_step;
    }
    
    return float4(light_contribution, 1.f);
    // return float4(1.f, 1.f, 1.f, 1.f);

}