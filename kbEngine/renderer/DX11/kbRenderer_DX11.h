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

extern class kbRenderer_DX11 * g_pRenderer;
extern XMFLOAT4X4 & XMFLOAT4X4FromkbMat4( kbMat4 & matrix );
extern kbMat4 & kbMat4FromXMFLOAT4X4( XMFLOAT4X4 & matrix );


/**
 *	kbRenderWindow
 */
class kbRenderWindow {

	friend class kbRenderer_DX11;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

																kbRenderWindow();
																~kbRenderWindow();

	void														Release() { SAFE_RELEASE( m_pSwapChain ); SAFE_RELEASE( m_pRenderTargetView ) ; }
	
	void														BeginFrame();
	void														EndFrame() { m_pSwapChain->Present( 0, 0 ); }

private:
	HWND														m_Hwnd;
	IDXGISwapChain *											m_pSwapChain;
	ID3D11RenderTargetView *									m_pRenderTargetView;

	unsigned int												m_ViewPixelWidth;
	unsigned int												m_ViewPixelHeight;
	float														m_fViewPixelWidth;
	float														m_fViewPixelHeight;
	float														m_fViewPixelHalfWidth;
	float														m_fViewPixelHalfHeight;

	kbMat4														m_ProjectionMatrix;
	kbMat4														m_InverseProjectionMatrix;
	kbMat4														m_ViewMatrix;
	kbMat4														m_ViewProjectionMatrix;
	kbMat4														m_InverseViewProjectionMatrix;
	kbVec3														m_CameraPosition;
	kbQuat														m_CameraRotation;

	kbMat4														m_EyeMatrices[2];

	kbVec3														m_CameraPosition_GameThread;
	kbQuat														m_CameraRotation_GameThread;

	std::map<const kbComponent *, kbRenderObject *>				m_RenderObjectMap;
	std::map<const kbLightComponent *, kbRenderLight *>			m_RenderLightMap;
	std::map<const void *, kbRenderObject *>					m_RenderParticleMap;
};

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

struct kbPostProcessSettings_t {
												kbPostProcessSettings_t() :
													m_AdditiveColor( kbVec3::zero ),
													m_Tint( 1.0f, 1.0f, 1.0f, 1.0f ) { }

	kbVec4										m_AdditiveColor;
	kbVec4										m_Tint;
};

/**
 *	kbRenderer_DX11
 */
class kbRenderer_DX11 {

	friend class kbRenderJob;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

												kbRenderer_DX11();
												~kbRenderer_DX11();
	
	void										Init( HWND, const int width, const int height, const bool bUseHMD, const bool bUseHMDTrackingOnly );

	int											CreateRenderView( HWND hwnd );
	void										SetRenderWindow( HWND hwnd );

	// Render Syncing
	void										RenderSync();
	void										SetReadyToRender();
	void										WaitForRenderingToComplete() const { while ( m_RenderThreadSync == 1 ) { } };
	bool										IsRenderingSynced() const { return m_RenderThreadSync == 0; }

	// View Transform
	void										SetRenderViewTransform( const HWND hwnd, const kbVec3 & position, const kbQuat & rotation );
	void										GetRenderViewTransform( const HWND hwnd, kbVec3 & position, kbQuat & rotation );

	// Models
	void										AddRenderObject( const kbComponent *const, const kbModel *const model, const kbVec3 & pos, const kbQuat & orientation, 
																 const kbVec3 & scale, const ERenderPass RenderPass = RP_Lighting, 
																 const std::vector<kbShader *> *const pShaderOverrideList = nullptr, 
																 const kbShaderParamOverrides_t *const pShaderParamsOverride = nullptr );

	void										UpdateRenderObject( const kbComponent *const , const kbModel *const model, const kbVec3 & pos, const kbQuat & orientation, 
																	const kbVec3 & scale, const ERenderPass RenderPass = RP_Lighting,
																	const std::vector<kbShader *> *const pShaderOverrideList = nullptr, 
																	const kbShaderParamOverrides_t *const pShaderParamsOverride = nullptr );

