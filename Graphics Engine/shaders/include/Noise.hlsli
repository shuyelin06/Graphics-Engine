// --- Hashing / Pseudorandom Generators ---
// Ad-Hoc Float Hash by Dave Hoskins:
float hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// Permutation Polynomials:
// Computes polynomials of the form 2Ax^2 + B mod A^2,
// which generates reasonably random-looking outputs.
// A must be prime-- here, we preset it to the value 17
// (B can be used as a "seed")
// Returns values in the rangs [0, A^2 = 288). Divide by 288.f to clamp to [0,1].
float permute(float x, float b)
{
    float h = fmod(x, 289.f);
    return fmod((34.f * h + b) * h, 289.f);
}

// Gradient Generation:
// Helper functions that let us generate 2D and 3D gradient vectors
float3 gradient3D(float3 p, float3 seed)
{
    float x = permute(permute(permute(p.x, seed.x) + p.y, seed.x) + p.z, seed.x);
    float y = permute(permute(permute(p.x, seed.y) + p.y, seed.y) + p.z, seed.y);
    float z = permute(permute(permute(p.x, seed.z) + p.y, seed.z) + p.z, seed.z);
    // (x,y,z) are in the range [0,288) from permute. Transform to [-1,1]
    // - Divide by 288 / 2 = 144 to go to [0,2]
    // - Subtract by 1
    // Finally, normalize to get unit vectors.
    return normalize(float3(x, y, z) / 144.f - 1.f);
}

// --- Simplex Noise ---
// Procedural shader function for simplex noise.
// https://www.shadertoy.com/view/tfdXRB
// To use, call noise3D.
// This returns a noise value in [0,1], and a gradient vector.
float3 xyz_to_uvw(float3 xyz)
{
    float s = (xyz.x + xyz.y + xyz.z) / 3.0f;
    return xyz + s;
}
float3 uvw_to_xyz(float3 uvw)
{
    float t = (uvw.x + uvw.y + uvw.z) / 6.0f;
    return uvw - t;
}

float noise3D(float3 p, float3 seed, out float3 gradient)
{
    // Convert our point to simplex space.
    // Then, find the corners of our simplex
    float3 uvw = xyz_to_uvw(p);
    
    // This tells us the relative position of our point in the simplex.
    float3 uvw_f = frac(uvw);

    // uvw_0: Is the bottom corner of the simplex
    // uvw_1: uvw_0 + 1 in the largest coordinate
    // uvw_2: uvw_1 + 1 in 1st AND 2nd largest coordinate
    // uvw_3: uvw_2 + 1 in ALL coordinates 
    // Note the step functions. For each coordinate we compare them to the other two. Then:
    // - Min enforces that the comparison must pass for both comparisons to be 1
    // - Max enforces that the comparison must pass for any comparison to be 1
    float3 uvw_0 = floor(uvw);
    float3 uvw_1 = uvw_0 + min(step(uvw_f.yzx, uvw_f.xyz), step(uvw_f.zxy, uvw_f.xyz));
    float3 uvw_2 = uvw_0 + max(step(uvw_f.yzx, uvw_f.xyz), step(uvw_f.zxy, uvw_f.xyz));
    float3 uvw_3 = uvw_0 + float3(1.f, 1.f, 1.f);
    
    // Calculate the gradients at all of my simplex corner points. Also find
    // our directional vectors from each corner to the point
    float3 g0 = gradient3D(uvw_0, seed);
    float3 g1 = gradient3D(uvw_1, seed);
    float3 g2 = gradient3D(uvw_2, seed);
    float3 g3 = gradient3D(uvw_3, seed);
    
    float3 v0 = p - uvw_to_xyz(uvw_0);
    float3 v1 = p - uvw_to_xyz(uvw_1);
    float3 v2 = p - uvw_to_xyz(uvw_2);
    float3 v3 = p - uvw_to_xyz(uvw_3);
    
    // Evaluate the simplex contribution function
    // For each corner: n = w^4 * dot(g, v) where w = max(0.5 - v^2, 0)
    // Gradient is just the derivative of this (apply chain rule)
    float d = 0.5f;
    float w0 = max(d - dot(v0, v0), 0.f);
    float w1 = max(d - dot(v1, v1), 0.f);
    float w2 = max(d - dot(v2, v2), 0.f);
    float w3 = max(d - dot(v3, v3), 0.f);
    
    // Calculate my gradient
    float3 dn0 = pow(w0, 4.) * g0 - 8. * pow(w0, 3.) * dot(g0, v0) * v0;
    float3 dn1 = pow(w1, 4.) * g1 - 8. * pow(w1, 3.) * dot(g1, v1) * v1;
    float3 dn2 = pow(w2, 4.) * g2 - 8. * pow(w2, 3.) * dot(g2, v2) * v2;
    float3 dn3 = pow(w3, 4.) * g3 - 8. * pow(w3, 3.) * dot(g3, v3) * v3;
    gradient = dn0 + dn1 + dn2 + dn3;
    
    // Calculate my noise
    float n0 = pow(w0, 4.) * dot(v0, g0);
    float n1 = pow(w1, 4.) * dot(v1, g1);
    float n2 = pow(w2, 4.) * dot(v2, g2);
    float n3 = pow(w3, 4.) * dot(v3, g3);
    float n = n0 + n1 + n2 + n3;
    return n;
}