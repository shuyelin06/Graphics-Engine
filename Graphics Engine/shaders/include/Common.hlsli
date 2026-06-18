#pragma once

// No-op that helps with readability
#define CB(var) var

#define DefineTex2D(name, reg) \
    Texture2D name : register(t##reg); \
    SamplerState name##_sampler : register(s##reg);

// Pixel Shader only. Computes mip automatically
#define SampleTex2D(name, uv) \
    name.Sample(name##_sampler, uv)
// Any Shader
#define SampleTex2DLevel(name, uv, mip, offset) \
    name.SampleLevel(name##_sampler, uv, mip, offset)