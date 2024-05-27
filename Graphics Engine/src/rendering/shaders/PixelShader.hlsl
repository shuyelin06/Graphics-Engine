struct LightData
{
    float3 position;
    float padding;
};

cbuffer LIGHTS : register(b0)
{
    LightData lights[10];
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
    
    float4 color = float4(0, 0, 0, 1.0);
    
    for (int i = 0; i < 5; i++)
    {
        float3 light_pos = lights[i].position;
        
        float3 pos = input.world_position;
    
        // Direction from light to position
        float3 light_direction = light_pos - pos;
        
        float angle = dot(normalize(light_direction), input.normal);
        
        // Add light value
        color.x += 10 * angle / (length(light_direction) + 1);
    }
    
    return color;
}
