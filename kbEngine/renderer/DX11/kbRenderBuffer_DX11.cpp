//===================================================================================================
// kbRenderBuffer_DX11.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <DirectXMath.h>
#include "kbCore.h"
#include "kbRenderer_DX11.h"
#include "kbRenderBuffer.h"

/**
 *	kbRenderBuffer::Release
 */
void kbRenderBuffer::Release() {
	SAFE_RELEASE( m_pBuffer );
}

/**
 *	kbRenderBuffer::CreateVertexBuffer
 */
void kbRenderBuffer::CreateVertexBuffer( const int numVerts, const int vertexSizeInBytes ) {
	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.ByteWidth = ( unsigned int ) ( numVerts * vertexSizeInBytes );
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	HRESULT hr = g_pD3DDevice->CreateBuffer( &vertexBufferDesc, NULL, &m_pBuffer );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderBuffer::CreateVertexBuffer() - Failed to create vertex buffer" );

#if defined(_DEBUG)
	static int BufferNum = 0;
	std::string BufferName = "VertexBuffer_";
	BufferName += std::to_string( BufferNum++ );
	m_pBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)BufferName.length(), BufferName.c_str() );
#endif
}

/**
 *	kbRenderBuffer::CreateIndexBuffer
 */
void kbRenderBuffer::CreateIndexBuffer( const int numIndices ) {
	D3D11_BUFFER_DESC indexBufferDesc = { 0 };
	indexBufferDesc.ByteWidth = (uint) ( numIndices * sizeof(ushort) );
	indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	HRESULT hr = g_pD3DDevice->CreateBuffer( &indexBufferDesc, NULL, &m_pBuffer );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderBuffer::CreateIndexBuffer() - Failed to create index buffer" );

#if defined(_DEBUG)
	static int BufferNum = 0;
	std::string BufferName = "IndexBuffer_";
	BufferName += std::to_string( BufferNum++ );
	m_pBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)BufferName.length(), BufferName.c_str() );
#endif
}

/**
 *	kbRenderBuffer::Map
 */
void * kbRenderBuffer::Map() {
	D3D11_MAPPED_SUBRESOURCE resource;
	ID3D11DeviceContext * pImmediateContext = nullptr;

	g_pD3DDevice->GetImmediateContext( &pImmediateContext );

	HRESULT hr = pImmediateContext->Map( m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderBuffer::CreateVertexBuffer() - Failed to create vertex buffer" );

	pImmediateContext->Release();

	return resource.pData;
}

/**
 *	kbRenderBuffer::Unmap
 */
void kbRenderBuffer::Unmap() {
	ID3D11DeviceContext * pImmediateContext = nullptr;
	g_pD3DDevice->GetImmediateContext( &pImmediateContext );

	pImmediateContext->Unmap( m_pBuffer, 0 );

	pImmediateContext->Release();
}

/**
 *	kbRenderBuffer::CreateVertexBuffer
 */
void kbRenderBuffer::CreateVertexBuffer( const std::vector< vertexLayout > & vertices ) {

	if ( m_pBuffer != nullptr ) {
		m_pBuffer->Release();
		m_pBuffer = nullptr;
	}

	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.ByteWidth = static_cast<UINT>( sizeof( vertexLayout ) * vertices.size() );
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT hr = g_pD3DDevice->CreateBuffer( &vertexBufferDesc, &vertexData, &m_pBuffer );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderBuffer::CreateVertexBuffer() - Failed to create vertex buffer" );
}

/**
 *	kbRenderBuffer::CreateIndexBuffer
 */
void kbRenderBuffer::CreateIndexBuffer( const std::vector<ushort> & indices ) {
	if ( m_pBuffer != nullptr ) {
		m_pBuffer->Release();
		m_pBuffer = nullptr;
	}

	D3D11_BUFFER_DESC indexBufferDesc = { 0 };
	indexBufferDesc.ByteWidth = static_cast<UINT>( sizeof(ushort) * indices.size() );
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	HRESULT hr = g_pD3DDevice->CreateBuffer( &indexBufferDesc, &indexData, &m_pBuffer );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderBuffer::CreateVertexBuffer() - Failed to create index buffer" );
}