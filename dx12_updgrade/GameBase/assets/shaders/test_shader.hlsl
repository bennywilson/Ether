cbuffer matrixBuffer {
	matrix modelMatrix;
	matrix mvpMatrix;
};
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

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
	output.uv = input.uv;
	return output;
}

///
///	pixelShader
///
 float4 pixel_shader( pixelInput	input, bool IsFrontFace	: SV_IsFrontFace ) : SV_TARGET {
	const float4 albedo = shaderTexture.Sample( SampleType, input.uv );
	const float3 normal = normalize(input.normal.xyz);
	const float3 light_dir = normalize(float3(0.0f, 1.0f, 0.0));
	const float nDotL = pow(saturate(dot(normal, light_dir)), 2.0);

	return albedo * nDotL;
}
 
