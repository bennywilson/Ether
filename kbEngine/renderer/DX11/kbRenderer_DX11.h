//==============================================================================
// kbRenderer_DX11.h
//
// Renderer implementation using DX11 API
//
// 2016 kbEngine 2.0
//==============================================================================
#ifndef _KBRENDERER_DX11_H_
#define _KBRENDERER_DX11_H_

#include <D3D11.h>
#include <DirectXMath.h>
#include "kbRenderer.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbRenderer_defs.h"
#include "kbMaterial.h"
#include "kbJobManager.h"

using namespace DirectX;

class kbModel;

class kbRenderWindow_DX11 : public kbRenderWindow {

	friend class kbRenderer_DX11;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderWindow_DX11( HWND inHwnd, const RECT & rect, const float nearPlane, const float farPlane );
												~kbRenderWindow_DX11();

private:

	virtual void								BeginFrame_Internal() override;
	virtual void								EndFrame_Internal() override;
	virtual void								Release_Internal() override;

	IDXGISwapChain *							m_pSwapChain;
	ID3D11RenderTargetView *					m_pRenderTargetView;

	kbMat4										m_EyeMatrices[2];
};

class kbRenderTexture_DX11 : public kbRenderTexture {

	friend class kbRenderer_DX11;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderTexture_DX11( const int width, const int height, const eTextureFormat targetFormat, const bool bIsCPUAccessible ) :
													kbRenderTexture( width, height, targetFormat, bIsCPUAccessible ),
													m_pRenderTargetTexture( nullptr ),
													m_pRenderTargetView( nullptr ),
													m_pShaderResourceView( nullptr ),
													m_pDepthStencilView( nullptr ) { }
   

private:

	virtual void								Release_Internal() {
													SAFE_RELEASE( m_pShaderResourceView );
													SAFE_RELEASE( m_pRenderTargetView );
													SAFE_RELEASE( m_pRenderTargetTexture );
													SAFE_RELEASE( m_pDepthStencilView );
												}

	ID3D11Texture2D *							m_pRenderTargetTexture;
	ID3D11RenderTargetView *					m_pRenderTargetView;
	ID3D11ShaderResourceView *					m_pShaderResourceView;
	ID3D11DepthStencilView	*					m_pDepthStencilView;
};

/**
 *	kbGPUTimeStamp
 */
class kbGPUTimeStamp {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	//
	static void									Init( ID3D11DeviceContext *const DeviceContext );
	static void									Shutdown();

	static void									BeginFrame( ID3D11DeviceContext *const DeviceContext );
	static void									EndFrame( ID3D11DeviceContext *const DeviceContext );
	static void									UpdateFrameNum();
	static void									PlaceTimeStamp( const kbString & timeStampName, ID3D11DeviceContext *const pDeviceContext );

	static int									GetNumTimeStamps() { return (int)m_TimeStampsThisFrame.size(); }
	static const kbString &						GetTimeStampName( const int idx ) { return m_TimeStampsThisFrame[idx]->m_Name; }
	static float								GetTimeStampMS( const int idx ) { return m_TimeStampsThisFrame[idx]->m_TimeMS; }

private:

	struct GPUTimeStamp_t {
		kbString								m_Name;
		ID3D11Query *							m_pQueries[2];
		float									m_TimeMS;
	};

	static int									m_TimeStampFrameNum;
	static ID3D11Query *						m_pDisjointTimeStamps[2];

	static const int							MaxTimeStamps = 128;
	static GPUTimeStamp_t						m_TimeStamps[MaxTimeStamps];
	static int									m_NumTimeStamps;
	static std::map<kbString, GPUTimeStamp_t *> m_TimeStampMap;
	static std::vector<GPUTimeStamp_t *>		m_TimeStampsThisFrame;
	static bool									m_bActiveThisFrame;
};

#define PLACE_GPU_TIME_STAMP(name) { \
	static kbString timeStampName(name); \
	kbGPUTimeStamp::PlaceTimeStamp( timeStampName, m_pDeviceContext ); \
}

/**
 *	eventMarker_t
 */
struct eventMarker_t {
	eventMarker_t( const wchar_t *const name, struct ID3DUserDefinedAnnotation *const pEventMarker );
	~eventMarker_t();

