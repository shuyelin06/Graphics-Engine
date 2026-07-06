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
};

DefineTex2D(TerrainColormap, 4);

cbuffer TerrainData : register(b4)
{
    float triplanarTextureScale;
    float triplanarTextureSharpness;
    float noiseScaling;
    float padding;
}

// Simple hash function to get random values based on position
float2 Hash2D(float2 p)
{
    return frac(sin(float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)))) * 43758.5453);
}

float2 fuzzed(float2 uv)
{
    float2 hash = Hash2D(uv);
    return uv + float2(triplanarTextureSharpness * hash.x * cos(hash.y), triplanarTextureSharpness
     * hash.x * sin(hash.y));
}

float3 triplanarSampleColormap(float3 position, float3 normal)
{
    float uvScaling = CB(triplanarTextureScale);
    float uvScaling2 = uvScaling * CB(triplanarTextureSharpness);
    float sharpness = CB(triplanarTextureSharpness);

    
    
    float3 xSample = SampleTex2D(TerrainColormap, uvScaling * position.zy);
    float3 ySample = SampleTex2D(TerrainColormap, uvScaling * position.xz);
    float3 zSample = SampleTex2D(TerrainColormap, uvScaling * position.xy);
    
    /*
    float3 xSample2 = SampleTex2D(TerrainColormap, uvScaling2 * position.zy);
    float3 ySample2 = SampleTex2D(TerrainColormap, uvScaling2 * position.xz);
    float3 zSample2 = SampleTex2D(TerrainColormap, uvScaling2 * position.xy);
    float3 grad;
    float noise = noise3D(noiseScaling * position, float3(0, 0, 0), grad);
    xSample = lerp(xSample, xSample2, noise);
    ySample = lerp(ySample, ySample2, noise);
    zSample = lerp(zSample, zSample2, noise);
    */
    
    float3 weights = abs(normal);
    weights = pow(weights, 1);
    weights = weights / (weights.x + weights.y + weights.z);
    float3 blended = xSample * weights.x + ySample * weights.y + zSample * weights.z;

    return blended;
}

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 psterrain_main(PS_IN input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 direction = normalize(float3(0.5f, -0.5f, 0.5f));

    float3 color = triplanarSampleColormap(input.world_position, normal);
    
    return float4(color, 1.f);
}
