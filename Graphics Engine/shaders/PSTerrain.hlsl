#include "Lighting.hlsli"
#include "ToneMap.hlsli"

SamplerState mesh_sampler : register(s0);
SamplerState shadowmap_sampler : register(s1);

// Mesh Data
Texture2D terrain_texture : register(t0);

// Illumination (Global + Local)
struct LightData
{
    float3 position;
    uint shadowmap_width;
    
    float3 color;
    uint shadowmap_height;
    
    float4x4 m_view;
    float4x4 m_projection;
};

Texture2D lightDepthMaps[10] : register(t1);

cbuffer CB1 : register(b1)
{
    float3 view_world_position;
    int light_count;
    
    LightData light_instances[10];
    
}

/* Vertex Shader Output (Pixel Shader Input) */
struct PS_IN
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 psterrain_main(PS_IN input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 1);
       
    // Texture coordinate is the xz coordinate, scaled to some value
    float scale = 0.1;
    
    float3 xz_color = terrain_texture.Sample(mesh_sampler, input.world_position.xz * scale).rgb;
    float xz_contribution = abs(dot(float3(0, 1, 0), input.normal));
    
    float3 xy_color = terrain_texture.Sample(mesh_sampler, input.world_position.xy * scale).rgb;
    float xy_contribution = abs(dot(float3(0, 0, 1), input.normal));
    
    float3 yz_color = terrain_texture.Sample(mesh_sampler, input.world_position.yz * scale).rgb;
    float yz_contribution = abs(dot(float3(1, 0, 0), input.normal));
    
    float3 mesh_color = xz_color * xz_contribution + xy_color * xy_contribution + yz_color * yz_contribution;
    
    // Ambient Lighting
    float ambient = 0.35f;
    color.rgb += mesh_color * ambient;
    
    for (int i = 0; i < light_count; i++)
    {
        // Find the point's coordinates in the light view
        float4 lightview_coords = toLightView(input.world_position, light_instances[i].m_view, light_instances[i].m_projection);
        
        if (-1 <= lightview_coords.x && lightview_coords.x <= 1)
        {
            if (-1 <= lightview_coords.y && lightview_coords.y <= 1)
            {   
                // Check if the point's depth exceeds that of the light's view
                // If sampled depth is < depth, the light cannot see the point, so it provides
                // no contribution to the color at that point (point is in shadow). 
                // We add a small offset 0.01 in the step() function to avoid precision errors
                float cur_depth = lightview_coords.z;
                 
                float2 shadowmap_coords = float2((lightview_coords.x + 1) / 2.f, (-lightview_coords.y + 1) / 2.f);
                float offset_x = 1.0f / float(light_instances[i].shadowmap_width);
                float offset_y = 1.0f / float(light_instances[i].shadowmap_height);
                
                // Normal Shadow Map
                // lightDepthMaps[i].Sample(shadowmap_sampler, shadowmap_coords).r;
                // Attempt at PCF
                float shadow_factor = 0.0f;
                
                for (int x = -1; x <= 1; x++)
                {
                    for (int y = -1; y <= 1; y++)
                    {
                        float2 sample_coords = shadowmap_coords + float2(x * offset_x, y * offset_y);
                        float sampled_depth = lightDepthMaps[i].Sample(shadowmap_sampler, sample_coords).r;
                        float inShadow = step(cur_depth, sampled_depth + 0.01f);
                        
                        shadow_factor += inShadow;
                    }
                }

                shadow_factor = shadow_factor / 9.0f; // Doesn't work very well
    
                // --- Calculate vectors necessary for lighting ---
                // 1) Normal at Point 
                float3 normal = normalize(input.normal);
                                
                // 2) Light --> Point Direction & Distance
                float3 light_direction = input.world_position - light_instances[i].position;
                float light_distance = length(light_direction);
                light_direction = light_direction / light_distance;
                
                // 3) Point --> View Direction
                float3 view_direction = view_world_position - input.world_position;
                view_direction = normalize(view_direction);
                
                // 4) Reflected Light --> Point Direction Across Surface Normal
                float3 light_direction_reflected = light_direction - 2 * dot(light_direction, normal) * normal;
                light_direction_reflected = normalize(light_direction_reflected);
                            
                // --- Diffuse Contribution ---
                // See how close the normal is to the direction to the light.
                // Flip the light direction since it points from light --> surface
                float diffuseContribution = max(0, dot(-light_direction, normal));
                
                // Contribution from the material (0 -> 1, 1 meaning perfect reflection; 0.5: kinda okay)
                // diffuseContribution *= 0.65f;
                
                // --- Specular Contribution ---
                float specularContribution = 0;
                // Compare how close the view direction is to the reflected light direction.
                // float specularContribution = max(0, dot(view_direction, light_direction_reflected));
                
                // Apply a shininess contribution from the material (0 -> infty, 50+ is pretty shiny)
                // specularContribution *= pow(specularContribution, 0);
                
                // --- Attenuation Factor ---
                // If not the sun, factor in attenuation (distance to light)
                float attenuationContribution = 1;

                if (i != 0)
                    attenuationContribution = 1 / (1 + 0.1 * light_distance);
                
                float3 light_color = light_instances[i].color;
                
                float3 combined_color = (mesh_color * light_color);
                float totalContribution = shadow_factor * attenuationContribution * (diffuseContribution + specularContribution);
                color.rgb += float3(totalContribution * combined_color);

            }
        }
    }
    
    // Tone Mapping (ToneMap.hlsli):
    // Each color channel can have an intensity in the range [0, infty). We need to "tone map" it,
    // by passing it into a function that will map this to the [0, 255] RGB range.
    color = tone_map(color, light_count);
    
    return color;
}
