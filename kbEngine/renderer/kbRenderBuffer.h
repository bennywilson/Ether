/// kbRenderBuffer.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "render_defs.h"
#include <D3D11.h>

typedef ID3D11Buffer kbHWBuffer;
typedef ID3D11ShaderResourceView kbHWTexture;
typedef ID3D11VertexShader kbHWVertexShader;
typedef ID3D11GeometryShader kbHWGeometryShader;
typedef ID3D11PixelShader kbHWPixelShader;
typedef ID3D11InputLayout kbHWVertexLayout;

/**
 *	kbRenderBuffer
 */
class kbRenderBuffer {
public:
	kbRenderBuffer() : m_pBuffer(nullptr) { }
	~kbRenderBuffer() {
		blk::warn_check(
			m_pBuffer == nullptr,
			"kbRenderBuffer::~kbRenderBuffer - Destructing a render buffer that hasn't been released"
		);
	}

	virtual void Release();

	virtual void CreateVertexBuffer(const int numVerts, const int vertexByteSize);
	virtual void CreateIndexBuffer(const int numIndexes);

	virtual void CreateVertexBuffer(const std::vector<vertexLayout>& vertices);
	virtual void CreateIndexBuffer(const std::vector<ushort>& indices);

	virtual void* Map();
	virtual void Unmap();

	const ID3D11Buffer* GetBufferPtr() const { return m_pBuffer; }

private:
	ID3D11Buffer* m_pBuffer;
};
