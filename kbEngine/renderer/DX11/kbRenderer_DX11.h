//==============================================================================
// kbRenderer_DX11.h
//
// Renderer implementation using DX11 API
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#ifndef _KBRENDERER_DX11_H_
#define _KBRENDERER_DX11_H_

#include <D3D11.h>
#include <DirectXMath.h>
#include "kbRenderer.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_D3D.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbRenderer_defs.h"
#include "kbMaterial.h"
#include "kbJobManager.h"

using namespace DirectX;

class kbModel;
class kbComponent;
class kbLightComponent;
class kbLightShaftsComponent;

extern XMFLOAT4X4 & XMFLOAT4X4FromkbMat4( kbMat4 & matrix );
extern kbMat4 & kbMat4FromXMFLOAT4X4( XMFLOAT4X4 & matrix );



/**
 *	kbRenderTexture
 */
enum eRenderTargetTexture {
	COLOR_BUFFER,		// Color in xyz.  Pixel Depth in W
	NORMAL_BUFFER,		// Normal in xyz. W currently unused
	SPECULAR_BUFFER,
	DEPTH_BUFFER,
	ACCUMULATION_BUFFER,
	SHADOW_BUFFER,
	SHADOW_BUFFER_DEPTH,
	DOWN_RES_BUFFER,
	DOWN_RES_BUFFER_2,
	SCRATCH_BUFFER,
	MOUSE_PICKER_BUFFER,
	NUM_RENDER_TARGETS,
};

class kbRenderTexture {

	friend class kbRenderer_DX11;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderTexture() :
													m_pRenderTargetTexture( nullptr ),
													m_pRenderTargetView( nullptr ),
													m_pShaderResourceView( nullptr ),
													m_pDepthStencilView( nullptr ),
													m_Width( 0 ),
													m_Height( 0 ),
													m_bIsDirty( true ) { }
   
												void Release() {
													SAFE_RELEASE( m_pShaderResourceView );
													SAFE_RELEASE( m_pRenderTargetView );
													SAFE_RELEASE( m_pRenderTargetTexture );
													SAFE_RELEASE( m_pDepthStencilView );
												}

private:
	ID3D11Texture2D *							m_pRenderTargetTexture;
	ID3D11RenderTargetView *					m_pRenderTargetView;
	ID3D11ShaderResourceView *					m_pShaderResourceView;
	ID3D11DepthStencilView	*					m_pDepthStencilView;
	int											m_Width;
	int											m_Height;
	bool										m_bIsDirty;
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
		if ( FAILED( hr ) ) {
			kbError( "kbDepthStencilState::SetDepthStencilState()" );
		}

