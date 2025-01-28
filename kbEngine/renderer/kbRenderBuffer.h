//===================================================================================================
// kbRenderBuffer.h
//
//
// 2016-2025 kbEngine 2.0
//===================================================================================================
#pragma once

#include "kbRenderer_defs.h"

/**
 *	kbRenderBuffer
 */
class kbRenderBuffer {
public:
	kbRenderBuffer() : m_pBuffer( nullptr ) { }
	~kbRenderBuffer() {
		kbWarningCheck( m_pBuffer == nullptr, "kbRenderBuffer::~kbRenderBuffer - Destructing a render buffer that hasn't been released" );
	}

	virtual void Release();

	virtual void CreateVertexBuffer( const int numVerts, const int vertexByteSize );
	virtual void CreateIndexBuffer( const int numIndexes );

	virtual void CreateVertexBuffer( const std::vector<vertexLayout> & vertices );
	virtual void CreateIndexBuffer( const std::vector<ushort> & indices );

	virtual void * Map();
	virtual void Unmap();


	const kbHWBuffer * GetBufferPtr() const { return m_pBuffer; }

private:

	kbHWBuffer *								m_pBuffer;
};
