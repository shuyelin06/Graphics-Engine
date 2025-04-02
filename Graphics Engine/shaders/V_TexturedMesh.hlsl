// Per-View Data
cbuffer CB1_VIEW_TRANSFORM : register(b1)
{
    float4x4 m_view;
    float4x4 m_projection;
}

// Per-Mesh Data
cbuffer CB2_MESH_TRANSFORM : register(b2)
{
    float4x4 m_world;
    float4x4 m_normals;
}

// CB3: Skinning Data
#if defined(SKINNED_MESH)
struct SkinnedJoint
{
    float4x4 m_joint;
    float4x4 m_joint_normal;
};
cbuffer CB3_SKIN_DATA : register(b3)
{
    SkinnedJoint joint_data[100];
}
#endif

/* Vertex Shader Input */
#if defined(SKINNED_MESH)
struct VS_IN
{
    float3 position_local : POSITION;
    float2 tex_coord : TEXTURE;
    float3 normal : NORMAL;

    float4 joints : JOINTS;
    float4 weights : WEIGHTS;
};
#else
struct VS_IN
{
    float3 position_local : POSITION;
    float2 tex_coord : TEXTURE;
    float3 normal : NORMAL;
};
#endif

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    float2 tex_coord : TEXTURE;
};

// Vertex Shader Entry Point - Takes VS_IN and outputs a VS_OUT
// Takes a position in local coordinates, and translates them to clipping
// coordinates
// Vertex Shaders MUST output a float4 XYZW setting a vertex's position in homogeneous clip space
// Must be between -1 and 1 on X and Y axes, and 0 and 1 on Z (positive into the screen). 
// Homogeneous coordiantes are identified by the SV_POSITION semantic
VS_OUT vs_main(VS_IN input)
{
	// Zero the Memory
    VS_OUT output = (VS_OUT) 0;
    
    // Texture Coordinates:
    output.tex_coord = input.tex_coord;
    
    // World / Clipping Position:
    float4 pos = float4(input.position_local, 1.0f);
    
#if defined(SKINNED_MESH)
    float4 joints = input.joints;
    float4 weights = input.weights;
    float4x4 m_skin = mul(joint_data[joints.x].m_joint, weights.x)
                        + mul(joint_data[joints.y].m_joint, weights.y)
                        + mul(joint_data[joints.z].m_joint, weights.z)
                        + mul(joint_data[joints.w].m_joint, weights.w);
    float4x4 m_skin_normal = mul(joint_data[joints.x].m_joint_normal, weights.x)
                        + mul(joint_data[joints.y].m_joint_normal, weights.y)
                        + mul(joint_data[joints.z].m_joint_normal, weights.z)
                        + mul(joint_data[joints.w].m_joint_normal, weights.w);
#endif
    
    // Find World Position
#if defined(SKINNED_MESH)
    pos = mul(m_skin, pos);
#endif
    pos = mul(m_world, pos);
    output.world_position = pos.xyz;
    
    // Find Clipping Position
    pos = mul(m_view, pos);
    pos = mul(m_projection, pos);
    output.position_clip = pos;
    
    // Normals:
    float4 norm = float4(input.normal, 0.0f);
    
    norm = mul(m_normals, norm);
#if defined(SKINNED_MESH)
    norm = mul(m_skin_normal, norm);
#endif
    output.normal = normalize(norm.xyz);
	
    return output;
}