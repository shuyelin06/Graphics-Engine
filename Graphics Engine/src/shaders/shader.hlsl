cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float3 color;
    float padding;
}

/* Vertex Shader Input */
struct VS_IN {
	float3 position_local : POSITION; // POSITION Semantic
};

/* Vertex Shader Output (Pixel Shader Input) */
struct VS_OUT {
	float4 position_clip : SV_POSITION;
	float3 position : POSITION;
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
	VS_OUT output = (VS_OUT)0;

	// Set Output Position in the Clipping Plane
	output.position_clip = float4(input.position_local.x, input.position_local.y, input.position_local.z, 1);
	
	// Set Normal
	output.position = input.position_local;
	
	return output;
}

// Pixel Shader Entry Point
// Takes clipping coordinates, and returns a color
float4 ps_main(VS_OUT input) : SV_TARGET {
	// input.position = normalize(input.position);
	float3 normal = float3(0.0, 0.0, 1.0);

	float d = 1 / (1 + length(input.position) * length(input.position));

	// return float4(d * 1.0, d * 1.0, d * 1.0, 1.0);
    return float4(color.rgb, 1.0);
	
	// return float4(abs(input.position_clip.x),abs(input.position_clip.y),abs(input.position_clip.z), abs(input.position_clip.a)); // must return an RGBA colour
}
