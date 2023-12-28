cbuffer TRANSFORM_MATRICES : register(b0)
{
    row_major float4x4 transform;
}

/* Vertex Shader Input */
struct VS_IN {
	float3 position_local : POSITION; // POSITION Semantic
    float3 color : COLOR;
};

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT {
	float4 position_clip : SV_POSITION;
    float3 color : COLOR;
};

// Vertex Shader Entry Point - Takes VS_IN and outputs a VS_OUT
// Takes a position in local coordinates, and translates them to clipping
// coordinates
// 
// Vertex Shaders MUST output a float4 XYZW setting a vertex's position in homogeneous clip space
// Must be between -1 and 1 on X and Y axes, and 0 and 1 on Z (positive into the screen). 
// Homogeneous coordiantes are identified by the SV_POSITION semantic
VS_OUT vs_main(VS_IN input) {
	// Zero the Memory
	VS_OUT output = (VS_OUT) 0;

	// Set Output Position in the Clipping Plane
    float4 pos = float4(input.position_local, 1.0f);
    pos = mul(pos, transform);

	output.position_clip = pos;
    output.color = input.color;
	
	return output;
}

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET {
    return float4(input.color, 1.0);
}
