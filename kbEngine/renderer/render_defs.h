/// render_defs.h	
///
/// 2025 kbEngine 2.0

#pragma once

///  vertexLayout
struct vertexLayout {
	kbVec3 position;
	kbVec2 uv;
	byte color[4];
	byte normal[4];
	byte tangent[4];

	void SetColor(const kbVec4& inColor) {
		color[0] = (byte)(inColor.x * 255.0f);
		color[1] = (byte)(inColor.y * 255.0f);
		color[2] = (byte)(inColor.z * 255.0f);
		color[3] = (byte)(inColor.w * 255.0f);
	}

	void SetNormal(const kbVec4& inNormal) {
		normal[0] = (byte)(((inNormal.x * 0.5f) + 0.5f) * 255.0f);
		normal[1] = (byte)(((inNormal.y * 0.5f) + 0.5f) * 255.0f);
		normal[2] = (byte)(((inNormal.z * 0.5f) + 0.5f) * 255.0f);
		normal[3] = (byte)(((inNormal.w * 0.5f) + 0.5f) * 255.0f);
	}

	void SetTangent(const kbVec4& inTangent) {
		tangent[0] = (byte)(((inTangent.x * 0.5f) + 0.5f) * 255.0f);
		tangent[1] = (byte)(((inTangent.y * 0.5f) + 0.5f) * 255.0f);
		tangent[2] = (byte)(((inTangent.z * 0.5f) + 0.5f) * 255.0f);
		tangent[3] = (byte)(((inTangent.w * 0.5f) + 0.5f) * 255.0f);
	}

	void SetBitangent(const kbVec4& inBitangent) {
		color[0] = (byte)(((inBitangent.x * 0.5f) + 0.5f) * 255.0f);
		color[1] = (byte)(((inBitangent.y * 0.5f) + 0.5f) * 255.0f);
		color[2] = (byte)(((inBitangent.z * 0.5f) + 0.5f) * 255.0f);
		color[3] = (byte)(((inBitangent.w * 0.5f) + 0.5f) * 255.0f);
	}

	kbVec3 GetNormal() const {
		kbVec3 outNormal((float)normal[0], (float)normal[1], (float)normal[2]);
		outNormal.x = ((outNormal.x / 255.0f) * 2.0f) - 1.0f;
		outNormal.y = ((outNormal.y / 255.0f) * 2.0f) - 1.0f;
		outNormal.z = ((outNormal.z / 255.0f) * 2.0f) - 1.0f;

		outNormal.Normalize();
		return outNormal;
	}

	kbVec3 GetTangent() const {
		kbVec3 outTangent((float)tangent[0], (float)tangent[1], (float)tangent[2]);
		outTangent.x = ((outTangent.x / 255.0f) * 2.0f) - 1.0f;
		outTangent.y = ((outTangent.y / 255.0f) * 2.0f) - 1.0f;
		outTangent.z = ((outTangent.z / 255.0f) * 2.0f) - 1.0f;

		outTangent.Normalize();
		return outTangent;
	}

	kbVec4 GetColor() const {
		kbVec4 outColor((float)color[2], (float)color[1], (float)color[0], (float)color[3]);
		outColor.x = outColor.x / 255.0f;
		outColor.y = outColor.y / 255.0f;
		outColor.z = outColor.z / 255.0f;
		outColor.w = outColor.w / 255.0f;

		return outColor;
	}

	void Clear() {
		memset(this, 0, sizeof(vertexLayout));
	}

	bool operator ==(const vertexLayout& op2) const {
		const float epsilon = 0.0000000001f;
		return  position.Compare(op2.position, epsilon) && uv.Compare(op2.uv, epsilon) &&
			kbCompareByte4(color, op2.color) &&
			kbCompareByte4(normal, op2.normal) && kbCompareByte4(tangent, op2.tangent);
	}
};

/// RenderPipeline
class RenderPipeline {
public:
	virtual ~RenderPipeline() = 0 {}
	virtual void release() = 0;

private:
	std::string name;
};

/// RenderBuffer
class RenderBuffer {
public:
	RenderBuffer() = default;

	~RenderBuffer() {
		release();
	}

	virtual void release() {};

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices);
	virtual void write_index_buffer(const std::vector<uint16_t>& indices);

	uint32_t num_elements() const { return m_num_elements; }
	uint32_t size_bytes() const { return m_size_bytes; }

private:
	uint32_t m_num_elements = 0;
	uint32_t m_size_bytes = 0;
};


///  kbVertexHash
struct kbVertexHash {
	size_t operator()(const vertexLayout& key) const {
		float val = key.position.x + key.position.y + key.position.z;
		return (INT_PTR)val;
	}
};