	ID3DUserDefinedAnnotation * m_pEventMarker;
};

#define START_SCOPED_RENDER_TIMER(index) kbScopedTimer a##index(index); eventMarker_t sm##index( L#index, m_pEventMarker );

/**
 *	kbRenderState
 */

D3D11_COLOR_WRITE_ENABLE & operator |= ( D3D11_COLOR_WRITE_ENABLE & lhs, const D3D11_COLOR_WRITE_ENABLE rhs );

struct kbRenderState {

	enum kbDepthWriteMask {
		DepthWriteMaskAll,
		DepthWriteMaskZero,
	};

	enum kbDepthStencilCompareTest {
		CompareLess,
		CompareNotEqual,
		CompareAlways,
	};

	enum kbStencilOp {
		StencilKeep,
		StencilReplace,
	};

	kbRenderState() : 
		m_pCurrentDepthStencilState( nullptr ), 
		m_pAlphaBlendState( nullptr ), 
		m_pDevice( nullptr ),
		m_pDeviceContext( nullptr ) { }

	void SetDeviceAndContext( ID3D11Device * const pDevice, ID3D11DeviceContext *const pDeviceContext ) {
		m_pDevice = pDevice;
		m_pDeviceContext = pDeviceContext;
	}

	void Shutdown() {
		SAFE_RELEASE( m_pCurrentDepthStencilState );
		SAFE_RELEASE( m_pAlphaBlendState );
		m_pDeviceContext = nullptr;
		m_pDevice = nullptr;
	}

	D3D11_DEPTH_WRITE_MASK GetD3DDepthWriteMask( const kbDepthWriteMask inDepthWriteMask ) const {
		switch ( inDepthWriteMask ) {
			case DepthWriteMaskAll : return D3D11_DEPTH_WRITE_MASK_ALL;
			case DepthWriteMaskZero : return D3D11_DEPTH_WRITE_MASK_ZERO;
		}

		return D3D11_DEPTH_WRITE_MASK_ALL;
	}

	D3D11_COMPARISON_FUNC GetDepthStencilCompareTest( const kbDepthStencilCompareTest inDepthStencilCompareTest ) const {
		switch( inDepthStencilCompareTest ) {
			case CompareLess : return D3D11_COMPARISON_LESS;
			case CompareNotEqual : return D3D11_COMPARISON_NOT_EQUAL;
			case CompareAlways : return D3D11_COMPARISON_ALWAYS;
		}

		return D3D11_COMPARISON_NOT_EQUAL;
	}

	D3D11_STENCIL_OP GetStencilOp( const kbStencilOp inStencilOp ) const {
		switch( inStencilOp ) {
			case StencilKeep : return D3D11_STENCIL_OP_KEEP;
			case StencilReplace : return D3D11_STENCIL_OP_REPLACE;
		}

		return D3D11_STENCIL_OP_KEEP;
	}

