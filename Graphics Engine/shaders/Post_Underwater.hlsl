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
    float water_surface_height;
    
    float sky_multiplier;
    float scattering_multiplier;
    float attenuation_multiplier;
    float fog_multiplier;
    
    float3 rgb_attenuation;
    int num_steps;
    
    float max_distance;
    float multiplier;
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

float4 ps_main(PS_IN input) : SV_TARGET
{
    float3 output_color = float3(0, 0, 0);
    
    // Figure out the corresponding texture coordinate for my pixel
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    // Use this uv to obtain the following:
    // 1) The current color + depth at this pixel. This represents the light 
    //    directly reflected towards the camera at this angle
    // 2) My viewing directional vector
    float3 color = render_target.Sample(s_point, uv);
    float depth = depth_map.Sample(s_point, uv).x;
    color = lerp(color, float3(0, 0, 0), pow(depth, fog_multiplier));
    
    float4 world_pos = float4(uv.x * 2.f - 1, 1.f - 2.f * uv.y, depth, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos /= world_pos.w;
    float world_depth = length(world_pos.xyz - view_pos);
    float3 view_vector = normalize(world_pos.xyz - view_pos);
    
    // Determine the amount of light reaching the view. To do this, we will start with an
    // initial color, and raymarch towards our camera.
    // During this march, we will add in ambient contribution and attenuate.
    float3 ray_position = view_pos;
    float3 ray_direction = view_vector;
    
    float ray_length = ray_surface_distance(ray_position, ray_direction, water_surface_height);
    ray_length = min(world_depth, ray_length);
    float ray_step = ray_length / (num_steps - 1);
    
    float3 ray_end = ray_position + ray_length * ray_direction;
    
    // Start with the color we see. Attenuate it based on how far it is from the surface
    output_color = color; // Initial Color
    float dist = ray_surface_distance(world_pos.xyz, -sun_direction, water_surface_height);
    output_color *= transmittance(dist);
    
    // Add direct lighting from the sky.
    if (ray_end.y >= water_surface_height - 0.01f)
    {
        output_color += float3(0, 1, 1) * transmittance(ray_length) * sky_multiplier;
    }
    
    for (int i = 0; i < num_steps; i++)
    {
        float d_surface_to_point = ray_surface_distance(ray_end, -sun_direction, water_surface_height);
        float3 ambient_contribution = transmittance(d_surface_to_point) * ray_step * scattering_multiplier;
        output_color += ambient_contribution;
        
        output_color *= transmittance(ray_step);
        
        ray_end -= ray_direction * ray_step;
    }
    
    // Dither my pixels to reduce banding
    float3 dither = gradient3D(world_pos.xyz, float3(10, 11, 17));
    output_color += (dither / 255.f);
    
    return float4(output_color, 1.f);
}
