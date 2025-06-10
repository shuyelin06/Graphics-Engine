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
    // Precomputed A, B, C on the Fog Curve
    float3 fog_params;
    // Fog Visibility (1 / Density)
    float visibility;
    
    // Surface Height (Where things are brightest)
    float surface_height;
    
    // Surface Color
    float3 shallow_waters;
    
    // Intensity Drop
    // How fast it gets darker as you get deeper
    float intensity_drop;
    
    // Abyss Color
    float3 deep_waters;
}

float4 ps_main(PS_IN input) : SV_TARGET
{
    // Figure out the texture coordinates that my pixel is on, and offsets to the 
    // next texel (if needed).
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    // Now, sample for the current color in the render target and apply fog
    // to add a water effect.
    // The further we can see, the more "particles" there are scattering light.
    // Fog is a blend between a bright (surface) color and a dark (deep)
    // color. These colors darken as you get & look deeper.
    float3 color = render_target.Sample(s_point, uv);
    
    // Compute the world position and viewing vector, to see what color fog we see.
    // The higher we look, the brighter the color is
    float4 world_pos = float4(uv.x * 2.f - 1, 1 - 2 * uv.y, 1.0f, 1.f);
    world_pos = mul(m_screen_to_world, world_pos);
    world_pos = world_pos / world_pos.w;
    float3 view_vector = normalize(world_pos.xyz - view_pos);
    
    float fog_color_factor = view_vector.y / 2.f + 0.5f;
    float3 fog_color = lerp(deep_waters, shallow_waters, fog_color_factor);
    
    // Sample for the depth of our view in world coordinates, to determine how much
    // our fog contributes to the color we see. The further we can see, the more contribution 
    // our fog has.
    float depth = depth_map.Sample(s_point, uv).x;
    float world_depth = depth * (view_zfar - view_znear) + view_znear;
    
    float fog_factor = fog_params.x * world_depth * world_depth + fog_params.y * world_depth + fog_params.z;
    fog_factor = pow(fog_factor, visibility);
    color = lerp(color, fog_color, fog_factor);
    
    // Determine our distance from the surface. The deeper we are, the darker the color 
    // we see.
    float height = max(0, surface_height - view_pos.y);
    float intensity = pow(10.f, 1 - intensity_drop * height) * 0.1f;
    color *= intensity;
    
    // Add a dither to reduce banding
    float3 dither = gradient3D(world_pos.xyz, float3(10, 11, 17));
    color += (dither / 255.f);
    
    return float4(color, 1.f);
}