	void SetDepthStencilState( const bool bDepthEnable = true,
							   const kbDepthWriteMask depthWriteMask = DepthWriteMaskAll,
							   const kbDepthStencilCompareTest depthComparisonTest = CompareLess,
							   const bool bStencilEnable = true,
							   const UINT8 stencilReadMask = 0xff,
							   const UINT8 stencilWriteMask = 0,
							   const kbStencilOp frontFaceStencilFailOp = StencilKeep,
							   const kbStencilOp frontFaceStencilDepthFailOp = StencilKeep,
							   const kbStencilOp frontFaceStencilPassOp = StencilKeep,
							   const kbDepthStencilCompareTest frontFaceComparisonTest = CompareNotEqual,
							   const kbStencilOp backFaceStencilFailOp = StencilKeep,
							   const kbStencilOp backFaceStencilDepthFailOp = StencilKeep,
							   const kbStencilOp backFaceStencilPassOp = StencilKeep,
							   const kbDepthStencilCompareTest backFaceComparisonTest = CompareNotEqual,
							   const UINT stencilRef = 1 ) {

		SAFE_RELEASE( m_pCurrentDepthStencilState );
		
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = { 0 };
		depthStencilDesc.DepthEnable = bDepthEnable;
		depthStencilDesc.DepthWriteMask = GetD3DDepthWriteMask( depthWriteMask );
		depthStencilDesc.DepthFunc = GetDepthStencilCompareTest( depthComparisonTest );
		depthStencilDesc.StencilEnable = bStencilEnable;
		depthStencilDesc.StencilReadMask = stencilReadMask;
		depthStencilDesc.StencilWriteMask = stencilWriteMask;
		depthStencilDesc.FrontFace.StencilFailOp = GetStencilOp( frontFaceStencilFailOp );
		depthStencilDesc.FrontFace.StencilDepthFailOp = GetStencilOp( frontFaceStencilDepthFailOp );
		depthStencilDesc.FrontFace.StencilPassOp = GetStencilOp( frontFaceStencilPassOp );
		depthStencilDesc.FrontFace.StencilFunc = GetDepthStencilCompareTest( frontFaceComparisonTest );
		depthStencilDesc.BackFace.StencilFailOp = GetStencilOp( backFaceStencilFailOp );
		depthStencilDesc.BackFace.StencilDepthFailOp = GetStencilOp( backFaceStencilDepthFailOp );
		depthStencilDesc.BackFace.StencilPassOp = GetStencilOp( backFaceStencilPassOp );
		depthStencilDesc.BackFace.StencilFunc = GetDepthStencilCompareTest( backFaceComparisonTest );;

		HRESULT hr = m_pDevice->CreateDepthStencilState( &depthStencilDesc, &m_pCurrentDepthStencilState );
		blk::error_check( SUCCEEDED(hr), "kbDepthStencilState::SetDepthStencilState() Failed" );

		m_pDeviceContext->OMSetDepthStencilState( m_pCurrentDepthStencilState, stencilRef );
	}

	ID3D11DepthStencilState * m_pCurrentDepthStencilState;


	D3D11_BLEND GetD3DBlend( const kbBlend blend ) {
		switch( blend ) {
			case Blend_Zero : return D3D11_BLEND_ZERO;
			case Blend_One : return D3D11_BLEND_ONE;
			case Blend_SrcColor : return D3D11_BLEND_SRC_COLOR;
			case Blend_InvSrcColor : return D3D11_BLEND_INV_SRC_COLOR;
			case Blend_SrcAlpha : return D3D11_BLEND_SRC_ALPHA;
			case Blend_InvSrcAlpha : return D3D11_BLEND_INV_SRC_ALPHA;
			case Blend_DstAlpha : return D3D11_BLEND_DEST_ALPHA;
			case Blend_InvDstAlpha : return D3D11_BLEND_INV_DEST_ALPHA;
			case Blend_DstColor : return D3D11_BLEND_DEST_COLOR;
			case Blend_InvDstColor : return D3D11_BLEND_INV_DEST_COLOR;
		};

		return D3D11_BLEND_ONE;
	}

	D3D11_BLEND_OP GetD3DBlendOp( const kbBlendOp blendOp ) {
		switch( blendOp ) {
			case BlendOp_Add : return D3D11_BLEND_OP_ADD;
			case BlendOp_Subtract : return D3D11_BLEND_OP_SUBTRACT;
			case BlendOp_Max : return D3D11_BLEND_OP_MAX;
			case BlendOp_Min : return D3D11_BLEND_OP_MIN;
		}

		return D3D11_BLEND_OP_ADD;
	}

