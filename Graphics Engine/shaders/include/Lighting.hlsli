float4 toLightView(float3 world_pos, float4x4 m_view, float4x4 m_projection)
{
    // Converts a world position into a coordinate in the light's view.
    float4 pos = float4(world_pos, 1);
    pos = mul(m_view, pos);
    pos = mul(m_projection, pos);
    
    // Perform a manual z-divide
    pos = pos / pos.w;
    
    return pos;
}