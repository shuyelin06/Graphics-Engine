#include "PerlinNoise.h"

#include <cstdlib>

#include "Compute.h"

namespace Engine {
namespace Math {

// Fade Function:
// 6t^5 - 15t^4 + 10t^3
// Given a number from [0,1], smooths the input curve
// so things look smoother and aren't as jagged.
static float fade(float t) { return (t * t * t) * (10 + t * (6 * t - 15)); }

// Gradient Function:
// Given a hash value, and a (x,y) coordinate, returns the result of the
// gradient vector dotted with the direction vector (from (x,y) to the corner
// where that gradient vector is).
static float grad2D(int hash, float x, float y) {
    constexpr float sqrt2 = 1.41421356f;

    // The last 2 bits of the hash function determine what gradient vector to
    // use. Gradient vectors are the vectors from the center of the square, to
    // the edges. (1,0), (0,1), (-1,0), (0,-1) We automatically dot this with
    // the vector (x,y), which represents the vector from some corner to (x,y).
    switch (hash & 0x7) {
    case 0x0: // (1,0)
        return x;
    case 0x1: // (0,1)
        return y;
    case 0x2: // (-1,0)
        return -x;
    case 0x3: // (0,-1)
        return -y;
    case 0x4: // (-1/sqrt(2), -1/sqrt(2))
        return (-x - y) / sqrt2;
    case 0x5: // (-1/sqrt(2), 1/sqrt(2))
        return (-x + y) / sqrt2;
    case 0x6: // (1/sqrt(2), 1/sqrt(2))
        return (x + y) / sqrt2;
    case 0x7: // (1/sqrt(2), -1/sqrt(2))
        return (-x - y) / sqrt2;
    default:
        return 0;
    }
}

static float grad3D(int hash, float x, float y, float z) {
    // The last 4 bits of the hash function determine what gradient vector to
    // use. This is pseudorandomly chosen from the following list:
    // (1,1,0), (-1,1,0), (1,-1,0), (-1,-1,0),
    // (1, 0, 1), (-1, 0, 1), (1, 0, -1), (-1, 0, -1), (0, 1, 1), (0, -1, 1),
    // (0, 1, -1), (0, -1, -1)
    switch (hash & 0xF) {
    case 0x0:
        return x + y;
    case 0x1:
        return -x + y;
    case 0x2:
        return x - y;
    case 0x3:
        return -x - y;
    case 0x4:
        return x + z;
    case 0x5:
        return -x + z;
    case 0x6:
        return x - z;
    case 0x7:
        return -x - z;
    case 0x8:
        return y + z;
    case 0x9:
        return -y + z;
    case 0xA:
        return y - z;
    case 0xB:
        return -y - z;
    case 0xC:
        return y + x;
    case 0xD:
        return -y + z;
    case 0xE:
        return y - x;
    case 0xF:
        return -y - z;
    default:
        return 0; // never happens
    }
}

// OctaveNoise2D;
// Returns perlin noise, combined by amplitude to greate larger patterns.
float PerlinNoise::octaveNoise2D(float x, float y, int octaves,
                                 float persistence) const {
    // We will sample perlin noise along multiple octaves, where along each
    // we will increas the frequency and amplitude by a factor given by the
    // persistence
    float total = 0;
    // MaxValue is used to normalize the result to [0,1]
    float maxValue = 0;

    float frequency = 1;
    float amplitude = 1;

    for (int i = 0; i < octaves; i++) {
        total += noise2D(x * frequency, y * frequency) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2;
    }

    return total / maxValue;
}

PerlinNoise::PerlinNoise(unsigned int seed) { seedGenerator(seed); }

// SeedGenerator:
// Generates the permutation table for the generator using the Fisher-Yates
// algorithm for generating random permutations.
void PerlinNoise::seedGenerator(unsigned int seed) {
    // Seed my random number generator
    srand(seed);

    // Initialize my permutation table with entries 0, 1, ... 255.
    for (int i = 0; i < 256; i++) {
        permutation_table[i] = i;
    }

    // Randomly choose pairs (i,j), where i <= j <= n-1,
    // and swap the values.
    for (int i = 0; i < 254; i++) {
        const int j = i + Random(0, 255 - i);

        const unsigned int temp = permutation_table[i];
        permutation_table[i] = permutation_table[j];
        permutation_table[j] = temp;
    }
}

// IndexTable:
// Given an index, indexes the permutation table. Applies modulus if needed.
unsigned char PerlinNoise::indexTable(int index) const {
    return permutation_table[index % 256];
}

// SampleNoise2D:
// Samples the perlin noise given x,y coordinates.
// Multiply x,y with a "frequency" in [0,1] to sample the noise at larger or
// smaller intervals. Frequencies between [0, 0.3] yield good results.
float PerlinNoise::noise2D(float x, float y) const {
    // If x or y is negative, wrap it to the positive numbers so our perlin
    // noise properly wraps and repeats.
    if (x < 0)
        x = ((int)(-x / 256) + 1) * 256 + x;
    if (y < 0)
        y = ((int)(-y / 256) + 1) * 256 + y;

    // Cell index in the grid (centered at (0,0)). We use this, with our
    // permutation table, to generate a pseudonumber to determine our gradient
    // vectors.
    const int xi = ((int)x) & 0xFF;
    const int yi = ((int)y) & 0xFF;

    // Coordinates within our cell, faded for a smoother input.
    // This represents a coordinate within
    // our cell which we want to find the Perlin Noise for.
    const float xf = Clamp(fade(x - (int)x), 0, 1);
    const float yf = Clamp(fade(y - (int)y), 0, 1);

    // For my coordinates, randomly choose a number from [0, 255] using the
    // permutation table. This hash will determine the gradient vector
    // chosen at that coordinate
    // aa represents the bottom-left vertex in the square (0,0), bb represents
    // (1,1).
    const int aa = indexTable(indexTable(xi) + yi);
    const int ab = indexTable(indexTable(xi) + yi + 1);
    const int ba = indexTable(indexTable(xi + 1) + yi);
    const int bb = indexTable(indexTable(xi + 1) + yi + 1);

    // The hash at each vertex determines the gradient vector chosen. We dot the
    // gradient with the vector from that vertex to (x,y), and linearly
    // interpolate.
    const float grad_aa = grad2D(aa, xf, yf);
    const float grad_ab = grad2D(ab, xf, yf - 1);
    const float grad_ba = grad2D(ba, xf - 1, yf);
    const float grad_bb = grad2D(bb, xf - 1, yf - 1);

    const float perlin_value =
        Lerp(Lerp(grad_aa, grad_ab, yf), Lerp(grad_ba, grad_bb, yf), xf);
    const float normalized_value = (perlin_value + 1) / 2.f;

    return normalized_value;
}

// SampleNoise3D:
// Samples the perlin noise given x,y,z coordinates.
// Generalizes the 2D case for 3D coordinates.
float PerlinNoise::noise3D(float x, float y, float z) const {
    // If x,y,z are negative, wrap it to the positive numbers so our perlin
    // noise properly wraps and repeats.
    if (x < 0)
        x = ((int)(-x / 256) + 1) * 256 + x;
    if (y < 0)
        y = ((int)(-y / 256) + 1) * 256 + y;
    if (z < 0)
        z = ((int)(-z / 256) + 1) * 256 + z;

    // Cell index in the grid (centered at (0,0)). We use this, with our
    // permutation table, to generate a pseudonumber to determine our gradient
    // vectors.
    const int xi = ((int)x) & 0xFF;
    const int yi = ((int)y) & 0xFF;
    const int zi = ((int)z) & 0xFF;

    // Coordinates within our cell, faded for a smoother input.
    // This represents a coordinate within
    // our cell which we want to find the Perlin Noise for.
    const float xf = Clamp(fade(x - (int)x), 0, 1);
    const float yf = Clamp(fade(y - (int)y), 0, 1);
    const float zf = Clamp(fade(z - (int)z), 0, 1);

    // For my coordinates, randomly choose a number from [0, 255] using the
    // permutation table. This hash will determine the gradient vector
    // chosen at that coordinate
    // aaa represents the bottom-left vertex in the square (0,0,0), bbb
    // represents (1,1,1).
    const int aaa = indexTable(indexTable(indexTable(xi) + yi) + zi);
    const int aab = indexTable(indexTable(indexTable(xi) + yi) + zi + 1);
    const int aba = indexTable(indexTable(indexTable(xi) + yi + 1) + zi);
    const int abb = indexTable(indexTable(indexTable(xi) + yi + 1) + zi + 1);

    const int baa = indexTable(indexTable(indexTable(xi + 1) + yi) + zi);
    const int bab = indexTable(indexTable(indexTable(xi + 1) + yi) + zi + 1);
    const int bba = indexTable(indexTable(indexTable(xi + 1) + yi + 1) + zi);
    const int bbb =
        indexTable(indexTable(indexTable(xi + 1) + yi + 1) + zi + 1);

    // The hash at each vertex determines the gradient vector chosen. We dot the
    // gradient with the vector from that vertex to (x,y), and linearly
    // interpolate.
    const float grad_aaa = grad3D(aaa, xf, yf, zf);
    const float grad_aab = grad3D(aab, xf, yf, zf - 1);
    const float grad_aba = grad3D(aba, xf, yf - 1, zf);
    const float grad_abb = grad3D(abb, xf, yf - 1, zf - 1);

    const float grad_baa = grad3D(baa, xf - 1, yf, zf);
    const float grad_bab = grad3D(bab, xf - 1, yf, zf - 1);
    const float grad_bba = grad3D(bba, xf - 1, yf - 1, zf);
    const float grad_bbb = grad3D(bbb, xf - 1, yf - 1, zf - 1);

    float x1, x2;

    x1 = Lerp(grad_aaa, grad_baa, xf);
    x2 = Lerp(grad_aba, grad_bba, xf);
    const float y1 = Lerp(x1, x2, yf);

    x1 = Lerp(grad_aab, grad_bab, xf);
    x2 = Lerp(grad_abb, grad_bbb, xf);
    const float y2 = Lerp(x1, x2, yf);

    return (Lerp(y1, y2, zf) + 1) / 2;
}

} // namespace Math
} // namespace Engine