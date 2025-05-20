float2 clip_to_uv(float4 position_clip, float2 resolution)
{
    return float2(position_clip.x / resolution.x, position_clip.y / resolution.y);
}

float3 reflect(float3 direction, float3 normal)
{
    return normalize(direction - 2 * dot(direction, normal) * normal);
}