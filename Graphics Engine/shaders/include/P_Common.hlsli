#ifndef __P_COMMON_HEADER__
#define __P_COMMON_HEADER__
// P_Common.hlsli:
// Contains common bindings which are to be used
// across all pixel shaders.
// If a binding is not given here, it is assumed to be a slot
// that can be used by any shader for any other purpose.
SamplerState s_point : register(s0);
SamplerState s_shadow : register(s1);

Texture2D color_atlas : register(t0);
Texture2D shadow_atlas : register(t1);

#endif