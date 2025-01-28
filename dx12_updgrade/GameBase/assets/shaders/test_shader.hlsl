
//-------------------------------------
struct vertexInput {
	float4 position		: POSITION;
	float4 color		: COLOR;
};

//-------------------------------------
struct pixelInput {
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
	float4 color		: COLOR;
};

//-------------------------------------
struct PS_OUTPUT {
	float4 color		: SV_TARGET0;	// Albedo.rgb, Emissive.z
};


///
///	vertexShader
///
pixelInput vertex_shader(vertexInput input) {
	pixelInput output = (pixelInput)(0);
	output.position = input.position;
	output.color = input.color;
	return output;
}

///
///	pixelShader
///
 PS_OUTPUT pixel_shader( pixelInput	input, bool IsFrontFace	: SV_IsFrontFace ) {
 	PS_OUTPUT output = (PS_OUTPUT) 0;
	output.color = float4(1,0,1,1);

	return output;
}
 
