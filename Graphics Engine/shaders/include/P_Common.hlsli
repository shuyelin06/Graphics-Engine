#include "Common.hlsli"

#ifndef __P_COMMON_HEADER__
#define __P_COMMON_HEADER__
// P_Common.hlsli:
// Contains common bindings which are to be used
// across all pixel shaders.
// If a binding is not given here, it is assumed to be a slot
// that can be used by any shader for any other purpose.
DefineTex2D(color_atlas, 0);
DefineTex2D(shadow_atlas, 1);

cbuffer CB0_GLOBAL_DATA : register(b0)
{
    // View Information
    float3 view_pos;
    float view_znear;
    
    float3 view_direction;
    float view_zfar;
    
    float4x4 m_world_to_screen;
    float4x4 m_screen_to_world;
    
    // Render Target Information
    float resolution_x;
    float resolution_y;
    float2 cb0_p0; // Padding
    
    // Rendering Config
    
    
}

// --- Utility Functions ---
// Given (u,v) derivatives and a texture they are sampling from, returns the ideal
// mip.
float computeMipFromDerivatives(Texture2D tex, float2 dx, float2 dy)
{
    // Get the texture information. This will tell us
    // 1) The dimensions of the texture so we can convert (u,v) derivatives
    //    to the change in texels
    // 2) The number of mips so we know what to clamp the output to
    uint textureWidth;
    uint textureHeight;
    uint textureMips;
    tex.GetDimensions(0, textureWidth, textureHeight, textureMips);

    // Convert UV coordinates to texture texels; change in texture texels.
    // The greatest texel change in either u or v direction tells us how many mips we need,
    // since the greater the change then the more area in the original texture we are covering
    float2 texeldx = float(textureWidth) * dx;
    float2 texeldy = float(textureHeight) * dy;
    float maxTexelChange = max(length(texeldx), length(texeldy));
    
    // Because every mip is a factor of 2 smaller than the previous, we compute the ideal
    // mip by taking the log of our max texel change. Clamp to the number of available mips
    // in the texture
    return clamp(log2(maxTexelChange), 0.f, float(textureMips));
}

#endif