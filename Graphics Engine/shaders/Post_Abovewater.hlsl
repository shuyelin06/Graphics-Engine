#include "P_Common.hlsli"
#include "Utility.hlsli"

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

Texture2D render_target : register(t2);
Texture2D depth_map : register(t3);

cbuffer CB2_PARAMS : register(b2)
{
    // Sun Configuration 
    float3 sun_direction;
    float sun_size;
    float3 sun_color;
    float p_cb2_0;
    
    // Atmospheric Parameters
    float density_falloff;
    float atmosphere_height;
    float max_distance;
    int num_steps_atmosphere;
    
    float3 scattering_coefficients;
    int num_steps_optical_depth;
    
    float reflective_strength;
}

// Density:
// Given a point in the world, returns the density at that point as a value in [0,1].
float density(float3 world_pos)
{
    // The water line is at y = 0. Normalize to [0,1]
    float height = world_pos.y / atmosphere_height;
    float local_density = exp(-height * density_falloff) * (1 - height);
    return local_density;
}

// Optical Depth:
// Given a ray, samples the ray at various points to determine the optical depth,
// a measure of the air's density that the ray passes through. The higher the optical depth,
// the more light gets attenuated.
float optical_depth(float3 ray_pos, float3 ray_direction, float ray_length)
{
    float3 sample_point = ray_pos;
    float step_size = ray_length / (num_steps_optical_depth - 1);
    
    float od = 0.f;
    
    for (int i = 0; i < num_steps_optical_depth; i++)
    {
        od += density(sample_point) * step_size;
        sample_point += ray_direction * step_size;
    }
    
    return od;
}

float4 ps_main(PS_IN input) : SV_TARGET
{
    float3 output_color = float3(0, 0, 0);
    
    // Figure out the texture coordinates that my pixel is on
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    // Sample my textures at this UV coordinate, and determine the XYZ world position
    // of this point.
    float depth = depth_map.Sample(s_point, uv);
    float world_depth = depth * (view_zfar - view_znear) + view_znear;
    float3 color = render_target.Sample(s_point, uv);
    output_color = color * exp(-world_depth * scattering_coefficients);
    
    // Convert this to a viewing direction 
    float4 world_pos = mul(m_screen_to_world, float4(uv.x * 2.f - 1, (1 - uv.y) * 2.f - 1, 0.5f, 1.f));
    world_pos = world_pos / world_pos.w;        
    
    // Find the distance we are from the atmosphere
    float3 view_direction = normalize(world_pos.xyz - view_pos);
    float3 atmosphere_origin = float3(0, atmosphere_height, 0);
    float atmosphere_normal = float3(0, -1, 0);
    
    float3 ray_position = view_pos;
    float3 ray_direction = view_direction;
    
    float t = ray_plane_intersection(ray_position, ray_direction, atmosphere_origin, atmosphere_normal);
    t = min(max_distance, t);
    t = lerp(t, max_distance, t < 0);
    
    float step_size = t / (num_steps_atmosphere - 1);
    float3 sun_contribution = float3(0, 0, 0);
    
    for (int i = 0; i < num_steps_atmosphere; i++)
    {
        // Find the optical depth of the light as it passes from the sun --> the ray point -->
        // the view eye. This will tell us how much of the sun's light is attenuated as it
        // passes to the camera. 
        float d_point_to_sun = ray_plane_intersection(ray_position, -sun_direction, atmosphere_origin, atmosphere_normal);
        d_point_to_sun = min(max_distance, d_point_to_sun);
        d_point_to_sun = lerp(d_point_to_sun, max_distance, d_point_to_sun < 0);
        
        float od_sun_to_point = optical_depth(ray_position, -sun_direction, d_point_to_sun);
        float od_point_to_view = optical_depth(view_pos, ray_direction, step_size * i);
        
        float3 transmittance = exp(-(od_sun_to_point + od_point_to_view) * scattering_coefficients);
        float local_density = density(ray_position);
        
        // Now, find the amount of light that transmits to our view.
        sun_contribution += transmittance * scattering_coefficients * local_density * step_size;
        
        // Step to next position
        ray_position += ray_direction * step_size;
    }
    
    // If depth == 1, we are seeing the sky. Blend between the sun and sky 
    if (depth >= 1.f)
    {
        float dot_product = dot(-sun_direction, view_direction);
        float blend_factor = (dot_product - (1 - sun_size)) / sun_size;
        blend_factor = clamp(blend_factor, 0.0f, 1.f);
        
        output_color = lerp(sun_contribution, sun_color, blend_factor);
    }
    
    // Set fog as we get far away
    /*
    float fog_begin = 0.98f;
    float dist = (depth - fog_begin) / (1.f - fog_begin);
    dist = smoothstep(0.0f, 1.0f, clamp(dist, 0.0f, 1.f));
    float3 temp = output_color;
    output_color = color * (1 - dist) + temp * (dist);
    */
    
    return float4(output_color, 1.f);
}
