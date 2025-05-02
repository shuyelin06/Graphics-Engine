// PerlinNoise.hlsli
// GPU implementation of the perlin noise algorithm.
// TODO: Have it read from a texture. The texture defines, for XY coordinates
// (normalized to [0,256) repeating), the 4 hash corners.
struct PerlinNoiseData
{
    // Gradient vectors
    float2 gradients[8];
    
    // Assumed to be a permutation of [0,255], duplicated.
    // We duplicate so that lookups are faster (no need to modulus the indexing)
    int permutation_table[512];
};

// Fade Function. Smooths the input.
float2 fade(float2 t)
{
    return (t * t * t) * (10 + t * (6 * t - 15));
}

// GPU Implementation of the 2D Perlin Noise algorithm.
// Based on
// https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-26-implementing-improved-perlin-noise#:~:text=Whereas%20Ken%27s%20chapter%20discussed%20how%20to%20implement%20fast,syntax%20that%20exactly%20matches%20the%20reference%20CPU%20implementation
float perlinNoise2D(float2 xy, PerlinNoiseData data)
{
    // Modulus our coordinates so that they are within [0,255) repeating.
    float2 P = fmod(xy + float2(1000.f, 1000.f), 256.f);
    
    // Pull the integer component
    int2 Pi = (int2) floor(P);
    // Pull our fractional component, and fade it to get a smoother
    // output
    float2 Pf = fade(P - floor(P));
    
    // Now, we use our permutation table and generate a hash for the 4
    // corners of our cube.
    int aa = data.permutation_table[data.permutation_table[Pi.x] + Pi.y];
    int ab = data.permutation_table[data.permutation_table[Pi.x] + Pi.y + 1];
    int ba = data.permutation_table[data.permutation_table[Pi.x + 1] + Pi.y];
    int bb = data.permutation_table[data.permutation_table[Pi.x + 1] + Pi.y + 1];
    
    // We use this hash functions to get our gradient vectors
    float grad_aa = dot(Pf, data.gradients[aa & 0xF]);
    float grad_ab = dot(Pf - float2(0, 1.f), data.gradients[ab & 0xF]);
    float grad_ba = dot(Pf - float2(1.f, 0), data.gradients[ba & 0xF]);
    float grad_bb = dot(Pf - float2(1.f, 1.f), data.gradients[bb & 0xF]);
    
    // Linearly interpolate between these gradient values to
    // get our noise value.
    float perlin_noise = lerp(lerp(grad_aa, grad_ab, Pf.y), lerp(grad_ba, grad_bb, Pf.y), Pf.x);
    // Normalize
    perlin_noise = (perlin_noise + 1) / 2.f;
    
    return perlin_noise;
}