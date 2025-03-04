struct VS_IN
{
    float4 position : SV_POSITION;
};

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

PS_IN vs_blur(VS_IN input)
{
    PS_IN output;
    output.position_clip = input.position;
    return output;
}

Texture2D render_target : register(t0);
SamplerState render_sampler : register(s0);

cbuffer CB0_RESOLUTION : register(b0)
{
    float resolution_x;
    float resolution_y;
    
    float kernel;
    float padding;
}

float4 ps_blur(PS_IN input) : SV_TARGET
{
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    float2 offsets = float2(1.f / resolution_x, 1.f / resolution_y);
    
    float3 final_color = float3(0, 0, 0);
    for (int i = 0; i < kernel; i++)
    {
        float4 sampled_color = render_target.Sample(render_sampler, uv + float2(offsets.x * (i - kernel / 2), 0.f));
        final_color = final_color + sampled_color.rgb;
    }
    final_color = final_color / kernel;
    
    // return float4(float3(1, 1, 1) - sampled_color.xyz, 1.f);
    return float4(final_color, 1.f);
}
