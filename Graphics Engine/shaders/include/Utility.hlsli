// Vector Operations:
// Reflect a direction vector about a normal vector.
float3 reflect(float3 direction, float3 normal)
{
    return normalize(direction - 2 * dot(direction, normal) * normal);
}

// PostProcessing:
// Convert a clip space coordinate to a uv texture coordinate.
// Used if we want to sample a depth buffer or frame buffer.
float2 clip_to_uv(float4 position_clip, float2 resolution)
{
    return float2(position_clip.x / resolution.x, position_clip.y / resolution.y);
}

// Bump Map:
// Given a TBN (Tangent, Binormal, Normal matrix) anda bump map
// RGB value, returns the corresponding normal. 
float3 bump_to_normal(float3 bump, float3x3 tbn)
{
    float3 bump_normal = (bump - float3(0.5f, 0.5f, 0.5f)) * 2.f;
    // Note: BumpMapBuilder assumes that y coordinate is "up", which corresponds to the normal vector
    // on the surface.
    float3 normal = bump_normal.x * tbn[0] + bump_normal.z * tbn[1] + bump_normal.y * tbn[2];
    return normalize(normal);
}