	void										RemoveRenderObject( const kbComponent *const );
	
	// Lights
	void										AddLight( const kbLightComponent *const pLightComponent, const kbVec3 & pos, const kbQuat & orientation );
	void										UpdateLight( const kbLightComponent * pLightComponent, const kbVec3 & pos, const kbQuat & orientation );
	void										RemoveLight( const kbLightComponent *const pLightComponent );
	void										HackClearLight( const kbLightComponent *const pLightComponent );

	// Fog
	void										UpdateFog( const kbColor & color, const float startDistance, const float endDistance );

	// Particles
	void										AddParticle( const void *const pParentPtr, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation );
	void										UpdateParticle( const void *const pParentPtr, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation );
	void										RemoveParticle( const void *const pParentPtr );

	// Light Shafts
	void										AddLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation );
	void										UpdateLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation );
	void										RemoveLightShafts( const kbLightShaftsComponent *const pComponent );


	// Post-process
	void										SetPostProcessSettings( const kbPostProcessSettings_t & postProcessSettings );

	bool										LoadTexture( const char * name, int index, int width = -1, int height = -1 );
	void										LoadShader( const std::string & fileName, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader,
															ID3D11PixelShader *& pixelShader, ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, 
															const std::string & pixelShaderFunc, struct kbShaderVarBindings_t * ShaderBindings = nullptr );

	// Various Drawing commands
	void										DrawScreenSpaceQuad( const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader *const pShader = nullptr );
	void										DrawLine( const kbVec3 & start, const kbVec3 & end, const kbColor & color );
	void										DrawBox( const kbBounds & bounds, const kbColor & color );
	void										DrawPreTransformedLine( const std::vector<kbVec3> & vertList, const kbColor & color );
	void										DrawSphere( const kbVec3 & origin, const float radius, const int NumSegments, const kbColor & color );
	void										DrawBillboard( const kbVec3 & position, const kbVec2 & size, const int textureIndex, kbShader *const pShader, const int entityId = -1 );
	void										DrawModel( const kbModel * pModel, const kbVec3 & start, const kbQuat & orientation, const kbVec3 & scale, const int entityId );

	//
	enum kbViewMode_t {
		ViewMode_Shaded,
		ViewMode_Wireframe
	};
	void										SetViewMode( const kbViewMode_t newViewMode ) { m_ViewMode_GameThread = newViewMode; }


	// Debug Text Drawing
	void										EnableConsole( const bool bEnable ) { m_bConsoleEnabled = bEnable; }
	void										DrawDebugText( const std::string & theString, const float X, const float Y, const float ScreenCharWidth, 
															   const float ScreenCharHeight, const kbColor & color );

	// Oculus
	bool										IsRenderingToHMD() const { return m_bRenderToHMD; }
	bool										IsUsingHMDTrackingOnly() const { return m_bUsingHMDTrackingOnly; }
	int											GetFrameNum() const { return m_FrameNum; }
	const ovrPosef *							GetOvrEyePose() const { return m_EyeRenderPose; }
	const kbMat4 *								GetEyeMatrices() const { return this->m_RenderWindowList[0]->m_EyeMatrices; }

	// Other
	int											GetBackBufferWidth() const { return Back_Buffer_Width; }
	int											GetBackBufferHeight() const { return Back_Buffer_Height; }

	kbVec2i										GetEntityIdAtScreenPosition( const uint x, const uint y );

	const static float							Near_Plane;
	const static float							Far_Plane;

