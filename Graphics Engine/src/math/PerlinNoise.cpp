#include "PerlinNoise.h"

namespace Engine {
namespace Math {

// Permutation Hash Table as defined by Ken Perlin.
// We use this to hash our values to generate "pseudo-random" numbers.
// This is a 512 byte array (256 array repeated twice) that has the values 0 -
// 255 in a random permutation order
unsigned char permutations[] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
    225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
    6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
    35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
    171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
    231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
    245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
    58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
    154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
    19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
    84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
    180, 151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233,
    7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,
    190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
    117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125,
    136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146,
    158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,
    46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,
    209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159,
    86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123,
    5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,
    16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,
    44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,
    253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246,
    97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,
    51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
    184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205,
    93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,
    156, 180};

// Fade Function:
// 6t^5 - 15t^4 + 10t^3
// Given a number from [0,1], smooths the input curve
// so things look smoother and aren't as jagged.
static float fade(float t) { return (t * t * t) * (10 + t * (6 * t - 15)); }

// Linear Interpolation Function:
// Given two numbers and a number from [0,1], linearly
// interpolates between the two numbers
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

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

// OctaveNoise2D;
// Returns perlin noise, combined by amplitude to greate larger patterns.
float PerlinNoise::octaveNoise2D(float x, float y, int octaves,
                                 float persistence) {
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

// Noise2D:
// Returns perlin noise given some x and y coordinate
float PerlinNoise::noise2D(float x, float y) {
    // Cell index in the grid (centered at (0,0)). We use this, with our
    // permutation table, to generate a pseudonumber to determine our gradient
    // vectors.
    const int xi = ((int)x) & 0xFF;
    const int yi = ((int)y) & 0xFF;

    // Coordinates within our cell, faded for a smoother input.
    // This represents a coordinate within
    // our cell which we want to find the Perlin Noise for.
    const float xf = fade(x - (int)x);
    const float yf = fade(y - (int)y);

    // For my coordinates, create a "hash" for each vertex using the permutation
    // table. This hash will determine my gradient vectors. aa represents the
    // bottom-left vertex in the square (0,0), bb represents (1,1).
    const int aa = permutations[permutations[xi] + yi];
    const int ab = permutations[permutations[xi] + yi + 1];
    const int ba = permutations[permutations[xi + 1] + yi];
    const int bb = permutations[permutations[xi + 1] + yi + 1];

    // The hash at each vertex determines the gradient. We dot the gradient with
    // the vector from that vertex to (x,y), and linearly interpolate.
    const float grad_aa = grad2D(aa, xf, yf);
    const float grad_ab = grad2D(ab, xf, yf - 1);
    const float grad_ba = grad2D(ba, xf - 1, yf);
    const float grad_bb = grad2D(bb, xf - 1, yf - 1);

    const float perlin_value =
        lerp(lerp(grad_aa, grad_ab, yf), lerp(grad_ba, grad_bb, yf), xf);

    // Normalize to be between (0,1)
    return (perlin_value + 1) / 2.f;
}

} // namespace Math
} // namespace Engine