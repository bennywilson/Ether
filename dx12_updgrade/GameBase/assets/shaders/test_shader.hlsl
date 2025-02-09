struct SceneData {
	matrix modelMatrix;
	float4 color;
};
ConstantBuffer<SceneData> scene_constants[1024] : register(b0);

struct SceneIndex {
	uint index;
};
ConstantBuffer<SceneIndex> scene_index : register(b0, space1);

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
	SceneData matrixBuffer = scene_constants[scene_index.index];
	pixelInput output = (pixelInput)(0);
	output.position = input.position;
	output.position = mul( input.position, matrixBuffer.modelMatrix );
	output.color = input.color * matrixBuffer.color;
	output.normal.xyz = input.normal.xyz;
	output.uv = input.uv;
	return output;
}

///
///	pixelShader
///
 float4 pixel_shader( pixelInput	input, bool IsFrontFace	: SV_IsFrontFace ) : SV_TARGET {
	const float4 albedo = shaderTexture.Sample( SampleType, input.uv ) * input.color;
	//const float3 normal = normalize(input.normal.xyz);
	//const float3 light_dir = normalize(float3(0.0f, 1.0f, 0.0));
	//const float nDotL = pow(saturate(dot(normal, light_dir)), 2.0);
	//return 1;
	return albedo;// * nDotL;
}
 