private:

	void										Shutdown();

	bool										InitializeOculus();

	void										CreateRenderTarget( const eRenderTargetTexture targetIndex, const int width, const int height, const DXGI_FORMAT format );
	void										SetRenderTarget( eRenderTargetTexture type );

	void										RenderScene();

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

	int											Back_Buffer_Width;
	int											Back_Buffer_Height;

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

	kbRenderWindow *							m_pCurrentRenderWindow;    // the render window BeginScene was called with

	const static int Max_Num_Textures = 128;
	kbTexture *									m_pTextures[Max_Num_Textures];
	
	ID3D11SamplerState *						m_pBasicSamplerState;
	ID3D11SamplerState *						m_pNormalMapSamplerState;
	ID3D11SamplerState *						m_pShadowMapSamplerState;

	std::vector<kbRenderWindow *>				m_RenderWindowList;
	
	kbRenderTexture								m_RenderTargets[NUM_RENDER_TARGETS];
	
	// debug
	ID3DUserDefinedAnnotation *					m_pEventMarker;

	std::vector<vertexLayout>					m_DebugLines;
	ID3D11Buffer *								m_DebugVertexBuffer;
	std::vector<vertexLayout>					m_DebugPreTransformedLines;
	ID3D11Buffer *								m_DebugPreTransformedVertexBuffer;
	
	struct debugDrawObject_t {
		kbVec3									m_Position;
		kbQuat									m_Orientation;
		kbVec3									m_Scale;
		const kbModel *							m_pModel;
		kbShader*								m_pShader;
		int										m_TextureIndex;
		int										m_EntityId;
	};
	
	std::vector<debugDrawObject_t>				m_DebugBillboards;
	std::vector<debugDrawObject_t>				m_DebugModels;

	kbViewMode_t								m_ViewMode_GameThread;
	kbViewMode_t								m_ViewMode;

	struct kbTextInfo_t {
		kbTextInfo_t() : 
			color( kbColor::green ) { }

		std::string								TextInfo;
		float									screenX;
		float									screenY;
		float									screenW;
		float									screenH;
		kbColor									color;
	};

	std::vector<kbTextInfo_t>					m_DebugStrings_GameThread;
	std::vector<kbTextInfo_t>					m_DebugStrings;

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

	// Threading
	kbRenderJob *								m_pRenderJob;
	volatile int								m_RenderThreadSync;

	std::vector<vertexLayout>					m_DebugLines_GameThread;
	std::vector<debugDrawObject_t>				m_DebugBillboards_GameThread;
	std::vector<debugDrawObject_t>				m_DebugModels_GameThread;

	struct ScreenSpaceQuad_t {
												ScreenSpaceQuad_t() { }

		kbVec2i									m_Pos;
		kbVec2i									m_Size;
		int										m_TextureIndex;
		class kbShader *						m_pShader;
	};
	std::vector<ScreenSpaceQuad_t>				m_ScreenSpaceQuads_GameThread;
	std::vector<ScreenSpaceQuad_t>				m_ScreenSpaceQuads_RenderThread;

	std::vector<kbRenderObject>					m_RenderObjectList_GameThread;
	std::vector<kbRenderLight>					m_LightList_GameThread;
	std::vector<kbRenderObject>					m_ParticleList_GameThread;

	std::vector<kbLightShafts>					m_LightShafts_GameThread;
	std::vector<kbLightShafts>					m_LightShafts_RenderThread;

	kbColor										m_FogColor_GameThread;
	kbColor										m_FogColor_RenderThread;

	float										m_FogStartDistance_GameThread;
	float										m_FogStartDistance_RenderThread;

	float										m_FogEndDistance_GameThread;
	float										m_FogEndDistance_RenderThread;

	kbPostProcessSettings_t						m_PostProcessSettings_GameThread;
	kbPostProcessSettings_t						m_PostProcessSettings_RenderThread;

	kbModel	*									m_DebugText;
	int											m_FrameNum;
	bool										m_bConsoleEnabled;
};

extern ID3D11Device * g_pD3DDevice;

extern const float g_DebugTextSize;
extern const float g_DebugLineSpacing;


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


#endif