/// kbMaterial.h
///
/// 2016-2025 kbEngine 2.0

#pragma once

#include <memory>
#include "kbRenderBuffer.h"
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

	kbHWTexture*								GetGPUTexture() const { return m_pGPUTexture; }

	const uint8_t*								GetCPUTexture( unsigned int & width, unsigned int & height );

	uint										GetWidth() const { return m_TextureWidth; }
	uint										GetHeight() const { return m_TextureHeight; }

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
		binding_t( const std::string & inName, const size_t offset, const bool bHasDefaultValue, const kbVec4 defaultValue, const bool bIsUserDefinedVar ) :
			m_VarName( inName ),
			m_VarByteOffset( offset ),
			m_DefaultValue( defaultValue ),
			m_bHasDefaultValue( bHasDefaultValue ),
			m_bIsUserDefinedVar( bIsUserDefinedVar ) { }

		std::string								m_VarName;
		size_t									m_VarByteOffset;
		kbVec4									m_DefaultValue;
		bool									m_bHasDefaultValue;
		bool									m_bIsUserDefinedVar;
	};
	std::vector<binding_t>						m_VarBindings;

	struct textureBinding_t {
		textureBinding_t() : m_pDefaultTexture( nullptr ), m_pDefaultRenderTexture( nullptr ), m_bIsUserDefinedVar( false ) { }

		std::string								m_TextureName;
		kbTexture *								m_pDefaultTexture;
		kbRenderTexture *						m_pDefaultRenderTexture;
		bool									m_bIsUserDefinedVar;
	};
    std::vector<textureBinding_t>				m_Textures;

	bool ContainsBinding( const char *const pBinding ) {
		for ( int i = 0; i < m_VarBindings.size(); i++ ) {
			if ( m_VarBindings[i].m_VarName == pBinding ) {
				return true;
			}
		}
		return false;
	}

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
	bool										IsBlendEnabled() const { return m_bBlendEnabled; }
	bool										IsDistortionEnabled() const { return m_bDistortionEnabled; }

	kbBlend										GetSrcBlend() const { return m_SrcBlend; }
	kbBlend										GetDstBlend() const { return m_DstBlend; }
	kbBlendOp									GetBlendOp() const { return m_BlendOp; }

	kbBlend										GetSrcBlendAlpha() const { return m_SrcBlendAlpha; }
	kbBlend										GetDstBlendAlpha() const { return m_DstBlendAlpha; }
	kbBlendOp									GetBlendOpAlpha() const { return m_BlendOpAlpha; }

	kbColorWriteEnable							GetColorWriteEnable() const { return m_ColorWriteEnable; }

	ECullMode									GetCullMode() const { return m_CullMode; }

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

	bool										m_bBlendEnabled;
	bool										m_bDistortionEnabled;

	kbBlend										m_SrcBlend;
	kbBlend										m_DstBlend;
	kbBlendOp									m_BlendOp;

	kbBlend										m_SrcBlendAlpha;
	kbBlend										m_DstBlendAlpha;
	kbBlendOp									m_BlendOpAlpha;

	kbColorWriteEnable							m_ColorWriteEnable;
	ECullMode									m_CullMode;
};

/**
 *	kbMaterial
 */
class kbMaterial {

	friend class kbModel;

//---------------------------------------------------------------------------------------------------
public:

												kbMaterial() : m_pShader( nullptr ), m_CullingMode( CullMode_BackFaces ) { }

	const kbShader *							GetShader() const { return m_pShader; }
	const std::vector<const kbTexture *>		GetTextureList() const { return m_Textures; }
	const kbColor &								GetDiffuseColor() const { return m_DiffuseColor; }
	ECullMode									GetCullingMode() const { return m_CullingMode; }

	void										SetCullingMode( const ECullMode newMode ) { m_CullingMode = newMode; }

private:

	std::vector<const kbTexture *>				m_Textures;
	kbShader *									m_pShader;
	kbColor										m_DiffuseColor;
	ECullMode									m_CullingMode;
};