	D3D11_COLOR_WRITE_ENABLE GetD3DColorWriteEnable( const kbColorWriteEnable colorWriteEnable ) {

		D3D11_COLOR_WRITE_ENABLE retVal = (D3D11_COLOR_WRITE_ENABLE)0;
		if ( colorWriteEnable & ColorWriteEnable_Red ) {
			retVal |= D3D11_COLOR_WRITE_ENABLE_RED;
		}

		if ( colorWriteEnable & ColorWriteEnable_Green ) {
			retVal |= D3D11_COLOR_WRITE_ENABLE_GREEN;
		}

		if ( colorWriteEnable & ColorWriteEnable_Blue ) {
			retVal |= D3D11_COLOR_WRITE_ENABLE_BLUE;
		}

		if ( colorWriteEnable & ColorWriteEnable_Alpha) {
			retVal |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
		}

		if ( retVal == (D3D11_COLOR_WRITE_ENABLE)0 ) {
			return D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		return retVal;
	}

	void SetBlendState( const kbShader *const pShader ) {
		SetBlendState(	false,
						false,
						pShader->IsBlendEnabled(),
						pShader->GetSrcBlend(),
						pShader->GetDstBlend(),
						pShader->GetBlendOp(),
						pShader->GetSrcBlendAlpha(),
						pShader->GetDstBlendAlpha(),
						pShader->GetBlendOpAlpha(),
						pShader->GetColorWriteEnable() );
	}

	void SetBlendState( const bool bAlphaToCoverageEnable = false,
						const bool bIndependentBlendEnabled = false,
						const bool bBlendEnable = false,
						const kbBlend sourceBlend = Blend_One,
						const kbBlend destBlend = Blend_One,
						const kbBlendOp blendOp = BlendOp_Add,
						const kbBlend sourceAlpha = Blend_One,
						const kbBlend destAlpha = Blend_One,
						const kbBlendOp alphaBlendOp = BlendOp_Add,
						const kbColorWriteEnable renderTargetWriteMask = ColorWriteEnable_All,
						const UINT sampleMask = 0xffffffff ) {

		SAFE_RELEASE( m_pAlphaBlendState );

	/*	if ( bBlendEnable == false ) {
			m_pDeviceContext->OMSetBlendState( nullptr, nullptr, sampleMask );
			return;
		}*/

		D3D11_BLEND_DESC BlendStateDesc = { 0 };
		BlendStateDesc.AlphaToCoverageEnable = bAlphaToCoverageEnable;
		BlendStateDesc.IndependentBlendEnable = bIndependentBlendEnabled;
		BlendStateDesc.RenderTarget[0].BlendEnable = bBlendEnable;
		BlendStateDesc.RenderTarget[0].SrcBlend = GetD3DBlend( sourceBlend );
		BlendStateDesc.RenderTarget[0].DestBlend = GetD3DBlend( destBlend );
		BlendStateDesc.RenderTarget[0].BlendOp = GetD3DBlendOp( blendOp );
		BlendStateDesc.RenderTarget[0].SrcBlendAlpha = GetD3DBlend( sourceAlpha );
		BlendStateDesc.RenderTarget[0].DestBlendAlpha = GetD3DBlend( destAlpha );
		BlendStateDesc.RenderTarget[0].BlendOpAlpha = GetD3DBlendOp( alphaBlendOp );
		BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = GetD3DColorWriteEnable( renderTargetWriteMask );

		HRESULT hr = m_pDevice->CreateBlendState( &BlendStateDesc, &m_pAlphaBlendState );
		blk::error_check( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create additive blend state" );

		m_pDeviceContext->OMSetBlendState( m_pAlphaBlendState, nullptr, sampleMask );
	}

	ID3D11BlendState * m_pAlphaBlendState;

	ID3D11DeviceContext * m_pDeviceContext;
	ID3D11Device * m_pDevice;
};

/**
 *	kbRenderer_DX11
 */
class kbRenderer_DX11 : public kbRenderer {

	friend class kbRenderJob;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

												kbRenderer_DX11();
												~kbRenderer_DX11();

	virtual int									CreateRenderView( HWND hwnd ) override;
	virtual void								SetRenderWindow( HWND hwnd ) override;

	void										LoadShader( const std::string & fileName, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader,
															ID3D11PixelShader *& pixelShader, ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, 
															const std::string & pixelShaderFunc, struct kbShaderVarBindings_t * pShaderBindings = nullptr );

	void										CreateShaderFromText( const std::string & fileName, const std::string & shaderText, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader,
															ID3D11PixelShader *& pixelShader, ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, 
															const std::string & pixelShaderFunc, struct kbShaderVarBindings_t * pShaderBindings = nullptr );

	virtual kbVec2i								GetEntityIdAtScreenPosition( const uint x, const uint y ) override;

	virtual void								SetGlobalShaderParam( const kbShaderParamOverrides_t::kbShaderParam_t & shaderParam ) override;
	virtual void								SetGlobalShaderParam( const kbShaderParamOverrides_t & shaderParam ) override;

	// Render thread
	virtual void								RT_SetRenderTarget( kbRenderTexture *const pRenderTexture ) override;
	virtual void								RT_ClearRenderTarget( kbRenderTexture *const pRenderTexture, const kbColor & color ) override;
	virtual void								RT_RenderMesh( const kbModel *const pModel, kbShader * pShader, const kbShaderParamOverrides_t *const pShaderParams ) override;
	virtual void								RT_Render2DLine( const kbVec3 & startPt, const kbVec3 & endPt, const kbColor & color, const float width, const kbShader * pShader, const struct kbShaderParamOverrides_t *const ShaderBindings = nullptr ) override;
	virtual void								RT_Render2DQuad( const kbVec2 & origin, const kbVec2 & size, const kbColor & color, const kbShader * pShader, const struct kbShaderParamOverrides_t *const ShaderBindings = nullptr );
	virtual void								RT_CopyRenderTarget( kbRenderTexture *const pSrcTexture, kbRenderTexture *const pDstTexture ) override;
	virtual kbRenderTargetMap					RT_MapRenderTarget( kbRenderTexture *const pDstTexture ) override;
	virtual void								RT_UnmapRenderTarget( kbRenderTexture *const pDstTexture ) override;

private:

	virtual void								Init_Internal( HWND, const int width, const int height ) override;
	virtual bool								LoadTexture_Internal( const char * name, int index, int width = -1, int height = -1 ) override;

	virtual kbRenderTexture *					GetRenderTexture_Internal( const int width, const int height, const eTextureFormat texFormat, const bool bIsCPUAccessible ) override;
	virtual void								ReturnRenderTexture_Internal( const kbRenderTexture *const ) override;

	virtual void								RenderSync_Internal() override;

	virtual void								Shutdown_Internal() override;

	void										ReadShaderFile( std::string & shaderText, kbShaderVarBindings_t *const pShaderBindings );

	void										SetRenderTarget( eReservedRenderTargets type );

	virtual void								RenderScene() override;

	void										PreRenderCullAndSort();

	void										RenderMesh( const kbRenderSubmesh *const pRenderMesh, const bool bShadowPass = false, const bool bSkipMeshBlendSettings = false );

	void										RenderScreenSpaceQuads();
	void										RenderScreenSpaceQuadImmediate( const int start_x, const int start_y, const int size_x, const int size_y, 
																				const int textureIndex, kbShader * pShader = nullptr, kbShaderParamOverrides_t * pParamOverrides = nullptr );
	void										RenderDebugLines();
	void										RenderPretransformedDebugLines();
	void										RenderDebugBillboards( const bool bIsEntityIdPass );
	void										RenderPostProcess();
	void										RenderBloom();
	void										RenderSSAO();
	void										RenderConsole();
	void										RenderLights();
	void										RenderLight( const kbRenderLight *const );
	void										RenderShadow( const kbRenderLight *const, kbMat4 splitMatrices[] );
	void										RenderLightShafts();
	void										RenderTranslucency();
	void										RenderDebugText();
	void										RenderMousePickerIds();
	void										Blit( kbRenderTexture *const src, kbRenderTexture *const dest );
	ID3D11Buffer *								SetConstantBuffer( const kbShaderVarBindings_t& shaderBindings, const kbShaderParamOverrides_t* shaderParamOverrides, const kbRenderObject* const pRenderObject, byte* const pInMappedBufferData, const char* const pShaderName = nullptr );

	ID3D11Buffer *								GetConstantBuffer( const size_t requestSize );
	void										SetShaderMat4( const std::string & varName, const kbMat4 & inMatrix, void *const pBuffer, const kbShaderVarBindings_t & binding );
	void										SetShaderVec4( const std::string & varName, const kbVec4 & inVec, void *const pBuffer, const kbShaderVarBindings_t & binding );
	void										SetShaderMat4Array( const std::string & varName, const kbMat4 *const mat4Array, const int arrayLen, void *const pBuffer, const kbShaderVarBindings_t & binding );
	void										SetShaderVec4Array( const std::string & varName, const kbVec4 *const vec4Array, const int arrayLen, void *const pBuffer, const kbShaderVarBindings_t & binding );
	void										SetShaderFloat( const std::string & varName, const float inFloat, void *const pBuffer, const kbShaderVarBindings_t & binding );
	void										SetShaderInt( const std::string & varName, const int inInt, void *const pBuffer, const kbShaderVarBindings_t & binding );
	int											GetVarBindingIndex( const std::string & varName, const kbShaderVarBindings_t & binding );

	void										DrawTexture( ID3D11ShaderResourceView *const pShaderResourceView, const kbVec3 & pixelPosition, 
															 const kbVec3 & pixelSize, const kbVec3 & renderTargetSize );

	kbRenderTexture_DX11 *						GetRenderTarget_DX11( const eReservedRenderTargets target ) { return (kbRenderTexture_DX11*) m_pRenderTargets[target]; }
	kbRenderTexture_DX11 *						GetAccumBuffer( const int index ) { return  (kbRenderTexture_DX11*) m_pAccumBuffers[index]; }

	HWND										m_hwnd;
	IDXGIFactory *								m_pDXGIFactory;
	ID3D11Device *								m_pD3DDevice;
	ID3D11DeviceContext *						m_pDeviceContext;
	ID3D11Texture2D *							m_pDepthStencilBuffer;

	kbRenderState								m_RenderState;

	ID3D11DepthStencilView *					m_pDepthStencilView;

	ID3D11Texture2D *							m_pOffScreenRenderTargetTexture;

	kbShader *									m_pOpaqueQuadShader;
	kbShader *									m_pTranslucentShader;
	kbShader *									m_pScreenTintShader;
	kbShader *									m_pMultiplyBlendShader;
	kbShader *									m_pBasicShader;
	kbShader *									m_pDebugShader;
	kbShader *									m_pMissingShader;
	kbShader *									m_pUberPostProcess;
	kbShader *									m_pDirectionalLightShader;
	kbShader *									m_pPointLightShader;
	kbShader *									m_pCylindricalLightShader;
	kbShader *									m_pBasicParticleShader;
	kbShader *									m_pDirectionalLightShadowShader;
	kbShader *									m_pLightShaftsShader;
	kbShader *									m_pSimpleAdditiveShader;
	kbShader *									m_pGodRayIterationShader;
	kbShader *									m_pMousePickerIdShader;
	kbShader *									m_pSSAO;
	kbShader *									m_pBasicFont;

	// Non-resource managed shaders (Game assets cannot reference these).  These have to be manually released
	kbShader *									m_pSkinnedDirectionalLightShadowShader;
	kbShader *									m_pBloomGatherShader;
	kbShader *									m_pBloomBlur;

	ID3D11Buffer *								m_pUnitQuad;
	ID3D11Buffer *								m_pConsoleQuad;

	std::map<size_t, ID3D11Buffer*>				m_ConstantBuffers;		// Maps constant buffers to their byte width

	ID3D11RasterizerState *						m_pDefaultRasterizerState;
	ID3D11RasterizerState *						m_pFrontFaceCullingRasterizerState;
	ID3D11RasterizerState *						m_pNoFaceCullingRasterizerState;
	ID3D11RasterizerState *						m_pWireFrameRasterizerState;

	const static int Max_Num_Textures = 128;
	kbTexture *									m_pTextures[Max_Num_Textures];
	
	ID3D11SamplerState *						m_pBasicSamplerState;
	ID3D11SamplerState *						m_pNormalMapSamplerState;
	ID3D11SamplerState *						m_pShadowMapSamplerState;
	
	// Shader Params
	std::vector<kbShaderParamOverrides_t::kbShaderParam_t>	m_GlobalShaderParams_GameThread;
	std::vector<kbShaderParamOverrides_t::kbShaderParam_t>	m_GlobalShaderParams_RenderThread;

	// debug
	ID3DUserDefinedAnnotation *					m_pEventMarker;
	ID3D11Buffer *								m_DebugVertexBuffer;
	ID3D11Buffer *								m_DebugPreTransformedVertexBuffer;

	kbModel	*									m_DebugText;
	int											m_FrameNum;
};

extern ID3D11Device * g_pD3DDevice;
extern kbRenderer_DX11 * g_pD3D11Renderer;

XMMATRIX & XMMATRIXFromkbMat4( kbMat4 & matrix );
kbMat4 & kbMat4FromXMMATRIX( FXMMATRIX & matrix );

#endif