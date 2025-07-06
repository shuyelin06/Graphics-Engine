#include "P_Common.hlsli"
#include "Utility.hlsli"
#include "Noise.hlsli"

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

Texture2D render_target : register(t2);
Texture2D depth_map : register(t3);

cbuffer CB1_PARAMS : register(b2)
{
    float3 sun_direction;
    float water_surface_height; // Water Surface Height 
    
    float3 sky_color;
    float scattering_multiplier;
    
    float3 rgb_attenuation;
    float attenuation_multiplier;
    
    int num_steps;
    float fog;
    float reflection_multiplier;
}

// Water Surface Point:
// For any world position, determines the point on the water surface obtained that can be seen
// along the sun's direction
float3 water_surface_point(float3 world_pos)
{
    float t = ((water_surface_height - world_pos.y) / sun_direction.y);
    return world_pos + sun_direction * t;
}

// Transmittance:
// Determines the amount of light transmitted (in range [0,1]) based on the optical depth.
// This is calculated by integration of density over an entire viewing ray; alternatively,
// this can iterativelysampled through sum(density_at_point * step_size)
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

float4 ps_main(PS_IN input) : SV_TARGET
{
    float3 output_color = float3(0, 0, 0);
    
    // Figure out the corresponding texture coordinate for my pixel
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    // Use this uv to obtain the following:
    // 1) The current color + depth at this pixel. This represents the light 
    //    directly reflected towards the camera at this angle
    float3 color = render_target.Sample(s_point, uv);
    float depth = depth_map.Sample(s_point, uv).x;
    // 2) My viewing directional vector (un-normalized).
    float4 world_pos = float4(uv.x * 2.f - 1, 1.f - 2.f * uv.y, depth, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos /= world_pos.w;
    float world_depth = length(world_pos.xyz - view_pos);
    float3 view_vector = world_pos.xyz - view_pos;
    
    // Compute the amount of light reaching the view reflected off of some surface
    float3 reflect_contribution = color * sky_color * reflection_multiplier;
    float3 ambient_contribution = sky_color * reflection_multiplier * transmittance(750 + (water_surface_height - view_pos.y) * scattering_multiplier);
    
    float3 trans = transmittance(length(water_surface_point(world_pos.xyz) - world_pos.xyz));
    trans *= phase_rayleigh(view_vector, sun_direction);
    trans *= transmittance(world_depth);
    reflect_contribution *= trans;
    
    output_color = lerp(reflect_contribution, ambient_contribution, pow(depth, fog));
    
    /*
    float3 dither = gradient3D(world_pos.xyz, float3(10, 11, 17));
    output_color += (dither / 255.f);
    */
    
    return float4(output_color, 1.f);
}
