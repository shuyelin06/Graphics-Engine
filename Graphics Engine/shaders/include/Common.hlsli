#pragma once

#define DefineTex2D(name, reg) \
    Texture2D name : register(t##reg); \
    SamplerState name##_sampler : register(s##reg);
#define SampleTex2D(name, uv) \
    name.Sample(name##_sampler, uv)