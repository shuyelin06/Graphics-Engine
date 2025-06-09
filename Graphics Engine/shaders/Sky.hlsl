#include "P_Common.hlsli"

struct VS_IN
{
    float4 position : SV_POSITION;
};

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

PS_IN vs_sky(VS_IN input)
{
    PS_IN output;
    output.position_clip = input.position;
    return output;
}

Texture2D render_target : register(t2);
Texture2D depth_map : register(t3);

cbuffer CB0_RESOLUTION : register(b0)
{
    float resolution_x;
    float resolution_y;
    
    float kernel;
    float padding;
}

cbuffer CB1_SUN_DATA : register(b1)
{
    float4x4 m_projection_to_world;
    
    float3 sun_direction;
    float padding_2;
    
    float3 camera_position;
    float padding_3;
}

float4 ps_sky(PS_IN input) : SV_TARGET
{
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    float2 offsets = float2(1.f / resolution_x, 1.f / resolution_y);
    
    float3 final_color = render_target.Sample(s_point, uv);
    
    // If depth == 1, then we are seeing our sky. 
    float depth = depth_map.Sample(s_point, uv).x;
    
    float4 world_pos = float4(uv.x * 2.f - 1, (1 - uv.y) * 2.f - 1, 0.5f, 1.f);
    world_pos = mul(m_projection_to_world, world_pos);
    world_pos = world_pos / world_pos.w;
    
    float3 n_view_vector = normalize(world_pos.xyz - camera_position);
    float3 n_sun_direc = normalize(sun_direction);
    
    if (depth >= 1.f && dot(n_view_vector, -n_sun_direc) >= 0.985f)
    {
        final_color = float3(1.f, 1.f, 0.f);
    }
    
    return float4(final_color, 1.f);
}
