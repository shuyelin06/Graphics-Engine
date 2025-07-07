#include "P_Common.hlsli"
#include "ToneMap.hlsli"

// Lighting:
// Illumination (Global + Local)
#include "Lighting.hlsli"

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
    float3 mesh_color = float3(0.823f, 0.705f, 0.549f);
    
    // Ambient Lighting
    float ambient = 0.35f;
    color.rgb += mesh_color * ambient;
    
    // Compute vectors for lighting that can be reused
    float3 normal = normalize(input.normal);
    
    // --- Sun Light Contribution ---
    // Select the cascade that our point is in.
    LightData sun_light = sun_cascades[selectCascade(input.world_position)];
    // Then, select the shadow value of the point
    // float shadow_factor = shadowValue(input.world_position, sun_light, 0.01f);
    float shadow_factor = 1.f;
    
    // Diffuse lighting (flip the sun direction since it points from 
    // the light -> surface).
    float diffuse_factor = max(0, dot(-sun_direction, normal));
    color.rgb += (shadow_factor * diffuse_factor) * (mesh_color * sun_light.color);
    
    // --- Light Contributions ---
    for (int i = 0; i < light_count - 3; i++)
    {
        LightData light = light_instances[i];
        
        shadow_factor = shadowValue(input.world_position, light, 0.01f);
        
        float3 light_direc = normalize(light.position - input.world_position);
        diffuse_factor = max(0, dot(light_direc, normal));
        
        color.rgb += (shadow_factor * diffuse_factor) * (mesh_color * light.color);
    }

    return color;
}
