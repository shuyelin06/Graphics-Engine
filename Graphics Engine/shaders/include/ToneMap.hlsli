// Tone Mapping: https://64.github.io/tonemapping/

// Luminance:
// A function that computes luminance, a scalar measuring how bright an RGB
// color is (based on human perception).
// For a standard RGB value between [0,1], this gives a value in the range [0,1].
float luminance(float3 rgb) {
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

// ReinhardExtended:
// Given luminance, maps it so that it always remains between [0,1]. We
// use this to determine how much to scale our RGB colors by.
// Max Radiance specifies the upper asymptotic bound the luminance cannot
// exceed.
float reinhard_extended(float luminance, float max_radiance)
{
    float numerator = 1 + luminance / pow(max_radiance, 2);
    float denominator = 1 + luminance;

    return luminance * numerator / denominator;
}

// ToneMap:
// Given a color, uses Reinhard's Extended TMO function
// to tone map the color from [0, infty) to [0, 1] on each
// RGB channel
float4 tone_map(float4 color, float max_radiance)
{
    // Calculate the current luminance
    float curLuminance = luminance(color.rgb);
    
    // Calculate new luminance
    float newLuminance = reinhard_extended(curLuminance, max_radiance);
    
    // Scale the color so all channels remain within valid input ranges
    return color * newLuminance / curLuminance;    
}
