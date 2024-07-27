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
    int light_count;
    float3 padding;
    
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
    
                // Let's now perform our lighting calculation! First, normalize our normal
                // and find our direction to the light (as well as distance to the light)
                float3 normal = normalize(input.normal);
    
                float3 light_direction = light_instances[i].position - input.world_position;
                float light_distance = length(light_direction);
                light_direction = light_direction / light_distance;
    
                // Calculate diffuse contribution. We do this by comparing how close the normal
                // is to the direction to the light
                float diffuseContribution = max(0, dot(light_direction, normal));
    
                // Calculate specular contribution. We do this by comparing how close the view is
                // to the "reflected" light direction across the normal.
                // ...
    
                color.x += float4(inShadow * diffuseContribution, 0, 0, 0);
            }
        }
    }
    
    return color;
}
