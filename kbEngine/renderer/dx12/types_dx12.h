#pragma once

#include "kbRenderBuffer.h"

class RenderBufferDx12 : kbRenderBuffer {
public:
	virtual void Release();

	virtual void CreateVertexBuffer(const int numVerts, const int vertexByteSize);
	virtual void CreateIndexBuffer(const int numIndexes);

	virtual void CreateVertexBuffer(const std::vector<vertexLayout>& vertices);
	virtual void CreateIndexBuffer(const std::vector<ushort>& indices);

	virtual void* Map();
	virtual void Unmap();

};