		m_pDeviceContext->OMSetDepthStencilState( m_pCurrentDepthStencilState, stencilRef );
	}

	ID3D11DepthStencilState * m_pCurrentDepthStencilState;

	enum kbBlend {
		BF_One,
		BF_Zero,
		BF_SourceAlpha,
		BF_InvSourceAlpha,
	};

	enum kbBlendOp {
		BO_Add
	};

	enum kbColorWriteEnable {
		CW_All
	};

	D3D11_BLEND GetD3DBlend( const kbBlend blend ) {
		switch( blend ) {
			case BF_Zero : return D3D11_BLEND_ZERO;
			case BF_One : return D3D11_BLEND_ONE;
			case BF_SourceAlpha : return D3D11_BLEND_SRC_ALPHA;
			case BF_InvSourceAlpha : return D3D11_BLEND_INV_SRC_ALPHA;
		};

		return D3D11_BLEND_ONE;
	}

	D3D11_BLEND_OP GetD3DBlendOp( const kbBlendOp blendOp ) {
		switch( blendOp ) {
			case BO_Add : return D3D11_BLEND_OP_ADD;
		}

		return D3D11_BLEND_OP_ADD;
	}

	D3D11_COLOR_WRITE_ENABLE GetD3DColorWriteEnable( const kbColorWriteEnable colorWriteEnable ) {
		switch( colorWriteEnable ) {
			case CW_All : return D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		return D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	void SetBlendState( const bool bAlphaToCoverageEnable = false,
						const bool bIndependentBlendEnabled = false,
						const bool bBlendEnable = false,
						const kbBlend sourceBlend = BF_One,
						const kbBlend destBlend = BF_One,
						const kbBlendOp blendOp = BO_Add,
						const kbBlend sourceAlpha = BF_One,
						const kbBlend destAlpha = BF_One,
						const kbBlendOp alphaBlendOp = BO_Add,
						const kbColorWriteEnable renderTargetWriteMask = CW_All,
						const UINT sampleMask = 0xffffffff ) {

		SAFE_RELEASE( m_pAlphaBlendState );

		if ( bBlendEnable == false ) {
			m_pDeviceContext->OMSetBlendState( nullptr, nullptr, sampleMask );
			return;
		}

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
		BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = m_pDevice->CreateBlendState( &BlendStateDesc, &m_pAlphaBlendState );
		kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create additive blend state" );

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


	// View Transform
	virtual void								SetRenderViewTransform( const HWND hwnd, const kbVec3 & position, const kbQuat & rotation ) override;
	virtual void								GetRenderViewTransform( const HWND hwnd, kbVec3 & position, kbQuat & rotation ) override;



	// Post-process
	void										SetPostProcessSettings( const kbPostProcessSettings_t & postProcessSettings );

	void										LoadShader( const std::string & fileName, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader,
															ID3D11PixelShader *& pixelShader, ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, 
															const std::string & pixelShaderFunc, struct kbShaderVarBindings_t * ShaderBindings = nullptr );



	// Oculus
	bool										IsRenderingToHMD() const { return m_bRenderToHMD; }
	bool										IsUsingHMDTrackingOnly() const { return m_bUsingHMDTrackingOnly; }
	int											GetFrameNum() const { return m_FrameNum; }
	const ovrPosef *							GetOvrEyePose() const { return m_EyeRenderPose; }
	const kbMat4 *								GetEyeMatrices() const { return this->m_RenderWindowList[0]->m_EyeMatrices; }


	virtual kbVec2i								GetEntityIdAtScreenPosition( const uint x, const uint y ) override;

	const static float							Near_Plane;
	const static float							Far_Plane;

private:

	virtual void								Init_Internal( HWND, const int width, const int height, const bool bUseHMD, const bool bUseHMDTrackingOnly ) override;
	virtual bool								LoadTexture_Internal( const char * name, int index, int width = -1, int height = -1 ) override;
	virtual void								RenderSync_Internal() override;

	void										Shutdown();

	bool										InitializeOculus();

	void										CreateRenderTarget( const eRenderTargetTexture targetIndex, const int width, const int height, const DXGI_FORMAT format );
	void										SetRenderTarget( eRenderTargetTexture type );

	virtual void								RenderScene() override;

	void										PreRenderCullAndSort();

	void										RenderModel( const kbRenderObject *const pRenderObject, const ERenderPass renderPass, const bool bShadowPass = false );

	void										RenderScreenSpaceQuads();
	void										RenderScreenSpaceQuadImmediate( const int start_x, const int start_y, const int size_x, const int size_y, 
																				const int textureIndex, kbShader * pShader = nullptr );
	void										RenderDebugLines();
	void										RenderPretransformedDebugLines();
	void										RenderDebugBillboards( const bool bIsEntityIdPass );
	void										RenderPostProcess();
	void										RenderBloom();
	void										RenderConsole();
	void										RenderLights();
	void										RenderLight( const kbRenderLight *const );
	void										RenderShadow( const kbRenderLight *const, kbMat4 splitMatrices[] );
	void										RenderLightShafts();
	void										RenderTranslucency();
	void										RenderDebugText();
	void										RenderMousePickerIds();
	void										Blit( kbRenderTexture *const src, kbRenderTexture *const dest );

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
	kbShader *									m_pBasicShader;
	kbShader *									m_pDebugShader;
	kbShader *									m_pMissingShader;
	kbShader *									m_pUberPostProcess;
	kbShader *									m_pDirectionalLightShader;
	kbShader *									m_pPointLightShader;
	kbShader *									m_pCylindricalLightShader;
	kbShader *									m_pBasicParticleShader;
	kbShader *									m_pBasicSkinnedTextureShader;
	kbShader *									m_pDirectionalLightShadowShader;
	kbShader *									m_pLightShaftsShader;
	kbShader *									m_pSimpleAdditiveShader;
	kbShader *									m_pMousePickerIdShader;

	// Non-resource managed shaders (Game assets cannot reference these).  These have to be manually released
	kbShader *									m_pSkinnedDirectionalLightShadowShader;
	kbShader *									m_pBloomGatherShader;
	kbShader *									m_pBloomBlur;

	ID3D11Buffer *								m_pUnitQuad;
	ID3D11Buffer *								m_pConsoleQuad;

	std::map<size_t, ID3D11Buffer*>				m_ConstantBuffers;		// Maps constant buffers to their byte width

	ID3D11RasterizerState *						m_pDefaultRasterizerState;
	ID3D11RasterizerState *						m_pNoFaceCullingRasterizerState;
	ID3D11RasterizerState *						m_pWireFrameRasterizerState;

	const static int Max_Num_Textures = 128;
	kbTexture *									m_pTextures[Max_Num_Textures];
	
	ID3D11SamplerState *						m_pBasicSamplerState;
	ID3D11SamplerState *						m_pNormalMapSamplerState;
	ID3D11SamplerState *						m_pShadowMapSamplerState;
	
	kbRenderTexture								m_RenderTargets[NUM_RENDER_TARGETS];
	
	// debug
	ID3DUserDefinedAnnotation *					m_pEventMarker;
	ID3D11Buffer *								m_DebugVertexBuffer;
	ID3D11Buffer *								m_DebugPreTransformedVertexBuffer;

	// Oculus Rift
	ovrSession									m_ovrSession;
	int											m_HMDPass;
	ovrRecti									m_EyeRenderViewport[2];
	ovrEyeRenderDesc							m_EyeRenderDesc[2];
	ovrPosef									m_EyeRenderPose[2];
	class kbOculusTexture *						m_OculusTexture[2];
    ovrMirrorTexture							m_MirrorTexture;
	ovrHmdDesc									m_HMDDesc;
	double										m_SensorSampleTime;
	bool										m_bRenderToHMD;
	bool										m_bUsingHMDTrackingOnly;
	// End of Oculus Rift

	kbModel	*									m_DebugText;
	int											m_FrameNum;
};

extern ID3D11Device * g_pD3DDevice;



inline ovrVector3f kbVec3ToovrVec( const kbVec3 & inVec ) {
	ovrVector3f returnVec;
	memcpy( &returnVec, &inVec, sizeof( ovrVector3f ) );
	return returnVec;
}

inline kbVec3 ovrVecTokbVec3( const ovrVector3f & inVec ) {
	kbVec3 returnVec;
	memcpy( &returnVec, &inVec, sizeof( ovrVector3f ) );
	return returnVec;
}

inline ovrQuatf kbQuatToovrQuat( const kbQuat & inQuat ) {
	ovrQuatf returnQuat;
	memcpy( &returnQuat, &inQuat, sizeof( returnQuat ) );
	return returnQuat;
}

inline kbQuat ovrQuatTokbQuat( const ovrQuatf & inQuat ) {
	kbQuat returnQuat;
	memcpy( &returnQuat, &inQuat, sizeof( returnQuat ) );
	return returnQuat;
}

extern kbRenderer_DX11 * g_pD3D11Renderer;

#endif