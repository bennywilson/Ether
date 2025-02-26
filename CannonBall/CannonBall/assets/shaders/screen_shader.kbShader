struct SceneData {
	matrix modelMatrix;
	matrix world_matrix;
	float4 color;
	float4 spec;
	float4 camera;
	float4 pad0;
	matrix pad1;
};

ConstantBuffer<SceneData> scene_constants[1024] : register(b0);

struct SceneIndex {
	uint index;
};
ConstantBuffer<SceneIndex> scene_index : register(b0, space1);

SamplerState SampleType : register(s0);
Texture2D shaderTexture : register(t0);

/// VertexInput
struct VertexInput {
	float4 position		: POSITION;
	float2 uv			: TEXCOORD0;
	float4 color		: COLOR;
	float4 normal		: NORMAL;
	float4 tangent		: TANGENT;
};

/// PixelInput
struct PixelInput {
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
	float3 to_cam		: TEXCOORD1;
	float4 spec			: TEXCOORD2;
	float4 color		: COLOR;
	float4 normal		: NORMAL;
};

///	vertex_shader
PixelInput vertex_shader(VertexInput input) {
	SceneData matrixBuffer = scene_constants[scene_index.index];
	PixelInput output = (PixelInput)(0);
	output.position = input.position;
	output.uv = input.uv;
	return output;
}

///	pixelShader
float4 pixel_shader(PixelInput input) : SV_TARGET {
	const float4 albedo = shaderTexture.Sample( SampleType, input.uv );
	return albedo;
}
