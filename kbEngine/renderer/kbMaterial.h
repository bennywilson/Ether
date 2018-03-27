//===================================================================================================
// kbMaterial.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBMATERIAL_H_
#define _KBMATERIAL_H_

/**
 *	kbTexture
 */
class kbTexture : public kbResource {

//---------------------------------------------------------------------------------------------------
public:

												kbTexture() : m_pTexture( nullptr ) { }
												~kbTexture() { kbErrorCheck( m_pTexture == nullptr, " kbTexture::~kbTexture() - Destructing a kbTexture that hasn't been released" ); }

	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_TEXTURE; }

	void										Release() { Release_Internal(); }		// todo: should be a resource like everything else

	void *										GetTextureData() const { return m_pTexture; }

	byte *										GetRGBAData( unsigned int & width, unsigned int & height ) const;

private:

	virtual bool								Load_Internal();
	virtual void								Release_Internal();

	kbHWTexture *								m_pTexture;
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
	const kbHWVertexLayout *					GetVertexLayout() const { return m_pVertexLayout; }

	void										SetVertexShaderFunctionName( const std::string & inName ) { m_VertexShaderFunctionName = inName; }
	void										SetPixelShaderFunctionName( const std::string & inName ) { m_PixelShaderFunctionName = inName; }

	void										SetGlobalShaderParams( const std::vector<kbVec4> & shaderParams ) { m_GlobalShaderParams_GameThread = shaderParams; }
	void										CommitShaderParams();
	const std::vector<kbVec4>	&				GetGlobalShaderParams() const { return m_GlobalShaderParams_RenderThread; }	// todo: check if render thread

private:
	virtual bool								Load_Internal();
	virtual void								Release_Internal();

	kbHWVertexShader *							m_pVertexShader;
	kbHWPixelShader *							m_pPixelShader;
	kbHWVertexLayout *							m_pVertexLayout;

	std::string									m_VertexShaderFunctionName;
	std::string									m_PixelShaderFunctionName;

	std::vector<kbVec4>							m_GlobalShaderParams_GameThread;
	std::vector<kbVec4>							m_GlobalShaderParams_RenderThread;
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

private:

	const kbTexture *							m_Texture;
	kbShader *									m_pShader;
	kbColor										m_DiffuseColor;
	cullingMode_t								m_CullingMode;
};

#endif