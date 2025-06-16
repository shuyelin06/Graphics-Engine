#include "P_Common.hlsli" // ShadowMap Sampler Binding

// Lighting.hlsli
// Contains the data and methods neeeded for lighting calculations.
// Uses the following resource slots:
// Texture t1: Shadow map atlas
// Sampler s1: Sampler for the shadow map atlas
// CB1: Constant buffer 1 to store the lighting data.
struct LightData
{
    float3 position;
    float pad0;
    
    float3 color;
    float pad1;
    
    float4x4 m_view;
    float4x4 m_projection;
    
    float tex_x;
    float tex_y;
    float tex_width;
    float tex_height;
};

cbuffer CB1_LIGHTING_DATA : register(b1)
{
    int light_count;
    
    // Global Lighting (Sunlight) Information
    // The # Cascades should match the SUN_NUM_CASCADES value defined in Sunlight.h
    float3 sun_direction;
    
    float2 cascade_thresholds; // NUM_CASCADES - 1
    float2 cb1_p0; // Padding
    LightData sun_cascades[3];

    // Local Lighting Data
    LightData light_instances[7];
};

// --- Sun Cascades ---
// Given the world position of a pixel, computes the sun cascade that the pixel is in.
float selectCascade(float3 world_position)
{
    // Compute the distance of the pixel position to the camera,
    // and normalize it so that it is between [0,1].
    float distance = length(world_position - view_pos) - view_znear;
    
    // Based on the distance, select a cascade index and return it.
    // These distance thresholds should match what's defined in SunLight.h.
    float cascade = 0;
    cascade = cascade + step(cascade_thresholds.x * view_zfar, distance);
    cascade = cascade + step(cascade_thresholds.y * view_zfar, distance);
        
    return cascade;
}

// ShadowValue:
// Given data for a light and the world position of a pixel, computes the shadow value
// of the pixel. This is a value from [0,1], indicating how "shadowed" the pixel is.
// For hard shadowing, this value will be 0 or 1, but can be imbetween with soft shadows.
float shadowValue(float3 world_position, LightData light, float bias)
{
    // Convert the position into the light's coordinates
    float4 view_coords = float4(world_position, 1.f);
    
    view_coords = mul(light.m_view, view_coords);
    view_coords = mul(light.m_projection, view_coords);
    view_coords = view_coords / view_coords.w;

    float shadow_value = 0.f;
    
    // If the point is outside of the shadow map, it is automatically "in shadow".
    if (-1.f <= view_coords.x && view_coords.x <= 1.f)
    {
        if (-1.f <= view_coords.y && view_coords.y <= 1.f)
        {
            // Otherwise, sample the light's shadow map to see if the pointis in shadow.
            // First, convert the coordinates of the pixel to u,v coordinates in the light shadow map.
            float2 shadowmap_coords = float2((view_coords.x + 1) / 2.f, (1.f - view_coords.y) / 2.f);
            // Because the shadowmap is in the shadow atlas, transform the coordinates to the position in the shadow
            // atlas.            
            shadowmap_coords = float2(shadowmap_coords.x * light.tex_width + light.tex_x, shadowmap_coords.y * light.tex_height + light.tex_y);
            
            // Pull the depth of our point.
            float cur_depth = view_coords.z;
            // Sample the shadow map depth. 
            float depth = shadow_atlas.Sample(s_shadow, shadowmap_coords).x;
            
            // Perform a shadow test by seeing if the point's depth exceeds the depth. 
            // If sampled_depth is < depth, the light cannot see the point, so it provides
            // no contribution to the color at that point (point is in shadow). 
            // We add a bias in the test to reduce shadow acne due to imprecision.
            shadow_value = step(cur_depth, depth + bias);
            
            /*
            float sampled_depth = shadow_atlas.Sample(shadowmap_sampler, shadowmap_coords).r;
            shadow_value = step(cur_depth, sampled_depth + bias);
            */
        }
    }
    
    return shadow_value;
}

// Diffuse:
// Evaluate the diffuse contribution for a light
float diffuseValue(LightData light, float3 world_pos, float3 normal)
{
    float3 light_direc = normalize(light.position - world_pos);
    float diffuse = max(0, dot(light_direc, normal));
    return diffuse;
}