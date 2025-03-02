/// render_defs.h	
///
/// 2025 blk 1.0

#pragma once

#include "Matrix.h"

/// vertexLayout
struct vertexLayout {
	Vec3 position;
	Vec2 uv;
	byte color[4];
	byte normal[4];
	byte tangent[4];

	void SetColor(const Vec4& inColor) {
		color[0] = (byte)(inColor.x * 255.0f);
		color[1] = (byte)(inColor.y * 255.0f);
		color[2] = (byte)(inColor.z * 255.0f);
		color[3] = (byte)(inColor.w * 255.0f);
	}

	void SetNormal(const Vec4& inNormal) {
		normal[0] = (byte)(((inNormal.x * 0.5f) + 0.5f) * 255.0f);
		normal[1] = (byte)(((inNormal.y * 0.5f) + 0.5f) * 255.0f);
		normal[2] = (byte)(((inNormal.z * 0.5f) + 0.5f) * 255.0f);
		normal[3] = (byte)(((inNormal.w * 0.5f) + 0.5f) * 255.0f);
	}

	void SetTangent(const Vec4& inTangent) {
		tangent[0] = (byte)(((inTangent.x * 0.5f) + 0.5f) * 255.0f);
		tangent[1] = (byte)(((inTangent.y * 0.5f) + 0.5f) * 255.0f);
		tangent[2] = (byte)(((inTangent.z * 0.5f) + 0.5f) * 255.0f);
		tangent[3] = (byte)(((inTangent.w * 0.5f) + 0.5f) * 255.0f);
	}

	void SetBitangent(const Vec4& inBitangent) {
		color[0] = (byte)(((inBitangent.x * 0.5f) + 0.5f) * 255.0f);
		color[1] = (byte)(((inBitangent.y * 0.5f) + 0.5f) * 255.0f);
		color[2] = (byte)(((inBitangent.z * 0.5f) + 0.5f) * 255.0f);
		color[3] = (byte)(((inBitangent.w * 0.5f) + 0.5f) * 255.0f);
	}

	Vec3 GetNormal() const {
		Vec3 outNormal((float)normal[0], (float)normal[1], (float)normal[2]);
		outNormal.x = ((outNormal.x / 255.0f) * 2.0f) - 1.0f;
		outNormal.y = ((outNormal.y / 255.0f) * 2.0f) - 1.0f;
		outNormal.z = ((outNormal.z / 255.0f) * 2.0f) - 1.0f;

		outNormal.normalize_self();
		return outNormal;
	}

	Vec3 GetTangent() const {
		Vec3 outTangent((float)tangent[0], (float)tangent[1], (float)tangent[2]);
		outTangent.x = ((outTangent.x / 255.0f) * 2.0f) - 1.0f;
		outTangent.y = ((outTangent.y / 255.0f) * 2.0f) - 1.0f;
		outTangent.z = ((outTangent.z / 255.0f) * 2.0f) - 1.0f;

		outTangent.normalize_self();
		return outTangent;
	}

	Vec4 GetColor() const {
		Vec4 outColor((float)color[2], (float)color[1], (float)color[0], (float)color[3]);
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
		return position.compare(op2.position, epsilon) && uv.compare(op2.uv, epsilon) &&
			kbCompareByte4(color, op2.color) &&
			kbCompareByte4(normal, op2.normal) && kbCompareByte4(tangent, op2.tangent);
	}
};

/// ParticleVertex
struct ParticleVertex {
	Vec3 position;
	Vec2 uv;
	byte color[4];
	f32 rotation;
	f32 scale;
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
	virtual ~RenderBuffer() {}

	virtual void release() {};

	virtual u8* map() { return nullptr; }
	virtual void unmap() {}

	void create_vertex_buffer(const u32 num_verts);
	void write_vertex_buffer(const std::vector<vertexLayout>& vertices);

	void create_index_buffer(const u32 num_indices);
	void write_index_buffer(const std::vector<uint16_t>& indices);

	u32 num_elements() const { return m_num_elements; }
	u32 size_bytes() const { return m_size_bytes; }

private:
	virtual void create_internal() {}

	u32 m_num_elements = 0;
	u32 m_size_bytes = 0;
};


///  kbVertexHash
struct kbVertexHash {
	size_t operator()(const vertexLayout& key) const {
		f32 val = key.position.x + key.position.y + key.position.z;
		return (INT_PTR)val;
	}
};
