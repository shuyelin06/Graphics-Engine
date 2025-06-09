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
// Returns values in the rangs [0, A^2). Divide by 288.f to clamp to [0,1].
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
    return normalize(float3(x, y, z) - float3(0.5f, 0.5f, 0.5f));
}

// --- Simplex Noise ---
// Procedural shader function for simplex noise.
// https://www.shadertoy.com/view/tfdXRB
// To use, call noise3D.
// This returns a noise value in [0,1], and a gradient vector.
float3 xyz_to_uvw(float3 xyz)
{
    float u = xyz.y + xyz.z;
    float v = xyz.x + xyz.z;
    float w = xyz.x + xyz.y;
    return float3(u, v, w);
}
float3 uvw_to_xyz(float3 uvw)
{
    float x = (-uvw.x + uvw.y + uvw.z) / 2.;
    float y = (uvw.x - uvw.y + uvw.z) / 2.;
    float z = (uvw.x + uvw.y - uvw.z) / 2.;
    return float3(x, y, z);
}

float noise3D(float3 p, float3 seed, out float3 gradient)
{
    float3 uvw = xyz_to_uvw(p);
    
    // Find the corners of our simplex
    // We use min / max with step functions to determine what coordinate
    // we +1 in. Min essentially serves as an AND, and max essentially serves
    // as an OR
    float3 uvw_f = frac(uvw);
    
    float3 uvw_0 = floor(uvw);
    
    float3 uvw_1 = uvw_0; // uvw_0 + 1 in the largest coordinate
    uvw_1.x += min(step(uvw_f.y, uvw_f.x), step(uvw_f.z, uvw_f.x));
    uvw_1.y += min(step(uvw_f.x, uvw_f.y), step(uvw_f.z, uvw_f.y));
    uvw_1.z += min(step(uvw_f.x, uvw_f.z), step(uvw_f.y, uvw_f.z));
    
    float3 uvw_2 = uvw_1; // uvw_1 + 1 in the 2nd largest coordinate
    uvw_2.x += max(min(step(uvw_f.y, uvw_f.x), step(uvw_f.x, uvw_f.z)),
                min(step(uvw_f.z, uvw_f.x), step(uvw_f.x, uvw_f.y)));
    uvw_2.y += max(min(step(uvw_f.x, uvw_f.y), step(uvw_f.y, uvw_f.z)),
                min(step(uvw_f.z, uvw_f.y), step(uvw_f.y, uvw_f.x)));
    uvw_2.z += max(min(step(uvw_f.x, uvw_f.z), step(uvw_f.z, uvw_f.y)),
                min(step(uvw_f.y, uvw_f.z), step(uvw_f.z, uvw_f.x)));
    
    float3 uvw_3 = uvw_0 + float3(1.f, 1.f, 1.f);
    
    // Calculate the gradients at these 3 points
    float3 g0 = gradient3D(uvw_0, seed);
    float3 g1 = gradient3D(uvw_1, seed);
    float3 g2 = gradient3D(uvw_2, seed);
    float3 g3 = gradient3D(uvw_3, seed);
    
    // Convert the corners to xyz, and find our directional vectors
    float3 v0 = p - uvw_to_xyz(uvw_0);
    float3 v1 = p - uvw_to_xyz(uvw_1);
    float3 v2 = p - uvw_to_xyz(uvw_2);
    float3 v3 = p - uvw_to_xyz(uvw_3);
    
    float d = 0.55;
    float w0 = max(d - dot(v0, v0), 0.);
    float w1 = max(d - dot(v1, v1), 0.);
    float w2 = max(d - dot(v2, v2), 0.);
    float w3 = max(d - dot(v3, v3), 0.);
    
    // Calculate my gradient
    float3 dn0 = pow(w0, 3.) * g0 - 6. * pow(w0, 2.) * dot(g0, v0) * v0;
    float3 dn1 = pow(w1, 3.) * g1 - 6. * pow(w1, 2.) * dot(g1, v1) * v1;
    float3 dn2 = pow(w2, 3.) * g2 - 6. * pow(w2, 2.) * dot(g2, v2) * v2;
    float3 dn3 = pow(w3, 3.) * g3 - 6. * pow(w3, 2.) * dot(g3, v3) * v3;
    gradient = dn0 + dn1 + dn2 + dn3;
    
    // Calculate my noise
    float n0 = pow(w0, 3.) * dot(v0, g0);
    float n1 = pow(w1, 3.) * dot(v1, g1);
    float n2 = pow(w2, 3.) * dot(v2, g2);
    float n3 = pow(w3, 3.) * dot(v3, g3);
    float n = n0 + n1 + n2 + n3;
    
    return (n * 23.9 + 1.) / 2.;
}

