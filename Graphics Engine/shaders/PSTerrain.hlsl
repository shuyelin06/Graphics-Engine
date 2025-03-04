#include "ToneMap.hlsli"

// Mesh Data
Texture2D terrain_texture : register(t0);
SamplerState mesh_sampler : register(s0);

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
    
    // Compute vectors for lighting that can be reused
    float3 normal = normalize(input.normal);
    
    // --- Sun Light Contribution ---
    // Compute the contribution of the sun to the point.
    // Select the cascade that our point is in.
    LightData light = sun_cascades[selectCascade(input.world_position)];
    
    // Then, select the shadow value of the point
    float shadow_factor = shadowValue(input.world_position, light, 0.01f);
                   
    // Compute the diffuse contribution. Flip the sun direction since it points from 
    // the light -> surface.
    float diffuse_factor = max(0, dot(-sun_direction, normal));
    
    float cascade_contribution = shadow_factor * diffuse_factor;
    color.rgb += cascade_contribution * (mesh_color * light.color);
    
    // --- TODO ---
    // ...
    
    // Tone Mapping (ToneMap.hlsli):
    // Each color channel can have an intensity in the range [0, infty). We need to "tone map" it,
    // by passing it into a function that will map this to the [0, 255] RGB range.
    // color = tone_map(color, light_count);
    
    return color;
}
