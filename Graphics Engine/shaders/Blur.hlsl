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
    
    
    float padding;
    float padding_2;
}

float4 ps_blur(PS_IN input) : SV_TARGET
{
    float2 uv = float2(input.position_clip.x / resolution_x, input.position_clip.y / resolution_y);
    
    float4 sampled_color = render_target.Sample(render_sampler, uv);
    
    // return float4(float3(1, 1, 1) - sampled_color.xyz, 1.f);
    return sampled_color;
}