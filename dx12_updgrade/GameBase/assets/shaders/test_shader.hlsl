SamplerState SampleType : register(s0);

Texture2D shaderTexture : register(t0);

//-------------------------------------
struct vertexInput {
	float4 position		: POSITION;
	float2 uv			: TEXCOORD0;
	float4 color		: COLOR;
	float4 normal		: NORMAL;
	float4 tangent		: TANGENT;
};

//-------------------------------------
struct pixelInput {
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
	float4 color		: COLOR;
	float4 normal		: NORMAL;
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
	matrix mvp= {	1.31353f * 0.5, 0.f, 0.f, 0.f,
					0.f, 2.14451f * 0.5, 0.f, 0.f,
					0.f, 0.f, 1.00005f * 0.5, 1.f,
					0.f, -3.f, 4.5f, 5.f};
	output.position = mul( input.position, mvp );
	output.color = input.color;
	output.normal.xyz = input.normal.xyz;
	return output;
}

///
///	pixelShader
///
 PS_OUTPUT pixel_shader( pixelInput	input, bool IsFrontFace	: SV_IsFrontFace ) {
 	PS_OUTPUT output = (PS_OUTPUT) 1;
	output.color.xyz = input.normal.xyz;//float4(1,1,1,1);//shaderTexture.Sample( SampleType, input.uv ).xyz;

	return output;
}
 
