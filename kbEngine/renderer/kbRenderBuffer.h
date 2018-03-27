//===================================================================================================
// kbRenderBuffer.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBRENDERBUFFER_H_
#define _KBRENDERBUFFER_H_

#include "kbRenderer_defs.h"

/**
 *	kbRenderBuffer
 */
class kbRenderBuffer {

//---------------------------------------------------------------------------------------------------
public:

												kbRenderBuffer() : m_pBuffer( nullptr ) { }
												~kbRenderBuffer() { kbWarningCheck( m_pBuffer == nullptr, "kbRenderBuffer::~kbRenderBuffer - Destructing a render buffer that hasn't been released" ); }

	void										Release();

	void										CreateVertexBuffer( const int numVerts, const int vertexByteSize );
	void										CreateIndexBuffer( const int numIndexes );

	void										CreateVertexBuffer( const std::vector< vertexLayout > & vertices );
	void										CreateIndexBuffer( const std::vector< unsigned long > & indices );

	void *										Map();
	void										Unmap();


	const kbHWBuffer *							GetBufferPtr() const { return m_pBuffer; }

private:
	kbHWBuffer *								m_pBuffer;
};

#endif
