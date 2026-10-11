#include "P_Common.hlsli"
#include "ToneMap.hlsli"

#include "Noise.hlsli"

// Lighting:
// Illumination (Global + Local)
#include "Lighting.hlsli"

/* Vertex Shader Output (Pixel Shader Input) */
struct PS_IN
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    
    uint4 materialID : TEXCOORD0;
    float4 materialWeights : TEXCOORD1;
};

DefineTex2DArray(TerrainColormap, 4);

cbuffer TerrainData : register(b4)
{
    float triplanarTextureScale;
    float triplanarTextureSharpness;
    float noiseScaling;
    float padding;
}

// Good reference: https://iquilezles.org/articles/texturerepetition/
float3 sampleTextureArrayDetiled(float3 uvw, float mip)
{
    // Determine the variation pattern of the texture
    // - i tells us the variation index we want
    // - f tells us how much to interpolate between the two variations we're between
    float3 outGrad;
    float variations = 4.f;
    float noiseScaled = noise3D(uvw * 0.1f, float3(3, 2, 12), outGrad) * variations;
    
    float f = frac(noiseScaled);
    float i = floor(noiseScaled);
    
    float2 offsetA = sin(float2(3.0f, 7.0f) * (i + 0.0f));
    float2 offsetB = sin(float2(3.0f, 7.0f) * (i + 1.0f));
    
    float3 sampleA = SampleTex2DArrayLevel(TerrainColormap, uvw + float3(offsetA, 0.f), mip, float2(0,0));
    float3 sampleB = SampleTex2DArrayLevel(TerrainColormap, uvw + float3(offsetB, 0.f), mip, float2(0,0));
    
    float3 output = lerp(sampleA, sampleB, smoothstep(0.2, 0.8, f - 0.1 * dot(sampleA - sampleB, float3(1, 1, 1))));;
    
    return output;
}

float4 psterrain_main(PS_IN input) : SV_TARGET
{
    float3 position = input.world_position;
    // UV is taken from by projecting the XZ plane and scaling it
    float2 uv = position.xz * CB(triplanarTextureScale);
    float3 color = SampleTex2DArray(TerrainColormap, float3(uv, 0));
    return float4(color, 1.f);
}
