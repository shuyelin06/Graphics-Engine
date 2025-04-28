cbuffer CB0_TRANSFORM_INFO : register(b0)
{
    float4x4 m_world_to_screen;
    
    // Determines (x,z) offset
    float3 camera_position;
    // Determines (y) offset
    float surface_height;
}

// CB1:
// Stores information to be used in rendering the waves.
// One wave is composed of multiple sine waves, which have a distinct
// amplitude and period. Together, these sine waves (called the sum of sines)
// form unique looking waves.
struct WaveConfig
{
    int dimension; // 0 for x, 1 for z
    float period;
    float amplitude;
    float timing;
};
cbuffer CB1_WAVE_INFO : register(b1)
{
    float time;
    int num_waves;
    float2 padding;
    
    WaveConfig waves[100];
}

struct VS_INPUT
{
    // Position of the water surface
    float3 surface_point : POSITION;
};

struct PS_INPUT
{
    float4 position_clip : SV_Position;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // Transform the point so that the water surface is centered
    // around the camera
    float x = input.surface_point.x + camera_position.x;
    float z = input.surface_point.z + camera_position.z;
    
    // Sample our function to determine the water surface height.
    // We will also sample the function's x,z derivatives, so we can
    // calculate the wave's normal vector.
    float wave_sample = 0.f;
    float dx_sample = 0.f;
    float dz_sample = 0.f;
    
    for (int i = 0; i < num_waves; i++)
    {
        // Wave Config is for the X Dimension
        if (waves[i].dimension == 0)
        {
            float domain = waves[i].period * x + waves[i].timing * time;
            wave_sample += waves[i].amplitude * sin(domain);
            dx_sample += waves[i].amplitude * waves[i].period * cos(domain);
        }
        // Wave Config is for the Z Dimension
        else
        {
            float domain = waves[i].period * z + waves[i].timing + time;
            wave_sample += waves[i].amplitude * sin(domain);
            dz_sample += waves[i].amplitude * waves[i].period * cos(domain);
        }
    }
    float y = surface_height + wave_sample;
    
    // Translate this point to clip space
    output.world_position = float3(x, y, z);
    float4 clip_point = float4(x, y, z, 1.f);
    clip_point = mul(m_world_to_screen, clip_point);
    output.position_clip = clip_point;
    
    // Calculate the normal from the previously calculated dx and dz.
    float3 normal = cross(float3(0.0f, dz_sample, 1.0f), float3(1.0f, dx_sample, 0.0f));
    normal = normalize(normal);
    output.normal = normal;
    
    return output;
}