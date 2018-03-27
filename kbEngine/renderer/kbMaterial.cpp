//===================================================================================================
// kbMaterial.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <iostream>
#include <fstream>
#include "kbCore.h"
#include "kbRenderer_defs.h"
#include "kbMaterial.h"


/**
 *	kbTexture::Load_Internal
 */
bool kbTexture::Load_Internal() {
	if ( g_pD3DDevice == nullptr ) {
		return false;
	}

	// TODO!
	/*HRESULT hr = D3DX11CreateShaderResourceViewFromFile( 
		g_pD3DDevice,
		GetFullFileName().c_str(),
		NULL,
		NULL,
		&m_pTexture,
		NULL );*/

//	return hr == S_OK;
	return true;
}

/**
 *	kbTexture::GetRGBAData
 */
byte * kbTexture::GetRGBAData( unsigned int & width, unsigned int & height ) const {
	return g_pRenderer->GetRawTextureData( m_FullFileName, width, height );
}

/**
 *	kbTexture::Release_Internal
 */
void kbTexture::Release_Internal() {
	SAFE_RELEASE( m_pTexture );
}

/**
 *	kbShader::kbShader
 */
kbShader::kbShader() :
	m_pVertexShader( nullptr ),
	m_pPixelShader( nullptr ),
	m_pVertexLayout( nullptr ),
	m_VertexShaderFunctionName( "vertexShader" ),
	m_PixelShaderFunctionName( "pixelShader" ) {
}

/**
 *	kbShader::kbShader
 */
kbShader::kbShader( const std::string & fileName ) :
	m_pVertexShader( nullptr ),
	m_pPixelShader( nullptr ),
	m_pVertexLayout( nullptr ),
	m_VertexShaderFunctionName( "vertexShader" ),
	m_PixelShaderFunctionName( "pixelShader" ) {
	m_FullFileName = fileName;
}

/**
 *	kbShader::Load_Internal
 */
bool kbShader::Load_Internal() {
	if ( g_pRenderer != nullptr ) {
		g_pRenderer->LoadShader( GetFullFileName(), m_pVertexShader, m_pPixelShader, m_pVertexLayout, m_VertexShaderFunctionName.c_str(), m_PixelShaderFunctionName.c_str() );
	}
	return true;
}

/**
 *	kbShader::Release_Internal
 */
void kbShader::Release_Internal() {
	SAFE_RELEASE( m_pVertexShader );
	SAFE_RELEASE( m_pPixelShader );
	SAFE_RELEASE( m_pVertexLayout );
}

/**
 *	kbShader::CommitShaderParams
 */
void kbShader::CommitShaderParams() {
	kbErrorCheck( g_pRenderer->IsRenderingSynced(), "kbShader::CommitShaderParams() - Can only be called when rendering is synced" );

	m_GlobalShaderParams_RenderThread = m_GlobalShaderParams_GameThread;
}
