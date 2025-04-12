// Basic Post-Processing Vertex Shader.
struct VS_IN
{
    float4 position : SV_POSITION;
};

struct PS_IN
{
    float4 position_clip : SV_POSITION;
};

PS_IN vs_main(VS_IN input)
{
    PS_IN output;
    output.position_clip = input.position;
    return output;
}
