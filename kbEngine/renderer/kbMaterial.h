//===================================================================================================
// kbMaterial.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBMATERIAL_H_
#define _KBMATERIAL_H_

#include <memory>
#include "kbRenderer_Defs.h"

/**
 *	kbTexture
 */
class kbTexture : public kbResource {

//---------------------------------------------------------------------------------------------------
public:

												kbTexture();
	explicit									kbTexture( const kbString & fileName );

												~kbTexture() { kbErrorCheck( m_pGPUTexture == nullptr, " kbTexture::~kbTexture() - Destructing a kbTexture that hasn't been released" ); }

	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_TEXTURE; }

	void										Release() { Release_Internal(); }		// todo: should be a resource like everything else

	kbHWTexture *								GetGPUTexture() const { return m_pGPUTexture; }

	const uint8_t *								GetCPUTexture( unsigned int & width, unsigned int & height );

private:

	virtual bool								Load_Internal();
	virtual void								Release_Internal();

	kbHWTexture *								m_pGPUTexture;
	std::unique_ptr<uint8_t[]>					m_pCPUTexture;

	uint										m_TextureWidth;
	uint										m_TextureHeight;

	bool										m_bIsCPUTexture;
};

/**
 *	kbShaderVarBinding_t
 */
struct kbShaderVarBindings_t {
	kbShaderVarBindings_t() : m_ConstantBufferSizeBytes( 0 ) { }

	size_t										m_ConstantBufferSizeBytes;

	struct binding_t {
		binding_t( const std::string & inName, const size_t offset ) : m_VarName ( inName ), m_VarByteOffset( offset ) { }

		std::string								m_VarName;
		size_t									m_VarByteOffset;
	};
	std::vector<binding_t>						m_VarBindings;

	bool ContainsBinding( const char *const pBinding ) {
		for ( int i = 0; i < m_VarBindings.size(); i++ ) {
			if ( m_VarBindings[i].m_VarName == pBinding ) {
				return true;
			}
		}
		return false;
	}

    std::vector<std::string>					m_TextureNames;
};

/**
 *	kbShader
 */
class kbShader : public kbResource {

	friend class kbShader_TypeInfo;
	static kbShader_TypeInfo typeInfo;

//---------------------------------------------------------------------------------------------------
public:

												kbShader( const std::string & fileName );
												kbShader();

	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_SHADER; }

	const kbHWVertexShader *					GetVertexShader() const { return m_pVertexShader; }
	const kbHWPixelShader *						GetPixelShader() const { return m_pPixelShader; }
    const kbHWGeometryShader *                  GetGeometryShader() const { return m_pGeometryShader; }

	const kbHWVertexLayout *					GetVertexLayout() const { return m_pVertexLayout; }

	void										SetVertexShaderFunctionName( const std::string & inName ) { m_VertexShaderFunctionName = inName; }
	void										SetPixelShaderFunctionName( const std::string & inName ) { m_PixelShaderFunctionName = inName; }

	void										SetGlobalShaderParams( const std::vector<kbVec4> & shaderParams ) { m_GlobalShaderParams_GameThread = shaderParams; }
	void										CommitShaderParams();
	const std::vector<kbVec4> &					GetGlobalShaderParams() const { return m_GlobalShaderParams_RenderThread; }	// todo: check if render thread

	const kbShaderVarBindings_t &				GetShaderVarBindings() const { return m_ShaderVarBindings; }

	// Render States
	kbBlendFactor								GetSrcBlendFactor() const { return m_SrcBlendFactor; }
	kbBlendFactor								GetDstBlendFactor() const { return m_DstBlendFactor; }

private:
	virtual bool								Load_Internal();
	virtual void								Release_Internal();

	kbHWVertexShader *							m_pVertexShader;
    kbHWGeometryShader *                        m_pGeometryShader;
	kbHWPixelShader *							m_pPixelShader;
	kbHWVertexLayout *							m_pVertexLayout;

	std::map<kbString, int>						m_ShaderConstantsMap;	// Maps constant variable name to it's byte offset

	std::string									m_VertexShaderFunctionName;
	std::string									m_PixelShaderFunctionName;

	std::vector<kbVec4>							m_GlobalShaderParams_GameThread;
	std::vector<kbVec4>							m_GlobalShaderParams_RenderThread;

	kbShaderVarBindings_t						m_ShaderVarBindings;

	kbBlendFactor								m_SrcBlendFactor;
	kbBlendFactor								m_DstBlendFactor;
};

/**
 *	kbMaterial
 */
class kbMaterial {

	friend class kbModel;

//---------------------------------------------------------------------------------------------------
public:

	enum cullingMode_t {
		CM_FrontFaces,
		CM_BackFaces,
		CM_None,
	};

												kbMaterial() : m_Texture( nullptr ), m_pShader( nullptr ), m_CullingMode( CM_BackFaces ) { }

	const kbShader *							GetShader() const { return m_pShader; }
	const kbTexture *							GetTexture() const { return m_Texture; }
	const kbColor &								GetDiffuseColor() const { return m_DiffuseColor; }
	cullingMode_t								GetCullingMode() const { return m_CullingMode; }

	void										SetCullingMode( const cullingMode_t newMode ) { m_CullingMode = newMode; }

private:

	const kbTexture *							m_Texture;
	kbShader *									m_pShader;
	kbColor										m_DiffuseColor;
	cullingMode_t								m_CullingMode;
};

#endif