#include "ToneMap.hlsli"

SamplerState mesh_sampler : register(s0);
SamplerState shadowmap_sampler : register(s1);

// Mesh Data
Texture2D mesh_texture : register(t0);

// Illumination (Global + Local)
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

Texture2D shadow_atlas : register(t1);

cbuffer CB1 : register(b1)
{
    float3 view_world_position;
    int light_count;
    
    LightData light_instances[10];
    
}

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

// Lighting: https://lavalle.pl/vr/node197.html

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 1);
       
    // Ambient lighting
    color.rgb += float3(0.1f, 0.1f, 0.1f);
    
    // FORCE USAGE OF MESH TEXTURE?
    float3 mesh_color = input.color;
    
    for (int i = 0; i < light_count; i++)
    {
        // Check if the point is in shadow or not. 
        // Convert the point to the light projection space, to see how far it is
        // from the light.
        float4 view_position = float4(input.world_position, 1);
        view_position = mul(light_instances[i].m_view, view_position);
        view_position = mul(light_instances[i].m_projection, view_position);
        view_position = view_position / view_position.w; // Manual W-Divide 
        
        float depth = view_position.z;
    
        if (-1 <= view_position.x && view_position.x <= 1)
        {
            if (-1 <= view_position.y && view_position.y <= 1)
            {
                float map_x = light_instances[i].tex_x;
                float map_y = light_instances[i].tex_y;
                float map_width = light_instances[i].tex_width;
                float map_height = light_instances[i].tex_height;
                
                // Sample the light shadow map to see how far the light can see at that location. 
                float2 shadowmap_coords = float2((view_position.x + 1) / 2.f, (-view_position.y + 1) / 2.f);
                shadowmap_coords = float2(shadowmap_coords.x * map_width + map_x, shadowmap_coords.y * map_height + map_y);
                
                float sampledDepth = shadow_atlas.Sample(shadowmap_sampler, shadowmap_coords).r;
                
                // If sampled depth is < depth, the light cannot see the point, so it provides
                // no contribution to the color at that point (point is in shadow). 
                float inShadow = step(depth, sampledDepth + 0.01f); // Small offset 0.01 to avoid precision errors
    
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

                // if (i != 0)
                //    attenuationContribution = 1 / (1 + 0.1 * light_distance);
                
                float3 light_color = light_instances[i].color;
                
                float3 combined_color = (mesh_color * light_color);
                float totalContribution = inShadow * attenuationContribution * (diffuseContribution + specularContribution);
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
