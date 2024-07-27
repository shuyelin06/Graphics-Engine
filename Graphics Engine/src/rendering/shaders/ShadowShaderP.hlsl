// Light Bindings
struct LightData
{
    float3 position;
    float padding1;
    
    float3 color;
    float padding2;
    
    row_major float4x4 m_view;
    row_major float4x4 m_projection;
};

Texture2D lightDepthMaps[10] : register(t0);
SamplerState lightDepthSamplers[10] : register(s0);

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
};

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 1.0f);
    
    for (int i = 0; i < light_count; i++)
    {
        // Check if the point is shadowed. First, get the depth of the
        // point to the light by converting it to the light projection
        // space
        float4 view_position = float4(input.world_position, 1);
        view_position = mul(view_position, light_instances[i].m_view);
        view_position = mul(view_position, light_instances[i].m_projection);
        view_position = view_position / view_position.w; // Manual W-Divide 
        float depth = view_position.z;
    
        // USES BRANCHES.. avoid somehow?
        if (-1 <= view_position.x && view_position.x <= 1)
        {
            if (-1 <= view_position.y && view_position.y <= 1)
            {
                // Now, sample the light's shadow map to see its depth at that location, i.e.
                // how "far" it can see.
                float2 shadowmap_coords = float2((view_position.x + 1) / 2.f, (-view_position.y + 1) / 2.f);
                float sampledDepth = lightDepthMaps[i].Sample(lightDepthSamplers[i], shadowmap_coords).r;
    
                // If sampled depth is < depth, that means the light cannot see the
                // point, so its in shadow.
                // To represent this,
                float inShadow = step(depth, sampledDepth + 0.01f);
    
                // Let's now perform our lighting calculations! 
                // To do this, we need the find the following:
                // 1) Surface normal
                // 2) Light direction vector & light distance
                // 3) View direction vector
                // 4) Reflected light direction vector (across normal)
                float3 normal = normalize(input.normal);
    
                // Light --> Surface
                float3 light_direction = input.world_position - light_instances[i].position;
                float light_distance = length(light_direction);
                light_direction = light_direction / light_distance;
    
                // Surface --> View 
                float3 view_direction = view_world_position - input.world_position;
                view_direction = normalize(view_direction);
                
                float3 light_direction_reflected = light_direction - 2 * dot(light_direction, normal) * normal;
                light_direction_reflected = normalize(light_direction_reflected);
                
                // --- Diffuse Contribution ---
                // Compare how close the normal is to the direction to the light.
                // Flip the light direction since it points from light --> surface
                float diffuseContribution = max(0, dot(-light_direction, normal));
                // Apply a contribution from the material (0 -> 1, 1 meaning perfect reflection; 0.5: kinda okay)
                diffuseContribution *= 0.35f;
                
                // --- Specular Contribution ---
                // Compare how close the view direction is to the reflected light direction.
                float specularContribution = max(0, dot(view_direction, light_direction_reflected));
                // Apply a shininess contribution from the material (0 -> infty, 50+ is pretty shiny)
                specularContribution *= pow(specularContribution, 50);
                
                color.x += float4(inShadow * (diffuseContribution + specularContribution), 0, 0, 0);
            }
        }
    }
    
    return color;
}
