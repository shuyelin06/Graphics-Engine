#include "P_Common.hlsli"

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

Texture2D render_target : register(t2);
Texture2D depth_map : register(t3);

cbuffer CB0_RESOLUTION : register(b0)
{
    float resolution_x;
    float resolution_y;
    
    float z_near;
    float z_far;
}

cbuffer CB1_PARAMS : register(b1)
{
    // Viewport Information
    float4x4 m_proj_to_world;
    float3 view_position;
    float padding;
    
    // Sun direction
    float3 sun_direction;
    // Sun size
    float sun_size;
    
    // Color Parameters
    float3 sun_color;
    float padding_2;
    float3 sky_color;
    float padding_3;
}

float4 ps_main(PS_IN input) : SV_TARGET
{
    // Figure out the texture coordinates that my pixel is on
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    // Sample my textures at this UV coordinate
    float depth = depth_map.Sample(s_point, uv);
    float3 output_color = render_target.Sample(s_point, uv);
    
    // Convert this to a viewing direction 
    float4 world_pos = mul(m_proj_to_world, float4(uv.x * 2.f - 1, (1 - uv.y) * 2.f - 1, 0.5f, 1.f));
    world_pos = world_pos / world_pos.w;
    float3 view_direction = normalize(world_pos.xyz - view_position);
    
    // Set fog as we get far away
    float fog_begin = 0.98f;
    float dist = (depth - fog_begin) / (1.f - fog_begin);
    dist = smoothstep(0.0f, 1.0f, clamp(dist, 0.0f, 1.f));
    output_color = lerp(output_color, sky_color, dist);
    
    // If depth == 1, we are seeing the sky. Blend between the sun and sky
    if (depth >= 1.f)
    {
        float dot_product = dot(-sun_direction, view_direction);
        float blend_factor = (dot_product - (1 - sun_size)) / sun_size;
        blend_factor = clamp(blend_factor, 0.0f, 1.f);
        
        output_color = lerp(sky_color, sun_color, blend_factor);
    }
    
    return float4(output_color, 1.f);
}
