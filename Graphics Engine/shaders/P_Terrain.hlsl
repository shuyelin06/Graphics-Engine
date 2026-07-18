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

float3 triplanarSampleColormap(float3 position, float3 normal)
{
    float uvScaling = CB(triplanarTextureScale);
    float uvScaling2 = uvScaling * CB(triplanarTextureSharpness);
    float sharpness = CB(triplanarTextureSharpness);

    float3 positionDX = ddx(position);
    float3 positionDY = ddy(position);
    
    float2 xUVdx = uvScaling * positionDX.zy;
    float2 xUVdy = uvScaling * positionDY.zy;

    float2 yUVdx = uvScaling * positionDX.xz;
    float2 yUVdy = uvScaling * positionDY.xz;

    float2 zUVdx = uvScaling * positionDX.xy;
    float2 zUVdy = uvScaling * positionDY.xy;

    
    // TODO: Manual computation of UV derivatives for correct mips
    float2 xUV = uvScaling * position.zy;
    float2 yUV = uvScaling * position.xz;
    float2 zUV = uvScaling * position.xy;

    float3 xSample = SampleTex2DLevel(TerrainColormap, xUV, computeMipFromDerivatives(TerrainColormap, xUVdx, xUVdy), float2(0,0));
    float3 ySample = SampleTex2DLevel(TerrainColormap, yUV, computeMipFromDerivatives(TerrainColormap, yUVdx, yUVdy), float2(0,0));
    float3 zSample = SampleTex2DLevel(TerrainColormap, zUV, computeMipFromDerivatives(TerrainColormap, zUVdx, zUVdy), float2(0,0));
    
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
