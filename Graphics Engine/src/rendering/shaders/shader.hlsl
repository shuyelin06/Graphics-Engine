cbuffer TRANSFORM_MATRICES : register(b0)
{
    row_major float4x4 m_modelToWorld;
    row_major float4x4 m_worldToCamera;
    row_major float4x4 m_normalTransform;
}

struct LightData
{
    float3 position;
    float padding;
};

cbuffer LIGHTS : register(b1)
{
    LightData lights[10];
}

/* Vertex Shader Input */
struct VS_IN {
	float3 position_local : POSITION; // POSITION Semantic
    float3 normal : NORMAL;
};

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT {
	float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

// Vertex Shader Entry Point - Takes VS_IN and outputs a VS_OUT
// Takes a position in local coordinates, and translates them to clipping
// coordinates
// 
// Vertex Shaders MUST output a float4 XYZW setting a vertex's position in homogeneous clip space
// Must be between -1 and 1 on X and Y axes, and 0 and 1 on Z (positive into the screen). 
// Homogeneous coordiantes are identified by the SV_POSITION semantic
VS_OUT vs_main(VS_IN input) {
	// Zero the Memory
	VS_OUT output = (VS_OUT) 0;
    
    float4 pos = float4(input.position_local, 1.0f);
    float4 norm = float4(input.normal, 1.0f);
	
    // Find World Position
    pos = mul(pos, m_modelToWorld);
    output.world_position = pos.xyz;
    
    // Find Clipping Position
    pos = mul(pos, m_worldToCamera);
    output.position_clip = pos;
    
    // Find normal
    norm = mul(norm, m_normalTransform);
    output.normal = norm.xyz;
	
	return output;
}

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET {
    
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
