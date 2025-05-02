struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

SamplerState tex_sampler : register(s0);
Texture2D render_target : register(t0);

cbuffer CB0_RESOLUTION : register(b0)
{
    float resolution_x;
    float resolution_y;
    
    float z_near;
    float z_far;
}

float4 ps_main(PS_IN input) : SV_TARGET
{
    // Read from the render target texture and write what we read.
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    float3 color = render_target.Sample(tex_sampler, uv);
    
    // Add a very small dither to reduce the effects of banding
    float dither = frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
    color += dither * (1.0f / 255.f);
    
    return float4(color, 1.f);
}
