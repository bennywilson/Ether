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
Texture2D color_tex : register(t0);

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
	output.position = mul(input.position, matrixBuffer.modelMatrix);

	float3 world_pos = mul(input.position, matrixBuffer.world_matrix).xyz;
	output.to_cam = matrixBuffer.camera.xyz - world_pos;
	output.color = matrixBuffer.color;
	output.spec = matrixBuffer.spec;
	output.normal.xyz = mul(input.normal.xyz, (float3x3)matrixBuffer.world_matrix);
	output.uv = input.uv;
	return output;
}

///	pixelShader
float4 pixel_shader(PixelInput input) : SV_TARGET {
	const float4 albedo = color_tex.Sample( SampleType, input.uv ) * input.color;
	const float3 normal = normalize(input.normal.xyz);
	const float3 light_dir = normalize(float3(0.0f, 1.0f, -1.0));

	// Diffuse
	const float n_dot_l = pow(saturate(dot(normal, light_dir)), 1.0);
	const float3 diffuse = n_dot_l.xxx * albedo.xyz;

	// Spec
	const float3 to_cam = normalize(input.to_cam);
	const float3 r = 2 * n_dot_l * normal - light_dir;
	const float r_dot_v = saturate(dot(to_cam, r));
	const float highlight = pow(r_dot_v, input.spec.w);
	const float3 spec = input.spec.zzz * highlight.xxx * albedo.xyz;

	// Ambient
	const float3 ambient = float3(0.5f, 0.5f, 0.5f) * albedo.xyz;

	return float4(diffuse + spec + ambient, 1.f);
}
