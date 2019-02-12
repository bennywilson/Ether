//==============================================================================
// kbRenderer_DX11.cpp
//
// Renderer implementation using DX11 API
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#include <D3Dcompiler.h>
#include <d3d11_1.h>
#include <dxgicommon.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "kbCore.h"
#include "kbRenderer_DX11.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbConsole.h"

kbColorWriteEnable operator |( const kbColorWriteEnable lhs, const kbColorWriteEnable rhs ) { return (kbColorWriteEnable)((int)lhs | (int)rhs); }
D3D11_COLOR_WRITE_ENABLE & operator |= ( D3D11_COLOR_WRITE_ENABLE & lhs, const D3D11_COLOR_WRITE_ENABLE rhs ) { return lhs = (D3D11_COLOR_WRITE_ENABLE)(lhs | rhs); }

// Must match eTextureFormat
DXGI_FORMAT kbTextureFormatToDXGITextureFormat[NUM_TEXTURE_FORMATS] = {
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_R16G16_UINT,
};

// oculus
#include "OVR_CAPI_D3D.h"
#include "OVR_Math.h"
using namespace OVR;

#if _DEBUG
#pragma comment( lib, "dxguid.lib")
const UINT GCreateDebugD3DDevice = true;
#else
const UINT GCreateDebugD3DDevice = false;
#endif

const UINT GDebugCopyBackBufferToCPU = false;

ID3D11Device * g_pD3DDevice = nullptr;
ID3D11DeviceContext * g_pImmediateContext = nullptr;

const UINT Max_Shader_Bones = 128;
const float	kbRenderer_DX11::Near_Plane = 1.0f;
const float	kbRenderer_DX11::Far_Plane = 20000.0f;

XMMATRIX & XMMATRIXFromkbMat4( kbMat4 & matrix ) { return (*(XMMATRIX*) &matrix); }
kbMat4 & kbMat4FromXMMATRIX( FXMMATRIX & matrix ) { return (*(kbMat4*) & matrix); }


int														kbGPUTimeStamp::m_TimeStampFrameNum = 0;
int														kbGPUTimeStamp::m_NumTimeStamps = 0;
ID3D11Query *											kbGPUTimeStamp::m_pDisjointTimeStamps[2] = { nullptr, nullptr };
kbGPUTimeStamp::GPUTimeStamp_t							kbGPUTimeStamp::m_TimeStamps[kbGPUTimeStamp::MaxTimeStamps];
std::map<kbString, kbGPUTimeStamp::GPUTimeStamp_t * >	kbGPUTimeStamp::m_TimeStampMap;
std::vector<kbGPUTimeStamp::GPUTimeStamp_t * >			kbGPUTimeStamp::m_TimeStampsThisFrame;
bool													kbGPUTimeStamp::m_bActiveThisFrame = false;

/**
 *	kbOculusTexture
 */
class kbOculusTexture {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

    kbOculusTexture() :
        m_Session( nullptr ),
        m_TextureChain( nullptr ) {
    }

	~kbOculusTexture() {
		for ( int i = 0; i < (int)m_TexRtv.size(); i++ ) {
		    SAFE_RELEASE( m_TexRtv[i] );
		}

		if ( m_TextureChain ) {
		    ovr_DestroyTextureSwapChain( m_Session, m_TextureChain );
		}

		for ( int i = 0; i < (int)m_Texture2D.size(); i++ ) {
			SAFE_RELEASE( m_Texture2D[i] );
		}
    }

	bool Init( ovrSession session, const int width, int const height ) {
	    m_Session = session;
	
	    ovrTextureSwapChainDesc desc = {};
	    desc.Type = ovrTexture_2D;
	    desc.ArraySize = 1;
	    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	    desc.Width = width;
	    desc.Height = height;
	    desc.MipLevels = 1;
	    desc.SampleCount = 1;
	    desc.MiscFlags = ovrTextureMisc_DX_Typeless;
	    desc.BindFlags = ovrTextureBind_DX_RenderTarget;
	    desc.StaticImage = ovrFalse;
	
	    ovrResult result = ovr_CreateTextureSwapChainDX( m_Session , g_pD3DDevice, &desc, &m_TextureChain );
	    if ( OVR_SUCCESS( result ) == false ) {
	        return false;
		}

	    int textureCount = 0;
	    ovr_GetTextureSwapChainLength( m_Session, m_TextureChain, &textureCount );
	    for ( int i = 0; i < textureCount; i++ ) {
			ID3D11Texture2D* tex = nullptr;
			ovr_GetTextureSwapChainBufferDX( m_Session, m_TextureChain, i, IID_PPV_ARGS( &tex ) );
			D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			ID3D11RenderTargetView* rtv;
			g_pD3DDevice->CreateRenderTargetView( tex, &rtvd, &rtv );
			m_TexRtv.push_back( rtv );
			m_Texture2D.push_back( tex );
	    }
	
	    return true;
	}

	ovrTextureSwapChain GetTextureChain() {
		return m_TextureChain; 
	}

	ID3D11RenderTargetView * GetRTV() {
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex( m_Session, m_TextureChain, &index );
		return m_TexRtv[index];
	}

	ID3D11Texture2D * GetTexture2D() {
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex( m_Session, m_TextureChain, &index );
		return m_Texture2D[index];
	}

    void Commit() {
        ovr_CommitTextureSwapChain( m_Session, m_TextureChain);
    }

private:

	ovrSession									m_Session;
	ovrTextureSwapChain							m_TextureChain;
	std::vector<ID3D11RenderTargetView*>		m_TexRtv;
	std::vector<ID3D11Texture2D*>				m_Texture2D;

};

//-------------------------------------------------------------------------------------------------------------------------------------
//	kbRenderWindow
//-------------------------------------------------------------------------------------------------------------------------------------

/**
 *	kbRenderWindow_DX11::kbRenderWindow_DX11
 */
kbRenderWindow_DX11::kbRenderWindow_DX11( HWND inHwnd, const RECT & rect, const float nearPlane, const float farPlane ) :
	kbRenderWindow( inHwnd, rect, nearPlane, farPlane ),
	m_pSwapChain( nullptr ),
	m_pRenderTargetView( nullptr ) {

	// HACK should be in base constructor
	kbMat4 projectionMat = GetProjectionMatrix();
	XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( projectionMat ) );
	HackSetInverseProjectionMatrix( kbMat4FromXMMATRIX( inverseProj ) );
}

/**
 *	kbRenderWindow_DX11::~kbRenderWindow_DX11
 */
kbRenderWindow_DX11::~kbRenderWindow_DX11() {
	kbErrorCheck( m_pSwapChain == nullptr, "kbRenderWindow::~kbRenderWindow() - Swap chain still exists" );
	kbErrorCheck( m_pRenderTargetView == nullptr, "kbRenderWindow::~kbRenderWindow() - Target view still exists" );
}

/**
 *	kbRenderWindow_DX11::BeginFrame_Internal
 */
void kbRenderWindow_DX11::BeginFrame_Internal() {
	kbMat4 viewProjectionMatrix = GetViewProjectionMatrix();

	XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( viewProjectionMatrix ) );
	HackSetInverseViewProjectionMatrix( kbMat4FromXMMATRIX( inverseProj ) );
}

/**
 *	kbRenderWindow_DX11::EndFrame
 */
void kbRenderWindow_DX11::EndFrame_Internal() {
	m_pSwapChain->Present( 0, 0 ); 
}

/**
 *	kbRenderWindow::Release_Internal
 */
void kbRenderWindow_DX11::Release_Internal() {
	SAFE_RELEASE( m_pSwapChain );
	SAFE_RELEASE( m_pRenderTargetView ) ; 
}

//-------------------------------------------------------------------------------------------------------------------------------------
//	kbGPUTimeStamp_t
//-------------------------------------------------------------------------------------------------------------------------------------

/**
 *	kbGPUTimeStamp_t::Init
 */
void kbGPUTimeStamp::Init( ID3D11DeviceContext *const DeviceContext ) {

	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_TIMESTAMP;
	queryDesc.MiscFlags = 0;

	HRESULT hr;

	for ( int i = 0; i < MaxTimeStamps; i++ ) {

		hr = g_pD3DDevice->CreateQuery( &queryDesc, &m_TimeStamps[i].m_pQueries[0] );
		kbErrorCheck( SUCCEEDED( hr ), "kbGPUTimeStamp_t::SetupTimeStamps(  - Failed to create query" );

		hr = g_pD3DDevice->CreateQuery( &queryDesc, &m_TimeStamps[i].m_pQueries[1] );
		kbErrorCheck( SUCCEEDED( hr ), "kbGPUTimeStamp_t::SetupTimeStamps(  - Failed to create query" );
	}

	queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

	hr = g_pD3DDevice->CreateQuery( &queryDesc, &m_pDisjointTimeStamps[0] );
	kbErrorCheck( SUCCEEDED( hr ), "kbGPUTimeStamp_t::SetupTimeStamps(  - Failed to create query" );

	hr = g_pD3DDevice->CreateQuery( &queryDesc, &m_pDisjointTimeStamps[1] );
	kbErrorCheck( SUCCEEDED( hr ), "kbGPUTimeStamp_t::SetupTimeStamps(  - Failed to create query" );
}

/**
 *	kbGPUTimeStamp::Shutdown
 */
void kbGPUTimeStamp::Shutdown() {

	for ( int i = 0; i < MaxTimeStamps; i++ ) {
		SAFE_RELEASE( m_TimeStamps[i].m_pQueries[0] );
		SAFE_RELEASE( m_TimeStamps[i].m_pQueries[1] );
	}

	SAFE_RELEASE( m_pDisjointTimeStamps[0] );
	SAFE_RELEASE( m_pDisjointTimeStamps[1] );
}

/**
 *	kbGPUTimeStamp::BeginFrame
 */
void kbGPUTimeStamp::BeginFrame( ID3D11DeviceContext *const DeviceContext ) {

	m_TimeStampsThisFrame.clear();

	extern kbConsoleVariable g_ShowPerfTimers;
	if ( g_ShowPerfTimers.GetBool() == false ) {
		return;
	}

	DeviceContext->Begin( m_pDisjointTimeStamps[m_TimeStampFrameNum] );
	m_bActiveThisFrame = true;
}

/**
 *	kbGPUTimeStamp::EndFrame
 */
void kbGPUTimeStamp::EndFrame( ID3D11DeviceContext *const DeviceContext ) {
	START_SCOPED_TIMER( RENDER_GPUTIMER_STALL );

	if ( m_bActiveThisFrame == false ) {
		return;
	}

	DeviceContext->End( m_pDisjointTimeStamps[m_TimeStampFrameNum] );

	const int prevFrameIdx = m_TimeStampFrameNum ^ 1;

	while( DeviceContext->GetData( m_pDisjointTimeStamps[prevFrameIdx], nullptr, 0, 0 ) == S_FALSE ) {
		Sleep( 1 );
	}

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
	HRESULT hr = DeviceContext->GetData( m_pDisjointTimeStamps[prevFrameIdx], &tsDisjoint, sizeof(tsDisjoint), 0 );
	if ( !tsDisjoint.Disjoint ) {
		UINT64 prevTimeStamp = 0;
		for ( int i = 0; i < m_TimeStampsThisFrame.size(); i++ ) {
			UINT64 timeStamp;
			DeviceContext->GetData( m_TimeStampsThisFrame[i]->m_pQueries[prevFrameIdx], &timeStamp, sizeof(UINT64), 0 );
			if ( i == 0 ) {
				prevTimeStamp = timeStamp;
			} else {
				m_TimeStampsThisFrame[i]->m_TimeMS = float( timeStamp - prevTimeStamp ) / ( tsDisjoint.Frequency ) * 1000.0f;
				prevTimeStamp = timeStamp;
			}
		}
	}
}

/**
 *	kbGPUTimeStamp::UpdateFrameNum
 */
void kbGPUTimeStamp::UpdateFrameNum() {
	m_TimeStampFrameNum ^= 1;
}

/**
 *	kbGPUTimeStamp::PlaceTimeStamp
 */
void kbGPUTimeStamp::PlaceTimeStamp( const kbString & timeStampName, ID3D11DeviceContext *const pDeviceContext ) {

	extern kbConsoleVariable g_ShowPerfTimers;
	if ( g_ShowPerfTimers.GetBool() == false ) {
		return;
	}

	std::map<kbString, GPUTimeStamp_t *>::iterator it = m_TimeStampMap.find( timeStampName );
	GPUTimeStamp_t * pCurTimeStamp = nullptr;
	if ( it == m_TimeStampMap.end() ) {
		if ( m_NumTimeStamps >= MaxTimeStamps ) {
			kbError( "Ran out of GPU time stamps" );
			return;
		}
		pCurTimeStamp = &m_TimeStamps[m_NumTimeStamps];
		m_TimeStampMap[timeStampName] = &m_TimeStamps[m_NumTimeStamps++];
		m_TimeStampMap[timeStampName]->m_Name = timeStampName;
	} else {
		pCurTimeStamp = it->second;
	}
	pDeviceContext->End( pCurTimeStamp->m_pQueries[m_TimeStampFrameNum] );
	m_TimeStampsThisFrame.push_back( pCurTimeStamp );
}

/**
 *	eventMarker_t::eventMarker_t
 */
eventMarker_t::eventMarker_t( const wchar_t *const name, ID3DUserDefinedAnnotation *const pAnnotation ) {

	kbErrorCheck( name != nullptr && pAnnotation != nullptr, "eventMarker_t::eventMarker_t() - Invalid params" );
	m_pEventMarker = pAnnotation;
	m_pEventMarker->BeginEvent( name );
}

/**
 *	eventMarker_t::~eventMarker_t
 */	
eventMarker_t::~eventMarker_t() {
	m_pEventMarker->EndEvent();
}

//-------------------------------------------------------------------------------------------------------------------------------------
//	kbRenderer_DX11
//-------------------------------------------------------------------------------------------------------------------------------------

 kbRenderer_DX11 * g_pD3D11Renderer = nullptr;

/**
 *	kbRenderer_DX11::kbRenderer_DX11
 */
kbRenderer_DX11::kbRenderer_DX11() :
	m_hwnd( nullptr ),
	m_pDXGIFactory( nullptr ),
	m_pD3DDevice( nullptr ),
	m_pDeviceContext( nullptr ),
	m_pDepthStencilBuffer( nullptr ),
	m_pDefaultRasterizerState( nullptr ),
	m_pFrontFaceCullingRasterizerState( nullptr ),
	m_pNoFaceCullingRasterizerState( nullptr ),
	m_pWireFrameRasterizerState( nullptr ),
	m_pDepthStencilView( nullptr ),
	m_pUnitQuad( nullptr ),
	m_pConsoleQuad( nullptr ),
	m_pOpaqueQuadShader( nullptr ),
	m_pTranslucentShader( nullptr ),
	m_pMultiplyBlendShader( nullptr ),
	m_pBasicParticleShader( nullptr ),
	m_pBasicShader( nullptr ),
	m_pDebugShader( nullptr ),
	m_pDirectionalLightShadowShader( nullptr ),
	m_pPointLightShader( nullptr ),
	m_pCylindricalLightShader( nullptr ),
	m_pLightShaftsShader( nullptr ),
	m_pMissingShader( nullptr ),
	m_pDirectionalLightShader( nullptr ),
	m_pSimpleAdditiveShader( nullptr ),
	m_pGodRayIterationShader( nullptr ),
	m_pMousePickerIdShader( nullptr ),
	m_pBasicSamplerState( nullptr ),
	m_pNormalMapSamplerState( nullptr ),
	m_pShadowMapSamplerState( nullptr ),
	m_pEventMarker( nullptr ),
	m_DebugVertexBuffer( nullptr ),
	m_DebugPreTransformedVertexBuffer( nullptr ),
	m_pOffScreenRenderTargetTexture( nullptr ),
	m_ovrSession( nullptr ),
	m_HMDPass( 0 ),
	m_bRenderToHMD( false ),
	m_SensorSampleTime( 0.0 ),
	m_MirrorTexture( nullptr ),
	m_bUsingHMDTrackingOnly( false ),
	m_FrameNum( 0 ),
	m_DebugText( nullptr ) {

	m_pSkinnedDirectionalLightShadowShader = new kbShader( "../../kbEngine/assets/Shaders/directionalLightSkinnedShadow.kbShader" );
	m_pBloomGatherShader = new kbShader( "../../kbEngine/assets/Shaders/bloom.kbShader" );
	m_pBloomBlur = new kbShader( "../../kbEngine/assets/Shaders/bloom.kbShader" );

	ZeroMemory( m_pTextures, sizeof(m_pTextures) );

	m_OculusTexture[0] = m_OculusTexture[1] = nullptr;
	g_pD3D11Renderer = this;
}

/**
 *	kbRenderer_DX11::~kbRenderer_DX11
 */
kbRenderer_DX11::~kbRenderer_DX11() {
	Shutdown();
}

/**
 *	kbRenderer_DX11::Init
 */
void kbRenderer_DX11::Init_Internal( HWND hwnd, const int frameWidth, const int frameHeight, const bool bUseHMD, const bool bUseHMDTrackingOnly ) {

	kbLog( "Initializing kbRenderer_DX11" );

	m_hwnd = hwnd;

	// Create device and swap chain
	UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	if ( GCreateDebugD3DDevice )
		Flags |= D3D11_CREATE_DEVICE_DEBUG;

	HRESULT hr = CreateDXGIFactory1( __uuidof(IDXGIFactory), (void**)&m_pDXGIFactory );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create DXGI Factory" );

	// Initialize HMD if desired
	if ( bUseHMD || bUseHMDTrackingOnly ) {
		m_bRenderToHMD = bUseHMDTrackingOnly == false;
		m_bUsingHMDTrackingOnly = bUseHMDTrackingOnly;
		if ( InitializeOculus() == false ) {
			m_bRenderToHMD = false;
			m_bUsingHMDTrackingOnly = false;
		}
	}

	if ( m_bRenderToHMD == false ) {
		Back_Buffer_Width = frameWidth;
		Back_Buffer_Height = frameHeight;

		hr = D3D11CreateDevice( nullptr,
								D3D_DRIVER_TYPE_HARDWARE,
								nullptr,
								Flags,
								nullptr,
								0,
								D3D11_SDK_VERSION,
								&m_pD3DDevice,
								nullptr,
								&m_pDeviceContext );
	}

	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create D3D11 Device and swap chain" );

	g_pD3DDevice = m_pD3DDevice;
	g_pImmediateContext = m_pDeviceContext;
	m_RenderState.SetDeviceAndContext( m_pD3DDevice, m_pDeviceContext );

	// create swap chains
	CreateRenderView( hwnd );

	// create render targets
	const int deferredRTWidth = ( m_bRenderToHMD )?( m_EyeRenderViewport[0].Size.w * 2 ):( Back_Buffer_Width );
	const int deferredRTHeight = ( m_bRenderToHMD )?( m_EyeRenderViewport[0].Size.h ):( Back_Buffer_Height );

/*
	KBTEXTURE_NULLFORMAT,
	KBTEXTURE_R8G8B8A8,
	KBTEXTURE_R16G16B16A16,
	KBTEXTURE_R32G32,
	KBTEXTURE_R32,
	KBTEXTURE_D24S8,
	KBTEXTURE_R16G16,
	NUM_TEXTURE_FORMATS,
*/
	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R32, false );
	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16B16A16, false );

	const int shadowBufferSize = 4096;
	RT_GetRenderTexture( shadowBufferSize, shadowBufferSize, KBTEXTURE_R32, false );
	RT_GetRenderTexture( shadowBufferSize, shadowBufferSize, KBTEXTURE_D24S8, false );

	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTWidth / 2, deferredRTHeight / 2, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTWidth / 2, deferredRTHeight / 2, KBTEXTURE_R16G16B16A16, false );
	RT_GetRenderTexture( deferredRTHeight / 2, deferredRTHeight / 2, KBTEXTURE_R16G16B16A16, false );

	RT_GetRenderTexture( deferredRTWidth, deferredRTHeight, KBTEXTURE_R16G16, false );

	// MAX_HALF
	RT_GetRenderTexture( 1, 1, KBTEXTURE_R16G16B16A16, false );
	float maxHalf[] = { 65000.0f, 65000.0f, 65000.0f, 65000.0f };
	m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(MAX_HALF_BUFFER)->m_pRenderTargetView, maxHalf );

	// create back buffer
	D3D11_TEXTURE2D_DESC depthBufferDesc = { 0 };
	depthBufferDesc.Width = deferredRTWidth;
	depthBufferDesc.Height = deferredRTHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	hr = m_pD3DDevice->CreateTexture2D( &depthBufferDesc, nullptr, &m_pDepthStencilBuffer );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create DepthStencilBuffer" );

	// specify the subresources of a texture that are accesible from a depth-stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory( &depthStencilViewDesc, sizeof( depthStencilViewDesc ) );
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	hr = m_pD3DDevice->CreateDepthStencilView( m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create DepthStencilView" );

	// bind render target view and depth stencil to output render pipeline
	m_pDeviceContext->OMSetRenderTargets( 1, &((kbRenderWindow_DX11*)m_RenderWindowList[0])->m_pRenderTargetView, m_pDepthStencilView );

	// setting rasterizer state
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the default rasterizer state
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pDefaultRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create default rasterizer state" );

	// Create front face culling rasterizer state
	rasterDesc.CullMode = D3D11_CULL_FRONT;
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pFrontFaceCullingRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create front face culling rasterizer state" );

	// Create non-culling rasterizer state
	rasterDesc.CullMode = D3D11_CULL_NONE;
	// Create the rasterizer state from the description we just filled out.
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pNoFaceCullingRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create non-culling rasterizer state" );

	// Create a wireframe rasterizer state
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pWireFrameRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create wireframe rasterizer state" );

	// Now set the rasterizer state.
	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	// vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.ByteWidth = sizeof( vertexColorLayout ) * 3;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexColorLayout vertices[3];
	vertices[0].position.Set( -1.0f, -1.0f, 0.0f );
	vertices[0].SetColor( kbVec4( 0.25f, 0.35f, 0.75f, 1.0f ) );
	
	vertices[1].position.Set( 0.0f, 1.0f, 0.0f );
	vertices[1].SetColor( kbVec4( 0.25f, 0.35f, 0.75f, 1.0f ) );

	vertices[2].position.Set( 1.0f, -1.0f, 0.0f );
	vertices[2].SetColor( kbVec4( 0.25f, 0.35f, 0.75f, 1.0f ) );

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	vertexLayout fullScreenQuadVerts[6];

	// Tri 1
	fullScreenQuadVerts[2].position.Set( -1.0f, -1.0f, 0.0f );
	fullScreenQuadVerts[2].uv.Set( 0, 1 );
	fullScreenQuadVerts[2].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	fullScreenQuadVerts[1].position.Set( 1.0f, -1.0f, 0.0f );
	fullScreenQuadVerts[1].uv.Set( 1, 1 );
	fullScreenQuadVerts[1].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	fullScreenQuadVerts[0].position.Set( 1.0f, 1.0f, 0.0f );
	fullScreenQuadVerts[0].uv.Set( 1, 0 );
	fullScreenQuadVerts[0].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	// Tri 2
	fullScreenQuadVerts[5].position.Set( 1.0f, 1.0f, 0.0f );
	fullScreenQuadVerts[5].uv.Set( 1, 0 );
	fullScreenQuadVerts[5].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	fullScreenQuadVerts[4].position.Set( -1.0f, 1.0f, 0.0f );
	fullScreenQuadVerts[4].uv.Set( 0, 0 );
	fullScreenQuadVerts[4].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	fullScreenQuadVerts[3].position.Set( -1.0f, -1.0f, 0.0f );
	fullScreenQuadVerts[3].uv.Set( 0, 1 );
	fullScreenQuadVerts[3].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	vertexBufferDesc.ByteWidth = sizeof( vertexLayout ) * 6;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = fullScreenQuadVerts;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	hr = m_pD3DDevice->CreateBuffer( &vertexBufferDesc, &vertexData, &m_pUnitQuad );

	for ( int i = 0; i < 6; i++ ) {
		fullScreenQuadVerts[i].SetColor( kbVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
	}
	fullScreenQuadVerts[2].position.y = -0.5f;
	fullScreenQuadVerts[1].position.y = -0.5f;
	fullScreenQuadVerts[3].position.y = -0.5f;
	hr = m_pD3DDevice->CreateBuffer( &vertexBufferDesc, &vertexData, &m_pConsoleQuad );
	
	// Set up constant buffers
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;	// <-- TODO: Should be static?
	matrixBufferDesc.ByteWidth = 16;
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	while ( matrixBufferDesc.ByteWidth <= 512 ) {

		ID3D11Buffer * pNewConstantBuffer = nullptr;
		hr = m_pD3DDevice->CreateBuffer( &matrixBufferDesc, nullptr, &pNewConstantBuffer );
		kbErrorCheck( SUCCEEDED( hr ), "Failed to create matrix buffer" );

		m_ConstantBuffers.insert( std::pair<size_t, ID3D11Buffer*>( matrixBufferDesc.ByteWidth, pNewConstantBuffer ) );
		matrixBufferDesc.ByteWidth += 16;
	}

	// Load some shaders
	m_pBasicShader = (kbShader *)g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/BasicShader.kbshader", true );	
	m_pOpaqueQuadShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/basicTexture.kbshader", true );
	m_pTranslucentShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbshader", true );
	m_pMultiplyBlendShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/basicMultiplyBlend.kbshader", true );
	m_pBasicParticleShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/basicParticle.kbshader", true );
	m_pMissingShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/missingShader.kbshader", true );
	m_pDebugShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/debugShader.kbshader", true );

	m_pUberPostProcess = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/UberPostProcess.kbshader", true );
	m_pDirectionalLightShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/DirectionalLight.kbshader", true );
	m_pPointLightShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/PointLight.kbshader", true );
	m_pCylindricalLightShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/cylindricalLight.kbshader", true );

	m_pDirectionalLightShadowShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/directionalLightShadow.kbshader", true );
	m_pLightShaftsShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/lightShafts.kbshader", true );
	m_pSimpleAdditiveShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/simpleAdditive.kbshader", true );
	m_pGodRayIterationShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/godRayIteration.kbShader", true );

	m_pMousePickerIdShader = (kbShader *) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/mousePicker.kbshader", true );

	m_pSSAO = (kbShader*) g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/SSAO.kbShader", true );

	// Non-resource managed shaders
	m_pSkinnedDirectionalLightShadowShader->SetVertexShaderFunctionName( "skinnedVertexMain" );
	m_pSkinnedDirectionalLightShadowShader->SetPixelShaderFunctionName( "skinnedPixelMain" );
	m_pSkinnedDirectionalLightShadowShader->Load();

	m_pBloomGatherShader->SetVertexShaderFunctionName( "bloomGatherVertexMain" );
	m_pBloomGatherShader->SetPixelShaderFunctionName( "bloomGatherPixelMain" );
	m_pBloomGatherShader->Load();

	m_pBloomBlur->SetVertexShaderFunctionName( "bloomBlurVertexMain" );
	m_pBloomBlur->SetPixelShaderFunctionName( "bloomBlurPixelMain" );
	m_pBloomBlur->Load();

	// sampler state
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = m_pD3DDevice->CreateSamplerState( &samplerDesc, &m_pBasicSamplerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create basic sampler state" );

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = m_pD3DDevice->CreateSamplerState( &samplerDesc, &m_pNormalMapSamplerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create normal sampler state" );
	
	hr = m_pD3DDevice->CreateSamplerState( &samplerDesc, &m_pShadowMapSamplerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create shadowmap sampler state" );

	// debug vertex buffer
	m_DebugLines.resize( 1024 * 1024 );
	memset( m_DebugLines.data(), 0, sizeof( vertexLayout ) * m_DebugLines.size() );

	D3D11_BUFFER_DESC debugLinesVBDesc = { 0 };
	debugLinesVBDesc.ByteWidth = static_cast< UINT >( sizeof( vertexLayout ) * m_DebugLines.size() );
	debugLinesVBDesc.Usage = D3D11_USAGE_DYNAMIC;
	debugLinesVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	debugLinesVBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	debugLinesVBDesc.MiscFlags = 0;
	debugLinesVBDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA debugLinesVertexData;
	debugLinesVertexData.pSysMem = m_DebugLines.data();
	debugLinesVertexData.SysMemPitch = 0;
	debugLinesVertexData.SysMemSlicePitch = 0;

	hr = g_pD3DDevice->CreateBuffer( &debugLinesVBDesc, &debugLinesVertexData, &m_DebugVertexBuffer );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create debug line vertex buffer" );

	m_DebugPreTransformedLines.resize( 1024 );
	memset( m_DebugPreTransformedLines.data(), 0, sizeof( vertexLayout ) * m_DebugPreTransformedLines.size() );

	debugLinesVBDesc.ByteWidth = static_cast< UINT >( sizeof( vertexLayout ) * m_DebugPreTransformedLines.size() );
	debugLinesVBDesc.Usage = D3D11_USAGE_DYNAMIC;
	debugLinesVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	debugLinesVBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	debugLinesVBDesc.MiscFlags = 0;
	debugLinesVBDesc.StructureByteStride = 0;

	debugLinesVertexData.pSysMem = m_DebugPreTransformedLines.data();
	debugLinesVertexData.SysMemPitch = 0;
	debugLinesVertexData.SysMemSlicePitch = 0;

	hr = g_pD3DDevice->CreateBuffer( &debugLinesVBDesc, &debugLinesVertexData, &m_DebugPreTransformedVertexBuffer );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init_Internal() - Failed to create pretransformed debug line vertex buffer" );

	m_DebugLines.clear();

	//
	D3D11_TEXTURE2D_DESC textureDesc;
	
	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = Back_Buffer_Width;
	textureDesc.Height = Back_Buffer_Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R16G16_UINT;//DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_STAGING;
	textureDesc.BindFlags = 0;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	textureDesc.MiscFlags = 0;

	hr = m_pD3DDevice->CreateTexture2D( &textureDesc, nullptr, &m_pOffScreenRenderTargetTexture );

	LoadTexture( "../../kbEngine/assets/Textures/Editor/white.bmp", 0 );

	kbLog( "  kbRenderer initialized" );

	SetRenderWindow( m_hwnd );

	kbGPUTimeStamp::Init( m_pDeviceContext );

	LoadTexture( "../../kbEngine/assets/Textures/textbackground.dds", 3 );
	LoadTexture( "../../kbEngine/assets/Textures/Font.bmp", 4 );

	m_DebugText = new kbModel();
	m_DebugText->CreateDynamicModel( 10000, 10000 );

	ushort *const pDebugText = (ushort*)m_DebugText->MapIndexBuffer();
	int iVert = 0;

	for ( int i = 0; i < 9996; i += 6, iVert += 4 ) {
		pDebugText[i + 0] = iVert + 0;			// 0	1
		pDebugText[i + 1] = iVert + 2;			//	
		pDebugText[i + 2] = iVert + 3;			//
		pDebugText[i + 3] = iVert + 0;			// 3	2
		pDebugText[i + 4] = iVert + 1;
		pDebugText[i + 5] = iVert + 2;
	}
	m_DebugText->UnmapIndexBuffer();

	hr = m_pDeviceContext->QueryInterface( __uuidof(m_pEventMarker), (void**)&m_pEventMarker );
	kbErrorCheck( SUCCEEDED(hr), " kbRenderer_DX11::Initialize() - Failed to query user defined annotation" );
}

/**
 *	kbRenderer_DX11::InitializeOculus
 */
bool kbRenderer_DX11::InitializeOculus() {

	ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize( &initParams );
	if ( OVR_SUCCESS( result ) == false ) {
		return false;
	}

	ovrGraphicsLuid luid;
	result = ovr_Create( &m_ovrSession, &luid );
	if (OVR_SUCCESS( result ) == false) {
		return false;
	}

	m_HMDDesc = ovr_GetHmdDesc( m_ovrSession );

	// Find the adapter
	IDXGIAdapter * pAdapter = nullptr;
	for ( unsigned int iAdapter = 0; m_pDXGIFactory->EnumAdapters( iAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++iAdapter ) {
		DXGI_ADAPTER_DESC adapterDesc;
		pAdapter->GetDesc(&adapterDesc);

		if ( memcmp(&adapterDesc.AdapterLuid, (void *)&luid, sizeof( LUID ) ) == 0 )
			break;

		SAFE_RELEASE( pAdapter );
	}

	if ( pAdapter == nullptr ) {
		return false;
	}

	// Create Device
	unsigned int flags = (GCreateDebugD3DDevice) ? (D3D11_CREATE_DEVICE_DEBUG) : (0);
	HRESULT hr = D3D11CreateDevice(
		pAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		0,
		flags,
		0,
		0,
		D3D11_SDK_VERSION,
		&m_pD3DDevice,
		nullptr,
		&m_pDeviceContext
	);

	if ( hr != ERROR_SUCCESS ) {
		kbError( "Failed to create oc device" );
	}

	g_pD3DDevice = m_pD3DDevice;

	m_OculusTexture[0] = new kbOculusTexture();
	if ( m_OculusTexture[0]->Init( m_ovrSession, 1920 / 2, 1080 ) == false ) {
		kbError( "Failed to init oculus texture" );
	}
	
	m_EyeRenderViewport[0].Pos.x = 0;
	m_EyeRenderViewport[0].Pos.y = 0;
	m_EyeRenderViewport[0].Size.w = 1920 / 2;
	m_EyeRenderViewport[0].Size.h = 1080;

	m_OculusTexture[1] = new kbOculusTexture();
	if ( m_OculusTexture[1]->Init( m_ovrSession, 1920 / 2, 1080 ) == false ) {
		kbError( "Failed to init oculus texture" );
	}
	
	m_EyeRenderViewport[1].Pos.x = 0;
	m_EyeRenderViewport[1].Pos.y = 0;
	m_EyeRenderViewport[1].Size.w = 1920 / 2;
	m_EyeRenderViewport[1].Size.h = 1080;

    ovrMirrorTextureDesc mirrorDesc = {};	
	mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    mirrorDesc.Width = Back_Buffer_Width;
    mirrorDesc.Height = Back_Buffer_Height;
    result = ovr_CreateMirrorTextureDX( m_ovrSession, g_pD3DDevice, &mirrorDesc, &m_MirrorTexture );
    if ( OVR_SUCCESS( result ) == false )  {
		kbError( "kbRenderer_DX11::InitializeOculus() - Failed to create mirror texture" );
    }

	return true;
}

/*
 * kbRenderer_DX11::CreateRenderView
 */
int kbRenderer_DX11::CreateRenderView( HWND hwnd )
{
	kbErrorCheck( hwnd != nullptr, "nullptr window handle passed into kbRenderer_DX11::CreateRenderView" );

	DXGI_SWAP_CHAIN_DESC sd = { 0 };

	sd.BufferDesc.Width = Back_Buffer_Width;
	sd.BufferDesc.Height = Back_Buffer_Height;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = hwnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	RECT windowDimensions;
	GetClientRect( hwnd, &windowDimensions );

	kbRenderWindow_DX11 * renderView = new kbRenderWindow_DX11( hwnd, windowDimensions, Near_Plane, Far_Plane );

	m_pDXGIFactory->CreateSwapChain( m_pD3DDevice, &sd, &renderView->m_pSwapChain );

	ID3D11Texture2D * pBackBuffer;
	renderView->m_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer );
	m_pD3DDevice->CreateRenderTargetView( pBackBuffer, nullptr, &renderView->m_pRenderTargetView );
	pBackBuffer->Release();

	m_RenderWindowList.push_back( renderView );

	return (int)m_RenderWindowList.size() - 1;
}

/**
 *	kbRenderer_DX11::GetRenderTexture_Internal
 */
kbRenderTexture * kbRenderer_DX11::GetRenderTexture_Internal( const int width, const int height, const eTextureFormat targetFormat, const bool bIsCPUAccessible  ) {

	m_pRenderTargets.push_back( new kbRenderTexture_DX11( width, height, targetFormat, bIsCPUAccessible ) );

	kbRenderTexture_DX11 & rt = *GetRenderTarget_DX11( (eReservedRenderTargets)(m_pRenderTargets.size() - 1) );

	if ( targetFormat == KBTEXTURE_D24S8 ) {
		// create back buffer
		D3D11_TEXTURE2D_DESC depthBufferDesc = { 0 };
		depthBufferDesc.Width = width;
		depthBufferDesc.Height = height;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;
		m_pD3DDevice->CreateTexture2D( &depthBufferDesc, nullptr, &rt.m_pRenderTargetTexture );

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory( &depthStencilViewDesc, sizeof( depthStencilViewDesc ) );
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags = 0;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		m_pD3DDevice->CreateDepthStencilView( rt.m_pRenderTargetTexture, &depthStencilViewDesc, &rt.m_pDepthStencilView );

		// Shader resource view
	/*	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	
		shaderResourceViewDesc.Format = depthBufferDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
	
		HRESULT hr = m_pD3DDevice->CreateShaderResourceView( GetRenderTarget_DX11(index].m_pRenderTargetTexture, &shaderResourceViewDesc, &GetRenderTarget_DX11(index].m_pShaderResourceView );
*/
		return &rt;
	}

	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = kbTextureFormatToDXGITextureFormat[targetFormat];
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = ( bIsCPUAccessible == false ) ? ( D3D11_USAGE_DEFAULT ) : ( D3D11_USAGE_STAGING );
	textureDesc.BindFlags = ( bIsCPUAccessible == false ) ? ( D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE ) : ( 0 );
	textureDesc.CPUAccessFlags = ( bIsCPUAccessible == false ) ? ( 0 ) : ( D3D11_CPU_ACCESS_READ );
	textureDesc.MiscFlags = 0;

	HRESULT hr = m_pD3DDevice->CreateTexture2D( &textureDesc, nullptr, &rt.m_pRenderTargetTexture );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::CreateRenderTarget() - Failed to create 2D texture with format", (int)targetFormat );

	if ( bIsCPUAccessible ) {
		return &rt;
	}

	// Render target view
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	hr = m_pD3DDevice->CreateRenderTargetView( rt.m_pRenderTargetTexture, &renderTargetViewDesc, &rt.m_pRenderTargetView );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::CreateRenderTarget() - Failed to create RTV with format", (int)targetFormat );

	// Shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = m_pD3DDevice->CreateShaderResourceView( rt.m_pRenderTargetTexture, &shaderResourceViewDesc, &rt.m_pShaderResourceView );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::CreateRenderTarget() - Failed to create SRV texture for index", (int)targetFormat );

	return &rt;
}

/**
 *	kbRenderer_DX11::ReturnRenderTexture_Internal
 */
void kbRenderer_DX11::ReturnRenderTexture_Internal( const kbRenderTexture *const pRenderTexture ) {

}


/**
 *	kbRenderer_DX11::Shutdown_Internal
 */
void kbRenderer_DX11::Shutdown_Internal() {

	for ( int i = 0; i < Max_Num_Textures; i++ ) {
		if ( m_pTextures[i] != nullptr ) {
			m_pTextures[i]->Release();
			delete m_pTextures[i];
			m_pTextures[i] = nullptr;
		}
	}

	SAFE_RELEASE( m_pOffScreenRenderTargetTexture );
	SAFE_RELEASE( m_pDepthStencilView );
	SAFE_RELEASE( m_pDepthStencilBuffer );

	for ( int i = 0; i < m_RenderWindowList.size(); i++ ) {
		m_RenderWindowList[i]->Release();
		delete m_RenderWindowList[i];
	}
	m_RenderWindowList.clear();

	SAFE_RELEASE( m_DebugVertexBuffer );
	SAFE_RELEASE( m_DebugPreTransformedVertexBuffer );
	SAFE_RELEASE( m_pUnitQuad );
	SAFE_RELEASE( m_pConsoleQuad );

	for ( int i = 0; i < m_ConstantBuffers.size(); i++ ) {
		SAFE_RELEASE( m_ConstantBuffers[i] );
	}
	m_ConstantBuffers.clear();

	SAFE_RELEASE( m_pSkinnedDirectionalLightShadowShader );
	SAFE_RELEASE( m_pBloomGatherShader );
	SAFE_RELEASE( m_pBloomBlur );

	if ( m_bRenderToHMD ) {
		delete m_OculusTexture[0];
		m_OculusTexture[0] = nullptr;

		delete m_OculusTexture[1];
		m_OculusTexture[1] = nullptr;

		ovr_DestroyMirrorTexture( m_ovrSession, m_MirrorTexture );
		ovr_Destroy( m_ovrSession );
		ovr_Shutdown();
	}

	SAFE_RELEASE( m_pDefaultRasterizerState );
	SAFE_RELEASE( m_pFrontFaceCullingRasterizerState );
	SAFE_RELEASE( m_pNoFaceCullingRasterizerState );
	SAFE_RELEASE( m_pWireFrameRasterizerState );

	SAFE_RELEASE( m_pBasicSamplerState );
	SAFE_RELEASE( m_pNormalMapSamplerState );
	SAFE_RELEASE( m_pShadowMapSamplerState );

	SAFE_RELEASE( m_pDXGIFactory );

	delete m_DebugText;
	m_DebugText = nullptr;

	m_RenderState.Shutdown();

	kbGPUTimeStamp::Shutdown();
	SAFE_RELEASE( m_pEventMarker );

	SAFE_RELEASE( m_pDeviceContext );

	if ( GCreateDebugD3DDevice ) {
		kbLog( "" );
		kbLog( "" );
		kbLog( "  Reporting live D3D Objects..........................................................." );
		ID3D11Debug * debugDev;
		HRESULT hr = m_pD3DDevice->QueryInterface( __uuidof(ID3D11Debug), reinterpret_cast<void**>( &debugDev ) );
		debugDev->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
		debugDev->Release();
	}

	SAFE_RELEASE( m_pD3DDevice );

	kbLog( "  kbRenderer Shutdown" );
}

/**
 *	kbRenderer_DX11::SetRenderTarget
 */
void kbRenderer_DX11::SetRenderTarget( eReservedRenderTargets type ) {
	m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(type)->m_pRenderTargetView, m_pDepthStencilView );
}

/**
 *	kbRenderer_DX11::SetRenderWindow
 */
void kbRenderer_DX11::SetRenderWindow( HWND hwnd ) {

	if ( hwnd == nullptr ) {
		m_pCurrentRenderWindow = m_RenderWindowList[0];
		return;
	}

	for ( int i = 0 ; i < m_RenderWindowList.size(); i++ ) {
		if ( m_RenderWindowList[i]->GetHWND() == hwnd ) {
			m_pCurrentRenderWindow = m_RenderWindowList[i];
			return;
		}
	}

	kbError( "SetRenderWindow called with a bad window handle" );
}

/*
 *	kbRenderer_DX11:RenderScene
 */
void kbRenderer_DX11::RenderScene() {
	START_SCOPED_TIMER( RENDER_THREAD );

	PreRenderCullAndSort();

	kbGPUTimeStamp::BeginFrame( m_pDeviceContext );
	PLACE_GPU_TIME_STAMP( "Begin Frame" );

	const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const int numRenderPasses = ( IsRenderingToHMD() ) ? ( 2 ) : ( 1 );

	m_pCurrentRenderWindow->BeginFrame();

	for ( m_HMDPass = 0; m_HMDPass < numRenderPasses; m_HMDPass++ ) {

		{
			START_SCOPED_RENDER_TIMER( RENDER_THREAD_CLEAR_BUFFERS );
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(COLOR_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(NORMAL_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(SPECULAR_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(DEPTH_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );
		}

		if ( m_ViewMode != ViewMode_Wireframe ) {
			m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
		} else {
			m_pDeviceContext->RSSetState( m_pWireFrameRasterizerState );
		}

		D3D11_VIEWPORT viewport;
	
		if ( IsRenderingToHMD() ) {
			viewport.TopLeftX = ( float )m_EyeRenderViewport[m_HMDPass].Pos.x;
			viewport.TopLeftY = ( float )m_EyeRenderViewport[m_HMDPass].Pos.y;
			viewport.Width = ( float )m_EyeRenderViewport[m_HMDPass].Size.w;
			viewport.Height = ( float )m_EyeRenderViewport[m_HMDPass].Size.h;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
		
			const ovrMatrix4f proj = ovrMatrix4f_Projection( m_EyeRenderDesc[m_HMDPass].Fov, kbRenderer_DX11::Near_Plane, kbRenderer_DX11::Far_Plane, ovrProjection_None );
			// HACK TODO
/*			memcpy( &m_pCurrentRenderWindow->m_ProjectionMatrix, &proj, sizeof( ovrMatrix4f ) );
			m_pCurrentRenderWindow->m_ProjectionMatrix.TransposeSelf();

			m_pCurrentRenderWindow->m_ProjectionMatrix[2].z *= -1.0f;
			m_pCurrentRenderWindow->m_ProjectionMatrix[2].w *= -1.0f;

			m_pCurrentRenderWindow->m_ViewMatrix = m_pCurrentRenderWindow->m_EyeMatrices[m_HMDPass];
			m_pCurrentRenderWindow->m_ViewProjectionMatrix = m_pCurrentRenderWindow->m_ViewMatrix * m_pCurrentRenderWindow->m_ProjectionMatrix;
		
			FXMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewProjectionMatrix ) );
			m_pCurrentRenderWindow->m_InverseViewProjectionMatrix = kbMat4FromXMMATRIX( inverseProj );

			FXMMATRIX inverseView = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewMatrix ) );
			m_pCurrentRenderWindow->m_CameraPosition.x = inverseView.m[3][0];
			m_pCurrentRenderWindow->m_CameraPosition.y = inverseView.m[3][1];
			m_pCurrentRenderWindow->m_CameraPosition.z = inverseView.m[3][2];*/


		} else {
			viewport.Width = (float)Back_Buffer_Width;
			viewport.Height = (float)Back_Buffer_Height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
/*
			if ( IsUsingHMDTrackingOnly() ) {
			/	m_pCurrentRenderWindow->m_ViewMatrix = m_pCurrentRenderWindow->m_EyeMatrices[m_HMDPass];
				m_pCurrentRenderWindow->m_ViewProjectionMatrix = m_pCurrentRenderWindow->m_ViewMatrix * m_pCurrentRenderWindow->m_ProjectionMatrix;

				XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewProjectionMatrix ) );
				m_pCurrentRenderWindow->m_InverseViewProjectionMatrix = kbMat4FromXMMATRIX( inverseProj );
			}

			if ( this->IsUsingHMDTrackingOnly() ) {
				XMMATRIX inverseView = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewMatrix ) );
				m_pCurrentRenderWindow->m_CameraPosition.x = inverseView.m[3][0];
				m_pCurrentRenderWindow->m_CameraPosition.y = inverseView.m[3][1];
				m_pCurrentRenderWindow->m_CameraPosition.z = inverseView.m[3][2];
			}*/
		}
	
		{

			for ( int iHook = 0; iHook < m_RenderHooks[RP_FirstPerson].size(); iHook++ ) {
				m_RenderHooks[RP_FirstPerson][iHook]->RenderThreadCallBack();
			}

			m_pDeviceContext->RSSetViewports( 1, &viewport );
			ID3D11RenderTargetView * RenderTargetViews[] = { GetRenderTarget_DX11(COLOR_BUFFER)->m_pRenderTargetView, GetRenderTarget_DX11(NORMAL_BUFFER)->m_pRenderTargetView, GetRenderTarget_DX11(SPECULAR_BUFFER)->m_pRenderTargetView, GetRenderTarget_DX11(DEPTH_BUFFER)->m_pRenderTargetView };
			m_pDeviceContext->OMSetRenderTargets( 4, RenderTargetViews, m_pDepthStencilView );

			START_SCOPED_RENDER_TIMER( RENDER_G_BUFFER );

			// First person Render Pass
			// Note: The first-person pass is drawn in the foreground pass before world objects. Foreground pixels set the stencil buffer to one to mask out
			//  later pixels in the later background (world) pass. overwriting pixels drawn in the first-person pass
			m_RenderState.SetDepthStencilState( true,
												kbRenderState::DepthWriteMaskAll,
												kbRenderState::CompareLess,
												true,
												0,
												0xff,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareAlways,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareAlways,
												1);

			m_RenderState.SetBlendState();

			std::vector<kbRenderSubmesh> & FirstPersonPassVisibleList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_FirstPerson );
			for ( int i = 0; i < FirstPersonPassVisibleList.size(); i++ ) {
				RenderMesh( &FirstPersonPassVisibleList[i], false );
			}

			// Render models that need to be lit
			m_RenderState.SetDepthStencilState();

			std::vector<kbRenderSubmesh> & LightingPassVisibleList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Lighting );
			for ( int i = 0; i < LightingPassVisibleList.size(); i++ ) {
				RenderMesh( &LightingPassVisibleList[i], false );
			}

			m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

			PLACE_GPU_TIME_STAMP( "GBuffer" );
		}

		RenderLights();
	
		RenderSSAO();

		{
			START_SCOPED_RENDER_TIMER( RENDER_UNLIT )

			m_RenderState.SetDepthStencilState();
			m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetView, m_pDepthStencilView );

			if ( m_ViewMode == ViewMode_Wireframe ) {
				m_pDeviceContext->CopyResource( GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetTexture, GetRenderTarget_DX11(COLOR_BUFFER)->m_pRenderTargetTexture );
			}

			// Post-Lighting Render Pass
			std::vector<kbRenderSubmesh> & PostLightingVisibleList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_PostLighting );
			for ( int i = 0; i < PostLightingVisibleList.size(); i++ ) {
				RenderMesh( &PostLightingVisibleList[i], false  );
			}

			PLACE_GPU_TIME_STAMP( "Unlit" );
		}

		if ( m_ViewMode == ViewMode_Shaded ) {
			RenderLightShafts();
		}
		
		PLACE_GPU_TIME_STAMP( "Light Shafts" );

		RenderTranslucency();
		RenderScreenSpaceQuads();

		if ( m_ViewMode == ViewMode_Shaded ) {
			// In World UI Pass
			m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );
			std::vector<kbRenderSubmesh> & InWorldUIVisibleList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_InWorldUI );
			for ( int i = 0; i < InWorldUIVisibleList.size(); i++ ) {
				RenderMesh( &InWorldUIVisibleList[i], false );
			}

			m_RenderState.SetBlendState();
		}

		PLACE_GPU_TIME_STAMP( "Transparency" );

		//m_pDeviceContext->OMSetDepthStencilState( m_pNoDepthStencilState, 1 );

		RenderPostProcess();

		PLACE_GPU_TIME_STAMP( "Post-Process" );

		m_RenderState.SetDepthStencilState();
	
		if ( m_ViewMode == ViewMode_Shaded ) {
			START_SCOPED_RENDER_TIMER( RENDER_DEBUG )
			RenderDebugBillboards( false );
			RenderDebugLines();
			RenderPretransformedDebugLines();

			for ( int i = 0; i < m_DebugModels.size(); i++ ) {

				kbRenderObject renderObject;
				renderObject.m_pModel = m_DebugModels[i].m_pModel;
				renderObject.m_Materials = m_DebugModels[i].m_Materials;
				renderObject.m_Position = m_DebugModels[i].m_Position;
				renderObject.m_Orientation = m_DebugModels[i].m_Orientation;
				renderObject.m_Scale = m_DebugModels[i].m_Scale;
				renderObject.m_EntityId = m_DebugModels[i].m_EntityId;
				for ( int j = 0; j < renderObject.m_pModel->NumMeshes(); j++ ) {
					kbRenderSubmesh newMesh( &renderObject, j, RP_Debug, 0.0f );
					RenderMesh( &newMesh, false );
				}
			}
		}
	
		m_RenderState.SetDepthStencilState();

		if ( IsRenderingToHMD() || this->IsUsingHMDTrackingOnly() ) {
			m_OculusTexture[m_HMDPass]->Commit();
		}
	}

	RenderDebugText();

	if ( g_UseEditor ) {
		m_RenderState.SetBlendState();
		RenderMousePickerIds();
	}
	m_DebugLines.clear();
	m_DebugBillboards.clear();
	m_ScreenSpaceQuads_RenderThread.clear();
	m_DebugModels.clear();

	PLACE_GPU_TIME_STAMP( "Debug Rendering" );

	if ( IsUsingHMDTrackingOnly() || IsRenderingToHMD() ) {
		// Initialize our single full screen Fov layer.
		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		ld.Header.Flags = 0;
	
		for ( int eye = 0; eye < 2; ++eye ) {
			ld.ColorTexture[eye] = m_OculusTexture[eye]->GetTextureChain();
			ld.Viewport[eye] = m_EyeRenderViewport[eye];
			ld.Fov[eye] = m_HMDDesc.DefaultEyeFov[eye];
			ld.RenderPose[eye] = m_EyeRenderPose[eye];
			ld.SensorSampleTime = m_SensorSampleTime;
		}

		ovrLayerHeader* layers = &ld.Header;
		ovrResult result = ovr_SubmitFrame( m_ovrSession, m_FrameNum - 1, nullptr, &layers, 1 );		// hack: need the frame number to start at 0

		if ( IsUsingHMDTrackingOnly() == false ) {
			// Render mirror
			ID3D11Texture2D* tex = nullptr;
			ID3D11Texture2D * pBackBuffer = nullptr;
			((kbRenderWindow_DX11*)m_pCurrentRenderWindow)->m_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer );
			ovr_GetMirrorTextureBufferDX( m_ovrSession, m_MirrorTexture, IID_PPV_ARGS(&tex) );
			m_pDeviceContext->CopyResource( pBackBuffer, tex );
			tex->Release();
			((kbRenderWindow_DX11*)m_pCurrentRenderWindow)->m_pSwapChain->Present( 0, 0 );
		}

		if ( GetAsyncKeyState( VK_SPACE ) ) {
		   ovr_RecenterTrackingOrigin( m_ovrSession );
		}
	} 

	if ( IsRenderingToHMD() == false || IsUsingHMDTrackingOnly() == true ) {
		START_SCOPED_RENDER_TIMER( RENDER_PRESENT );
		m_pCurrentRenderWindow->EndFrame();
	}

	PLACE_GPU_TIME_STAMP( "End Frame" );
	kbGPUTimeStamp::EndFrame( m_pDeviceContext );
}

/**
 *	kbRenderer_DX11::PreRenderCullAndSort
 */
void kbRenderer_DX11::PreRenderCullAndSort() {

	for ( int i = 0; i < NUM_RENDER_PASSES; i++ ) {
		m_pCurrentRenderWindow->GetVisibleSubMeshes( i ).clear();
	}

	for ( auto iter = m_pCurrentRenderWindow->GetRenderObjectMap().begin(); iter != m_pCurrentRenderWindow->GetRenderObjectMap().end(); iter++ ) {

		bool bIsVisible = true;

		kbRenderObject & renderObj = *iter->second;

		const float distToCamSqr = ( renderObj.m_Position - m_pCurrentRenderWindow->GetCameraPosition() ).LengthSqr();
		if ( renderObj.m_CullDistance > 0 ) {
			const float cullDistSqr = renderObj.m_CullDistance * renderObj.m_CullDistance;
	
			if ( distToCamSqr >= cullDistSqr ) {
				bIsVisible = false;
				continue;
			}

		}
		
		if ( bIsVisible ) {

			// Add submeshes to their proper render passes
			const kbModel *const pModel = renderObj.m_pModel;
			for ( int i = 0; i < pModel->GetMeshes().size(); i++ ) {
				const kbModel::mesh_t & mesh = pModel->GetMeshes()[i];
				const kbShader * pShader = nullptr;
				if ( renderObj.m_Materials.size() > i ) {
					pShader = renderObj.m_Materials[i].m_pShader;
				}

				if ( pShader == nullptr || pShader->IsBlendEnabled() == false ) {
					m_pCurrentRenderWindow->GetVisibleSubMeshes( renderObj.m_RenderPass ).push_back( kbRenderSubmesh( &renderObj, i, renderObj.m_RenderPass, sqrt( distToCamSqr ) ) );
				} else {
					m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Translucent ).push_back( kbRenderSubmesh( &renderObj, i, RP_Translucent, sqrt( distToCamSqr ) ) );
		
				}
			}
		}
	}

	const std::map<const void *, kbRenderObject *> & curMap = m_pCurrentRenderWindow->GetRenderParticleMap();
	for ( auto iter = curMap.begin(); iter != curMap.end(); iter++ ) {

		kbRenderObject & renderObj = *iter->second;
		const float distToCamSqr = ( renderObj.m_Position - m_pCurrentRenderWindow->GetCameraPosition() ).LengthSqr();
		if ( renderObj.m_CullDistance > 0 ) {
			const float cullDistSqr = renderObj.m_CullDistance * renderObj.m_CullDistance;
			
	
			if ( distToCamSqr >= cullDistSqr ) {
				continue;
			}
		}

		const float distToCam = sqrt( distToCamSqr );
		kbRenderSubmesh newMesh( iter->second, 0, RP_Translucent, distToCam );
		m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Translucent ).push_back( newMesh );
	}

	// Sort translucent meshes by depth
	std::vector<kbRenderSubmesh> & visibleTranslucentMeshes = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Translucent );
	std::sort( visibleTranslucentMeshes.begin(), visibleTranslucentMeshes.end(), []( kbRenderSubmesh & op1, kbRenderSubmesh & op2 ) {
		const float op1Dist = op1.GetDistFromCamera() + op1.GetRenderObject()->m_TranslucencySortBias;
		const float op2Dist = op2.GetDistFromCamera() + op2.GetRenderObject()->m_TranslucencySortBias;

		return op1Dist > op2Dist;
	});
}

/**
 *	kbRenderer_DX11::RenderTranslucency
 */
void kbRenderer_DX11::RenderTranslucency() {
	START_SCOPED_RENDER_TIMER( RENDER_TRANSLUCENCY );

	m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetView, m_pDepthStencilView );

	m_RenderState.SetDepthStencilState(	true,
										kbRenderState::DepthWriteMaskZero,
										kbRenderState::CompareLess,
										true,
										0xff,
										0x0,
										kbRenderState::StencilKeep,
										kbRenderState::StencilKeep,
										kbRenderState::StencilReplace,
										kbRenderState::CompareNotEqual,
										kbRenderState::StencilKeep,
										kbRenderState::StencilKeep,
										kbRenderState::StencilReplace,
										kbRenderState::CompareNotEqual,
										1);

	std::vector<kbRenderSubmesh> & visibleSubmeshList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Translucent );
	for ( int i = 0; i < visibleSubmeshList.size(); i++ ) {
		const kbRenderSubmesh & submesh = visibleSubmeshList[i];
		const kbModel *const pModel = submesh.GetRenderObject()->m_pModel;

		if ( submesh.GetRenderObject()->m_RenderPass == RP_FirstPerson ) {
			m_RenderState.SetDepthStencilState( true,
												kbRenderState::DepthWriteMaskZero,
												kbRenderState::CompareLess,
												true,
												0,
												0xff,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareAlways,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareAlways,
												1);
		} else {
			m_RenderState.SetDepthStencilState(	true,
												kbRenderState::DepthWriteMaskZero,
												kbRenderState::CompareLess,
												true,
												0xff,
												0x0,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareNotEqual,
												kbRenderState::StencilKeep,
												kbRenderState::StencilKeep,
												kbRenderState::StencilReplace,
												kbRenderState::CompareNotEqual,
												1);
		}

		RenderMesh( &visibleSubmeshList[i], false );
	}
	

	m_RenderState.SetBlendState();

	m_RenderState.SetDepthStencilState();
}

/**
 *	kbRenderer_DX11::SetShaderMat4
 */
void kbRenderer_DX11::SetShaderMat4( const std::string & varName, const kbMat4 & inMatrix, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const std::vector<kbShaderVarBindings_t::binding_t> & varBindings = binding.m_VarBindings;
	for ( int i = 0; i < varBindings.size(); i++ ) {
		if ( varBindings[i].m_VarName == varName ) {
			kbMat4 *const pMat = (kbMat4*)( (byte*) pBuffer + varBindings[i].m_VarByteOffset );
			*pMat = inMatrix;
			return;
		}
	}

	kbError( "Failed to set Shader var" );
}

/**
 *	kbRenderer_DX11::SetShaderVec4
 */
void kbRenderer_DX11::SetShaderVec4( const std::string & varName, const kbVec4 & inVec, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const std::vector<kbShaderVarBindings_t::binding_t> & varBindings = binding.m_VarBindings;
	for ( int i = 0; i < varBindings.size(); i++ ) {
		if ( varBindings[i].m_VarName == varName ) {
			kbVec4 *const pVec = (kbVec4*)( (byte*) pBuffer + varBindings[i].m_VarByteOffset );
			*pVec = inVec;
			return;
		}
	}
	kbError( "Failed to set Shader var" );
}

/**
 *	kbRenderer_DX11::SetShaderFloat
 */
void kbRenderer_DX11::SetShaderFloat( const std::string & varName, const float inFloat, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const int varBindingIdx = GetVarBindingIndex( varName, binding );
	kbErrorCheck( varBindingIdx >= 0, "kbRenderer_DX11::SetShaderFloat() - Failed to find binding for shader var %s", varName.c_str() );

	float *const pFloat = (float*)( (byte*)pBuffer + binding.m_VarBindings[varBindingIdx].m_VarByteOffset );
	*pFloat = inFloat;
}

/**
 *	kbRenderer_DX11::SetShaderInt
 */
void kbRenderer_DX11::SetShaderInt( const std::string & varName, const int inInt, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const int varBindingIdx = GetVarBindingIndex( varName, binding );
	kbErrorCheck( varBindingIdx >= 0, "kbRenderer_DX11::SetShaderInt() - Failed to find binding for shader var %s", varName.c_str() );

	int *const pInt = (int*)( (byte*)pBuffer + binding.m_VarBindings[varBindingIdx].m_VarByteOffset );
	*pInt = inInt;
}

/**
 *	kbRenderer_DX11::SetShaderVec4Array
 */
void kbRenderer_DX11::SetShaderVec4Array( const std::string & varName, const kbVec4 *const pSrcArray, const int arrayLen, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const int varBindingIdx = GetVarBindingIndex( varName, binding );
	kbErrorCheck( varBindingIdx >= 0, "kbRenderer_DX11::SetShaderVec4Array() - Failed to find binding for shader var %s", varName.c_str() );

	kbVec4 *const pDestArray = (kbVec4*)( (byte*)pBuffer + binding.m_VarBindings[varBindingIdx].m_VarByteOffset );
	for ( int j = 0; j < arrayLen; j++ ) {
		pDestArray[j] = pSrcArray[j];
	}
}

/**
 *	kbRenderer_DX11::SetShaderMat4Array
 */
void kbRenderer_DX11::SetShaderMat4Array( const std::string & varName, const kbMat4 *const pSrcArray, const int arrayLen, void *const pBuffer, const kbShaderVarBindings_t & binding ) {

	const int varBindingIdx = GetVarBindingIndex( varName, binding );
	kbErrorCheck( varBindingIdx >= 0, "kbRenderer_DX11::SetShaderMat4Array() - Failed to find binding for shader var %s", varName.c_str() );

	kbMat4 *const pDestArray = (kbMat4*)( (byte*)pBuffer + binding.m_VarBindings[varBindingIdx].m_VarByteOffset );
	for ( int j = 0; j < arrayLen; j++ ) {
		pDestArray[j] = pSrcArray[j];
	}
}

/**
 *	kbRenderer_DX11::GetVarBindingIndex
 */
int kbRenderer_DX11::GetVarBindingIndex( const std::string & varName, const kbShaderVarBindings_t & binding ) {
	const std::vector<kbShaderVarBindings_t::binding_t> & varBindings = binding.m_VarBindings;
	for ( int i = 0; i < varBindings.size(); i++ ) {
		if ( varBindings[i].m_VarName == varName ) {
			return i;
		}
	}

	return -1;
}

/**
 *	kbRenderer_DX11::GetConstantBuffer
 */
ID3D11Buffer * kbRenderer_DX11::GetConstantBuffer( const size_t requestSize ) {
	
	std::map<size_t, ID3D11Buffer *>::iterator constantBufferIt = m_ConstantBuffers.find( requestSize );
	kbErrorCheck( constantBufferIt != m_ConstantBuffers.end() && constantBufferIt->second != nullptr, "kbRenderer_DX11::GetConstantBuffer() - Could not find constant buffer of size %d", requestSize );

	return constantBufferIt->second;
}

/**
 *	kbRenderer_DX11::RenderDebugText
 */
void kbRenderer_DX11::RenderDebugText() {
	START_SCOPED_RENDER_TIMER( RENDER_TEXT );

	m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

	if ( m_bConsoleEnabled ) {
		RenderConsole();
	}

	extern kbConsoleVariable g_ShowPerfTimers;
	if ( g_ShowPerfTimers.GetBool() ) {
		RenderScreenSpaceQuadImmediate( int( Back_Buffer_Width * 0.25f ), int( Back_Buffer_Height * 0.1f ), int( Back_Buffer_Width * 0.51f ), int( Back_Buffer_Height * 0.65f ), 3, m_pTranslucentShader );
	}

	/*if ( m_bRenderToHMD == true ) {
		return;
	}*/

	vertexLayout * pDebugTextVB = (vertexLayout*)m_DebugText->MapVertexBuffer();
	int iVert = 0;

	for ( int iString = 0; iString < m_DebugStrings.size(); iString++ ) {
		float startX = m_DebugStrings[iString].screenX;
		float startY = m_DebugStrings[iString].screenY;
		const float charW = m_DebugStrings[iString].screenW;
		const kbColor & textColor = m_DebugStrings[iString].color;

		float charSpacing = charW * 0.5f;
		for ( int iChar = 0; iChar < m_DebugStrings[iString].TextInfo.size(); iChar++ ) {
			const char currentCharacter = m_DebugStrings[iString].TextInfo[iChar];
			const int characterIndex = currentCharacter - ' ' + 32;
			const float charRow = ( float )( characterIndex / 16 ) / 16.0f;
			const float charCol = ( float )( characterIndex % 16 ) / 16.0f;
			pDebugTextVB[iVert + 0].position.x = startX;
			pDebugTextVB[iVert + 0].position.y = startY;
			pDebugTextVB[iVert + 0].position.z = 0.0f;
			pDebugTextVB[iVert + 0].uv.x = charCol;
			pDebugTextVB[iVert + 0].uv.y = charRow;
			pDebugTextVB[iVert + 0].SetColor( textColor );

			pDebugTextVB[iVert + 1].position.x = startX + charW;
			pDebugTextVB[iVert + 1].position.y = startY;
			pDebugTextVB[iVert + 1].position.z = 0.0f;
			pDebugTextVB[iVert + 1].uv.x = charCol + ( 1.0f / 16.0f );
			pDebugTextVB[iVert + 1].uv.y = charRow;
			pDebugTextVB[iVert + 1].SetColor( textColor );

			pDebugTextVB[iVert + 2].position.x = startX + charW;
			pDebugTextVB[iVert + 2].position.y = startY + charW;
			pDebugTextVB[iVert + 2].position.z = 0.0f;
			pDebugTextVB[iVert + 2].uv.x = charCol + ( 1.0f / 16.0f );
			pDebugTextVB[iVert + 2].uv.y = charRow + ( 1.0f / 16.0f );
			pDebugTextVB[iVert + 2].SetColor( textColor );

			pDebugTextVB[iVert + 3].position.x = startX;
			pDebugTextVB[iVert + 3].position.y = startY + charW;
			pDebugTextVB[iVert + 3].position.z = 0.0f;
			pDebugTextVB[iVert + 3].uv.x = charCol;
			pDebugTextVB[iVert + 3].uv.y = charRow + ( 1.0f / 16.0f );
			pDebugTextVB[iVert + 3].SetColor( textColor );

			startX += charSpacing;

			iVert += 4;
		}
	}

	int numTris = ( iVert  / 4 ) * 2;
	m_DebugText->UnmapVertexBuffer((iVert / 4) * 6);

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;


	m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

	ID3D11Buffer * const vertexBuffer = ( ID3D11Buffer * const ) m_DebugText->m_VertexBuffer.GetBufferPtr();
	ID3D11Buffer * const indexBuffer = ( ID3D11Buffer * const ) m_DebugText->m_IndexBuffer.GetBufferPtr();

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetIndexBuffer( indexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[4]->GetGPUTexture();

	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pSimpleAdditiveShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pSimpleAdditiveShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pSimpleAdditiveShader->GetPixelShader(), nullptr, 0 );
	m_RenderState.SetBlendState( m_pSimpleAdditiveShader );

	const kbShaderVarBindings_t & shaderVarBindings = m_pSimpleAdditiveShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( shaderVarBindings.m_ConstantBufferSizeBytes );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

	kbMat4 mvpMatrix;
	mvpMatrix.MakeIdentity();
	mvpMatrix[0].x = 2.0f;
	mvpMatrix[1].y = -2.0f;
	mvpMatrix[3].x = -1.0f;
	mvpMatrix[3].y = 1.0f;
	SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, shaderVarBindings );
	
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->DrawIndexed( m_DebugText->GetMeshes()[0].m_NumTriangles * 3, 0, 0 );

	m_RenderState.SetBlendState();
}

/**
 *	kbRenderer_DX11::RenderMousePickerIds
 */
void kbRenderer_DX11::RenderMousePickerIds() {
	START_SCOPED_RENDER_TIMER( RENDER_ENTITYID )

	const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(MOUSE_PICKER_BUFFER)->m_pRenderTargetView, color );
	m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = ( float )GetRenderTarget_DX11(MOUSE_PICKER_BUFFER)->GetWidth();
	viewport.Height = ( float )GetRenderTarget_DX11(MOUSE_PICKER_BUFFER)->GetHeight();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );
	m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(MOUSE_PICKER_BUFFER)->m_pRenderTargetView, m_pDepthStencilView );
	m_RenderState.SetDepthStencilState();
	m_RenderState.SetBlendState();

	for ( auto iter = m_pCurrentRenderWindow->GetRenderObjectMap().begin(); iter != m_pCurrentRenderWindow->GetRenderObjectMap().end(); iter++ ) {
		if ( iter->second->m_EntityId > 0 ) {
			// TODO
			kbRenderSubmesh newMesh( iter->second, 0, RP_MousePicker, 0.0f );
			RenderMesh( &newMesh, false, true );
		}
	}

	RenderDebugBillboards( true );

	for ( int i = 0; i < m_DebugModels.size(); i++ ) {
		kbRenderObject renderObject;
		renderObject.m_pModel = m_DebugModels[i].m_pModel;
		renderObject.m_Materials = m_DebugModels[i].m_Materials;
		renderObject.m_Position = m_DebugModels[i].m_Position;
		renderObject.m_Orientation = m_DebugModels[i].m_Orientation;
		renderObject.m_Scale = m_DebugModels[i].m_Scale;
		renderObject.m_EntityId = m_DebugModels[i].m_EntityId;
		for ( int j = 0; j < renderObject.m_pModel->NumMeshes(); j++ ) {
			kbRenderSubmesh newMesh( &renderObject, j, RP_MousePicker, 0.0f );
			RenderMesh( &newMesh, false );
		}
	}
}

/**
 *	kbRenderer_DX11::Blit
 */
void kbRenderer_DX11::Blit( kbRenderTexture *const inSrc, kbRenderTexture *const inDest ) {
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	kbShader *const pShader = m_pDebugShader;

	kbRenderTexture_DX11 *const src = (kbRenderTexture_DX11*)inSrc;
	kbRenderTexture_DX11 *const dest = (kbRenderTexture_DX11*)inDest;

	if ( dest == nullptr ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &((kbRenderWindow_DX11*)m_pCurrentRenderWindow)->m_pRenderTargetView, m_pDepthStencilView );
	} else {
		m_pDeviceContext->OMSetRenderTargets( 1, &dest->m_pRenderTargetView, nullptr );
	}
	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	m_pDeviceContext->PSSetShaderResources( 0, 1, &src->m_pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );

	const kbShaderVarBindings_t & varBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );

	SetShaderMat4( "mvpMatrix", kbMat4::identity, mappedResource.pData, varBindings );
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );
}

/**
 *	kbRenderer_DX11::DrawTexture
 */
void kbRenderer_DX11::DrawTexture( ID3D11ShaderResourceView *const pShaderResourceView, const kbVec3 & pixelPosition, const kbVec3 & pixelSize, const kbVec3 & renderTargetSize ) {
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pDebugShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pDebugShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pDebugShader->GetPixelShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	
	auto & varBindings = m_pDebugShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
	
	//SShaderM
	//ShaderConstantMatrices * dataPtr = ( ShaderConstantMatrices * ) mappedResource.pData;
	//ShaderConstantMatrices sourceBuffer;
	
	kbMat4 finalMatrix;
	finalMatrix.MakeIdentity();

	kbVec3 screenSpaceSize = pixelSize;
	screenSpaceSize.MultiplyComponents( kbVec3( 1.0f / renderTargetSize.x, 1.0f / renderTargetSize.y, 0.0f ) );
	finalMatrix.MakeScale( screenSpaceSize );

	kbVec3 screenSpacePosition = pixelPosition;
	screenSpacePosition.MultiplyComponents( kbVec3( 2.0f / renderTargetSize.x, 2.0f / renderTargetSize.y, 0.0f ) );
	screenSpacePosition.AddComponents( kbVec3( -1.0f + screenSpaceSize.x, -1.0f + screenSpaceSize.y, 0.0f ) );
	screenSpacePosition.y *= -1.0f;

	finalMatrix[3] = screenSpacePosition;
	SetShaderMat4( "mvpMatrix", finalMatrix, mappedResource.pData, varBindings );	
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	
	m_pDeviceContext->Draw( 6, 0 );
}

/**
 *	kbRenderer_DX11::RenderSSAO
 */
void kbRenderer_DX11::RenderSSAO() {
	if (m_bRenderToHMD == true) {
		return;
	}

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)GetRenderTarget_DX11(ACCUMULATION_BUFFER)->GetWidth();
	viewport.Height = (float)GetRenderTarget_DX11(ACCUMULATION_BUFFER)->GetHeight();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports(1, &viewport);

	m_RenderState.SetBlendState( m_pSSAO );

	m_pDeviceContext->OMSetRenderTargets(1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetView, nullptr);
	const unsigned int stride = sizeof(vertexLayout);
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pUnitQuad, &stride, &offset);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->RSSetState(m_pDefaultRasterizerState);

	m_pDeviceContext->PSSetShaderResources(0, 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView);
	ID3D11SamplerState *const samplerStates[] = { m_pBasicSamplerState, m_pNormalMapSamplerState, m_pShadowMapSamplerState, m_pShadowMapSamplerState };
	m_pDeviceContext->PSSetSamplers( 0, 4, samplerStates );

	m_pDeviceContext->IASetInputLayout((ID3D11InputLayout*)m_pSSAO->GetVertexLayout());
	m_pDeviceContext->VSSetShader((ID3D11VertexShader *)m_pSSAO->GetVertexShader(), nullptr, 0);
	m_pDeviceContext->PSSetShader((ID3D11PixelShader *)m_pSSAO->GetPixelShader(), nullptr, 0);

	// TODO: Why isn't this handled in SetConstantBuffer
	ID3D11ShaderResourceView *const  RenderTargetViews[] = { GetRenderTarget_DX11(COLOR_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(NORMAL_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(SPECULAR_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(DEPTH_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(SHADOW_BUFFER)->m_pShaderResourceView };

	m_pDeviceContext->PSSetShaderResources( 0, 5, RenderTargetViews );

	const auto & varBindings = m_pSSAO->GetShaderVarBindings();

	kbShaderParamOverrides_t shaderParams;
	ID3D11Buffer * pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderSSAO() - Failed to map matrix buffer" );
	byte * pMappedData = (byte*)mappedResource.pData;

	SetConstantBuffer( varBindings, &shaderParams, nullptr, pMappedData );
	SetShaderMat4( "inverseViewProjection", m_pCurrentRenderWindow->GetInverseViewProjection(), pMappedData, varBindings );
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );

	m_pDeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
	m_pDeviceContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);

	// Draw
	m_pDeviceContext->Draw(6, 0);
}

/**
 *	kbRenderer_DX11::RenderBloom
 */
void kbRenderer_DX11::RenderBloom() {
	if ( m_bRenderToHMD == true ) {
		return;
	}

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = ( float )GetRenderTarget_DX11(RGBA_BUFFER)->GetWidth();
	viewport.Height = ( float )GetRenderTarget_DX11(RGBA_BUFFER)->GetHeight();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );

	m_RenderState.SetBlendState();

	///////////////////////////////
	// Gather
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(RGBA_BUFFER)->m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView );
		ID3D11SamplerState *const samplerState[] = { m_pNormalMapSamplerState };

		m_pDeviceContext->PSSetSamplers( 0, 1, samplerState );
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pBloomGatherShader->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pBloomGatherShader->GetVertexShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pBloomGatherShader->GetPixelShader(), nullptr, 0 );

		const auto & varBindings = m_pBloomGatherShader->GetShaderVarBindings();
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

		// Set constants
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );

		kbMat4 mvpMatrix;
		if ( m_bRenderToHMD ) {
			mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
		} else {
			mvpMatrix.MakeIdentity();
		}
		SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );

		m_pDeviceContext->Unmap( pConstantBuffer, 0 );

		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

		// Draw
		m_pDeviceContext->Draw( 6, 0 );
	}

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = ( float )GetRenderTarget_DX11(DOWN_RES_BUFFER)->GetWidth();
	viewport.Height = ( float )GetRenderTarget_DX11(DOWN_RES_BUFFER)->GetHeight();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );


	///////////////////////////////
	// Horizontal blur
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(DOWN_RES_BUFFER_2)->m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &GetRenderTarget_DX11(RGBA_BUFFER)->m_pShaderResourceView );
		ID3D11SamplerState * samplerState[] = { m_pNormalMapSamplerState };

		m_pDeviceContext->PSSetSamplers( 0, 1, samplerState );
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pBloomBlur->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pBloomBlur->GetVertexShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pBloomBlur->GetPixelShader(), nullptr, 0 );

		const auto & varBindings = m_pBloomBlur->GetShaderVarBindings();
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

		// Set constants
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );

		kbMat4 mvpMatrix;
		if ( m_bRenderToHMD ) {
			mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
		} else {
			mvpMatrix.MakeIdentity();
		}
		SetShaderMat4( "mvpMatrix", mvpMatrix, (byte*) mappedResource.pData, varBindings );
		SetShaderInt( "numSamples", 5, (byte*) mappedResource.pData, varBindings );

		const float texelSize = 1.0f / GetRenderTarget_DX11(DOWN_RES_BUFFER_2)->GetWidth();
		kbVec4 offsetsAndWeights[5];
		offsetsAndWeights[0].Set( 0.0f, 0.0f * texelSize, 0.22702f, 0.0f );
		offsetsAndWeights[1].Set( 0.0f, 1.0f * texelSize, 0.19459f, 0.0f );
		offsetsAndWeights[2].Set( 0.0f, 2.0f * texelSize, 0.12162f, 0.0f );
		offsetsAndWeights[3].Set( 0.0f, 3.0f * texelSize, 0.05405f, 0.0f );
		offsetsAndWeights[4].Set( 0.0f, 4.0f * texelSize, 0.01621f, 0.0f );
		SetShaderVec4Array( "offsetsAndWeights", offsetsAndWeights, 5, (byte*) mappedResource.pData, varBindings ); 
		m_pDeviceContext->Unmap( pConstantBuffer, 0 );

		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

		// Draw
		m_pDeviceContext->Draw( 6, 0 );

		ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView, GetRenderTarget_DX11(COLOR_BUFFER)->m_pShaderResourceView };
		m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
	}

	///////////////////////////////
	// Vertical blur
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(DOWN_RES_BUFFER)->m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &GetRenderTarget_DX11(DOWN_RES_BUFFER_2)->m_pShaderResourceView );
		ID3D11SamplerState * samplerState[] = { m_pNormalMapSamplerState };

		// Set constants
		const auto & varBindings = m_pBloomBlur->GetShaderVarBindings();
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

		// Set constants
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );

		kbMat4 mvpMatrix;
		if ( m_bRenderToHMD ) {
			mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
		} else {
			mvpMatrix.MakeIdentity();
		}

		SetShaderMat4( "mvpMatrix", mvpMatrix, (byte*) mappedResource.pData, varBindings );
		SetShaderInt( "numSamples", 5, (byte*) mappedResource.pData, varBindings );

		const float texelSize = 1.0f / GetRenderTarget_DX11(DOWN_RES_BUFFER_2)->GetWidth();
		kbVec4 offsetsAndWeights[5];
		offsetsAndWeights[0].Set( 0.0f * texelSize, 0.0f, 0.22702f, 0.0f );
		offsetsAndWeights[1].Set( 1.0f * texelSize, 0.0f, 0.19459f, 0.0f );
		offsetsAndWeights[2].Set( 2.0f * texelSize, 0.0f, 0.12162f, 0.0f );
		offsetsAndWeights[3].Set( 3.0f * texelSize, 0.0f, 0.05405f, 0.0f );
		offsetsAndWeights[4].Set( 4.0f * texelSize, 0.0f, 0.01621f, 0.0f );
		SetShaderVec4Array( "offsetsAndWeights", offsetsAndWeights, 5, (byte*) mappedResource.pData, varBindings ); 

		m_pDeviceContext->Unmap( pConstantBuffer, 0 );

		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

		// Draw
		m_pDeviceContext->Draw( 6, 0 );
	}

	// Final Render To Screen
	{
		const kbMat4 mvpMatrix = kbMat4::identity;

		D3D11_VIEWPORT viewport;
		viewport.Width = (float)Back_Buffer_Width;
		viewport.Height = (float)Back_Buffer_Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;

		m_pDeviceContext->RSSetViewports( 1, &viewport );
		m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pRenderTargetView, nullptr );

		ID3D11ShaderResourceView *const  RenderTargetViews[] = { GetRenderTarget_DX11(DOWN_RES_BUFFER)->m_pShaderResourceView };
		ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pSimpleAdditiveShader->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)this->m_pSimpleAdditiveShader->GetVertexShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pSimpleAdditiveShader->GetPixelShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShaderResources( 0, 1, RenderTargetViews );
		m_pDeviceContext->PSSetSamplers( 0, 1, SamplerStates );
		m_RenderState.SetBlendState( m_pSimpleAdditiveShader );

		const auto & varBindings = m_pSimpleAdditiveShader->GetShaderVarBindings();
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
		SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );
		m_pDeviceContext->Unmap( pConstantBuffer, 0 );
		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

		m_pDeviceContext->Draw( 6, 0 );
	}

	ID3D11ShaderResourceView *const nullArray[] = { nullptr };
	m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );

	m_RenderState.SetBlendState();

	viewport.Width = (float)Back_Buffer_Width;
	viewport.Height = (float)Back_Buffer_Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_pDeviceContext->RSSetViewports( 1, &viewport );
}

/**
 *	kbRenderer_DX11::RenderPostProcess
 */
void kbRenderer_DX11::RenderPostProcess() {
	START_SCOPED_RENDER_TIMER( RENDER_POST_PROCESS );

	if ( m_ViewMode == ViewMode_Wireframe ) {
		Blit( GetRenderTarget_DX11(ACCUMULATION_BUFFER), nullptr );
		return;
	} else if ( m_ViewMode == ViewMode_Color ) {
		Blit( GetRenderTarget_DX11(COLOR_BUFFER), nullptr );
	} else if ( m_ViewMode == ViewMode_Normals ) {
		Blit( GetRenderTarget_DX11(NORMAL_BUFFER), nullptr );
	} else if ( m_ViewMode == ViewMode_Specular ) {
		Blit( GetRenderTarget_DX11(SPECULAR_BUFFER), nullptr );
	} else if ( m_ViewMode == ViewMode_Depth ) {
		Blit( GetRenderTarget_DX11(DEPTH_BUFFER), nullptr );
	}

	RenderBloom();

	if ( m_bRenderToHMD == false ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &((kbRenderWindow_DX11*)m_pCurrentRenderWindow)->m_pRenderTargetView, m_pDepthStencilView );
	} else {
		ID3D11RenderTargetView *const rtv = m_OculusTexture[m_HMDPass]->GetRTV();
		m_pDeviceContext->OMSetRenderTargets( 1, &rtv, nullptr );
	}

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	if ( m_pCurrentRenderWindow->GetRenderLightMap().size() == 0 )
	{
		m_pDeviceContext->PSSetShaderResources( 0, 1, &GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView );
	}
	else
	{
		ID3D11ShaderResourceView * RenderTargetViews[] = { GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView, GetRenderTarget_DX11(DEPTH_BUFFER)->m_pShaderResourceView };
		m_pDeviceContext->PSSetShaderResources( 0, 2, RenderTargetViews );
	}

	ID3D11SamplerState * samplerState[] = { m_pNormalMapSamplerState, m_pNormalMapSamplerState };
	m_pDeviceContext->PSSetSamplers( 0, 2, samplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pUberPostProcess->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pUberPostProcess->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pUberPostProcess->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = m_pUberPostProcess->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
	
	kbMat4 mvpMatrix;
	if ( m_bRenderToHMD ) {
		mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
	} else {
		mvpMatrix.MakeIdentity();
	}

	SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );
	SetShaderMat4( "inverseProjection", m_pCurrentRenderWindow->GetInverseProjectionMatrix(), mappedResource.pData, varBindings );	
	SetShaderVec4( "fogColor", m_FogColor_RenderThread, mappedResource.pData, varBindings );
	SetShaderFloat( "fogStartDistance", m_FogStartDistance_RenderThread, mappedResource.pData, varBindings );
	SetShaderFloat( "fogEndDistance", m_FogEndDistance_RenderThread, mappedResource.pData, varBindings );
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	ID3D11ShaderResourceView * nullArray[] = { nullptr };

	m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );

	ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView, GetRenderTarget_DX11(COLOR_BUFFER)->m_pShaderResourceView };
	m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
}

/**
 *	kbRenderer_DX11::RenderConsole
 */
void kbRenderer_DX11::RenderConsole() {
	if ( m_bRenderToHMD == false ) {
		kbRenderWindow_DX11 *const pCurWindow = (kbRenderWindow_DX11*)m_pCurrentRenderWindow;
		m_pDeviceContext->OMSetRenderTargets( 1, &pCurWindow->m_pRenderTargetView, m_pDepthStencilView );
	} else {
		ID3D11RenderTargetView *const rtv = m_OculusTexture[0]->GetRTV();
		m_pDeviceContext->OMSetRenderTargets( 1, &rtv, nullptr );
	}

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pConsoleQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	ID3D11SamplerState *const samplerState[] = { m_pNormalMapSamplerState, m_pNormalMapSamplerState };
	m_pDeviceContext->PSSetSamplers( 0, 2, samplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pOpaqueQuadShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pDebugShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pDebugShader->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = m_pDebugShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );

	SetShaderMat4( "modelMatrix", kbMat4::identity, mappedResource.pData, varBindings );
	SetShaderMat4( "modelViewMatrix", kbMat4::identity, mappedResource.pData, varBindings );
	SetShaderMat4( "viewMatrix", m_pCurrentRenderWindow->GetViewMatrix(), mappedResource.pData, varBindings );

	kbMat4 mvpMatrix = kbMat4::identity;
	if ( m_bRenderToHMD ) {
		mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
	}
	SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );
	SetShaderMat4( "projection", m_pCurrentRenderWindow->GetViewProjectionMatrix(), mappedResource.pData, varBindings );
	SetShaderMat4( "inverseProjection", m_pCurrentRenderWindow->GetInverseProjectionMatrix(), mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	ID3D11ShaderResourceView * nullArray[] = { nullptr };

	m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );

	ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ GetRenderTarget_DX11(ACCUMULATION_BUFFER)->m_pShaderResourceView, GetRenderTarget_DX11(COLOR_BUFFER)->m_pShaderResourceView };
	m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
}

/**
 *	kbRenderer_DX11::LoadTexture_Internal
 */
bool kbRenderer_DX11::LoadTexture_Internal( const char * name, int index, int width, int height ) {
	if ( index < 0 || index >= Max_Num_Textures ) {
		kbError( "Error - kbRenderer_DX11::LoadTexture() - Texture out of range with index %d", index );
		return false;
	}

	if ( m_pTextures[index] != nullptr ) {
		m_pTextures[index]->Release();
		delete m_pTextures[index];
	}

	m_pTextures[index] = new kbTexture( kbString(name) );

	return true;
}

/**
 *	kbRenderer_DX11:RenderSync_Internal
 */
void kbRenderer_DX11::RenderSync_Internal() {

	// Oculus
	if ( IsRenderingToHMD() || IsUsingHMDTrackingOnly() ) {	

		m_EyeRenderDesc[0] = ovr_GetRenderDesc( m_ovrSession, ovrEye_Left, m_HMDDesc.DefaultEyeFov[0] );
		m_EyeRenderDesc[1] = ovr_GetRenderDesc( m_ovrSession, ovrEye_Right, m_HMDDesc.DefaultEyeFov[1] );

		// Get both eye poses simultaneously, with IPD offset already included. 
		ovrPosef HmdToEyePose[2] = { m_EyeRenderDesc[0].HmdToEyePose, m_EyeRenderDesc[1].HmdToEyePose };
		ovr_GetEyePoses( m_ovrSession, m_FrameNum, ovrTrue, HmdToEyePose, m_EyeRenderPose, &m_SensorSampleTime );

		for ( int iEye = 0; iEye < 2; iEye++ ) {

			const kbMat4 gameCameraMatrix = m_RenderWindowList[0]->GetCameraRotation().ToMat4();
			const kbVec3 rightVec = gameCameraMatrix[0].ToVec3();
			const kbVec3 forwardVec = kbVec3( gameCameraMatrix[2].ToVec3().x, 0.0f, gameCameraMatrix[2].ToVec3().z ).Normalized();
			float gameCameraYaw = acos( forwardVec.Dot( kbVec3::forward ) );

			const Matrix4f rollPitchYaw = Matrix4f::RotationY( kbPI + gameCameraYaw );
			const Quatf orientation = m_EyeRenderPose[iEye].Orientation;
			Matrix4f finalRollPitchYaw  = rollPitchYaw * Matrix4f(m_EyeRenderPose[iEye].Orientation);
			finalRollPitchYaw.M[0][0] *= -1.0f;
			finalRollPitchYaw.M[0][1] *= -1.0f;
			finalRollPitchYaw.M[0][2] *= -1.0f;
			const Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0,1,0));
			const Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0,0,1));
			const Vector3f shiftedEyePos = rollPitchYaw.Transform( m_EyeRenderPose[iEye].Position ) + 
										   Vector3f( m_RenderWindowList[0]->GetCameraPosition().x, m_RenderWindowList[0]->GetCameraPosition().y, m_RenderWindowList[0]->GetCameraPosition().z );
			
			const Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos - finalForward, finalUp);
			
			memcpy( &((kbRenderWindow_DX11*)m_RenderWindowList[0])->m_EyeMatrices[iEye], &view, sizeof( Matrix4f ) );
			((kbRenderWindow_DX11*)m_RenderWindowList[0])->m_EyeMatrices[iEye].TransposeSelf();
		}
	}

	extern kbConsoleVariable g_ShowPerfTimers;
	if ( g_ShowPerfTimers.GetBool() ) {

		float curY = 0.1f;
		curY += g_DebugLineSpacing;
	
		std::string gpuTimings = "GPU Timings";
		g_pRenderer->DrawDebugText( gpuTimings, 0.55f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green );
		curY += g_DebugLineSpacing;

		for ( int i = 1; i < kbGPUTimeStamp::GetNumTimeStamps(); i++, curY += g_DebugLineSpacing ) {
			std::string timing = kbGPUTimeStamp::GetTimeStampName(i).stl_str();
			timing += ": ";
			std::stringstream stream;
			stream << std::fixed << std::setprecision(3) << kbGPUTimeStamp::GetTimeStampMS(i);
			timing += stream.str();
			g_pRenderer->DrawDebugText( timing, 0.55f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green );
		}
	}

	kbGPUTimeStamp::UpdateFrameNum();
	m_FrameNum++;
}

kbString g_BuiltInShaderParams[] = {
	"billboardedModelMatrix",
	"modelMatrix",
	"modelViewMatrix",
	"viewMatrix",
	"mvpMatrix",
	"projection",
	"inverseProjection",
	"inverseViewProjection",
	"lightMatrix",
	"splitDistances",
	"lightDirection",
	"lightPosition",
	"viewProjection",
	"vpMatrix",
	"modelMatrix",
	"cameraPos",
	"viewProjection",
	"boneList",
	"time"
};

void kbRenderer_DX11::ReadShaderFile( std::string & shaderText, kbShaderVarBindings_t *const pShaderBindings ) {
	
	std::string::size_type n = shaderText.find( "cbuffer" );
	if ( n == std::string::npos ) {
		return;
	}

	const std::string::size_type startBlock = shaderText.find( '{', n );
	if ( startBlock == std::string::npos ) {
		return;
	}

	const std::string::size_type endBlock = shaderText.find( '}', startBlock );
	if ( endBlock == std::string::npos ) {
		return;
	}

	std::string bufferBlock( shaderText.begin() + startBlock + 1, shaderText.begin() + endBlock );
	std::vector<std::string> constantBufferStrings;
	const std::string delimiters = "\t ;\n()";

    std::string::size_type startPos = bufferBlock.find_first_not_of( delimiters, 0 );
    std::string::size_type endPos = bufferBlock.find_first_of( delimiters, startPos );

    while ( startPos != std::string::npos || endPos != std::string::npos ) {

        constantBufferStrings.push_back( bufferBlock.substr( startPos, endPos - startPos ) );

        startPos = bufferBlock.find_first_not_of( delimiters, endPos );
        endPos = bufferBlock.find_first_of( delimiters, startPos );
    }

	const int sizeofBuiltInParams = sizeof( g_BuiltInShaderParams );
	const int sizeofSTDString = sizeof( kbString );
	const int numBuiltInParams = sizeofBuiltInParams / sizeofSTDString;

	// Param defaults
	kbTexture *const pWhiteTex = (kbTexture*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Textures/white.bmp", true );
	kbTexture *const pBlackTex = (kbTexture*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Textures/black.bmp", true );
	kbTexture *const pDefaultNormal = (kbTexture*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Textures/defaultNormal.bmp", true );
	kbTexture *const pNoiseTex = (kbTexture*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Textures/noise.jpg", true );

	size_t currOffset = 0;
	for ( int i = 0; i < constantBufferStrings.size(); i += 2 ) {

		currOffset = ( currOffset + 15 ) & 0xfffffff0;

		std::string & varName = constantBufferStrings[i+1];
		int count = 1;
		std::string::size_type arrayStart = varName.find( '[' );
		std::string::size_type arrayEnd = varName.find( ']' );

		if ( arrayStart != std::string::npos && arrayEnd != std::string::npos ) {
			std::string sCount = varName.substr( arrayStart + 1, arrayEnd - 2 );
			count = std::stoi( sCount );
			varName.resize( arrayStart );
		}

		bool bIsUserDefinedVar = true;
		kbString varNameString = kbString( varName );
		for ( int iParamCheck = 0; iParamCheck < numBuiltInParams; iParamCheck++ ) {
			if ( varNameString == g_BuiltInShaderParams[iParamCheck] ) {
				bIsUserDefinedVar = false;
			}
		}
		pShaderBindings->m_VarBindings.push_back( kbShaderVarBindings_t::binding_t( varName, currOffset, bIsUserDefinedVar ) );

		if ( constantBufferStrings[i] == "matrix" || constantBufferStrings[i] == "float4x4" ) {
			currOffset += 64 * count;
		} else if ( constantBufferStrings[i] == "float4" ) {
			currOffset += 16 * count;
		} else {
			currOffset += 4 * count;
		}
	}

	pShaderBindings->m_ConstantBufferSizeBytes = ( currOffset + 15 ) & 0xfffffff0;

    // Bind textures
    std::string::size_type texturePos = shaderText.find( "Texture2D" );
    while ( texturePos != std::string::npos ) {

        auto startPos = shaderText.find_first_not_of( delimiters, texturePos + 9 );
        auto endPos = shaderText.find_first_of( delimiters, startPos );

        if ( startPos == std::string::npos || endPos == std::string::npos ) {
            break;
        }

		kbShaderVarBindings_t::textureBinding_t textureBinding;
		textureBinding.m_pDefaultTexture = nullptr;
		textureBinding.m_pDefaultRenderTexture = nullptr;
		textureBinding.m_TextureName = shaderText.substr( startPos, endPos - startPos );

		// Check for default values
		if ( shaderText[endPos] == '(' ) {
			auto defaultValStart = endPos + 1;
			auto defaultValEnd = shaderText.find_first_of( delimiters, defaultValStart );
			if ( defaultValEnd == std::string::npos || shaderText[defaultValEnd] != ')' ) {
				break;
			}

			std::string defaultTexture = shaderText.substr( defaultValStart, defaultValEnd - defaultValStart );
			std::transform( defaultTexture.begin(), defaultTexture.end(), defaultTexture.begin(), ::tolower );

			defaultValStart--;
			while( defaultValStart <= defaultValEnd ) {
				shaderText[defaultValStart++] = ' ';
			}

			if ( defaultTexture == "white" ) {
				textureBinding.m_pDefaultTexture = pWhiteTex;
			} else if ( defaultTexture == "black" ) {
				textureBinding.m_pDefaultTexture = pBlackTex;
			} else if ( defaultTexture == "defaultnormal" ) {
				textureBinding.m_pDefaultTexture = pDefaultNormal;
			} else if ( defaultTexture == "colorbuffer" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[COLOR_BUFFER];
			} else if ( defaultTexture == "normalbuffer" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[NORMAL_BUFFER];
			} else if ( defaultTexture == "depthbuffer" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[DEPTH_BUFFER];
			} else if ( defaultTexture == "specularbuffer" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[SPECULAR_BUFFER];
			} else if ( defaultTexture == "shadowbuffer" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[SHADOW_BUFFER];
			} else if ( defaultTexture == "maxhalf" ) {
				textureBinding.m_pDefaultRenderTexture = m_pRenderTargets[MAX_HALF_BUFFER];
			} else if ( defaultTexture == "noise" ) {
				textureBinding.m_pDefaultTexture = pNoiseTex;
			} else {
				kbWarning( "Default texture %s not found", defaultTexture.c_str() );
			}
		}
		pShaderBindings->m_Textures.push_back( textureBinding );

        texturePos = shaderText.find( "Texture2D", texturePos + 1 );
    }
}

/**
 *	kbRenderer_DX11::LoadShader
 */
void kbRenderer_DX11::LoadShader( const std::string & fileName, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader, ID3D11PixelShader *& pixelShader, 
								  ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, const std::string & pixelShaderFunc,
								  kbShaderVarBindings_t * pShaderBindings ) {

	std::ifstream shaderFile;
	shaderFile.open( fileName.c_str(), std::fstream::in );
	std::string shaderText( ( std::istreambuf_iterator<char>(shaderFile) ), std::istreambuf_iterator<char>() );
	shaderFile.close();

	kbTextParser shaderTextParser( shaderText );
	shaderTextParser.RemoveComments();

	CreateShaderFromText( fileName, shaderText, vertexShader, geometryShader, pixelShader, vertexLayout, vertexShaderFunc, pixelShaderFunc, pShaderBindings );
}
/**
 *	kbRenderer_DX11::CreateShaderFromText
 */

void kbRenderer_DX11::CreateShaderFromText( const std::string & fileName, const std::string & inShaderText, ID3D11VertexShader *& vertexShader, ID3D11GeometryShader *& geometryShader,
											ID3D11PixelShader *& pixelShader, ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, 
											const std::string & pixelShaderFunc, struct kbShaderVarBindings_t * pShaderBindings ) {

	std::string shaderText = inShaderText;

	HRESULT hr;
	struct shaderBlobs_t {
		~shaderBlobs_t() {
			SAFE_RELEASE(errorMessage)
			SAFE_RELEASE(vertexShaderBuffer)
			SAFE_RELEASE(pixelShaderBuffer)
            SAFE_RELEASE(geometryShaderBuffer)
		}

		ID3D10Blob * errorMessage = nullptr;
		ID3D10Blob * vertexShaderBuffer = nullptr;
		ID3D10Blob * pixelShaderBuffer = nullptr;
        ID3D10Blob * geometryShaderBuffer = nullptr;
	} localBlobs;


		ReadShaderFile( shaderText, pShaderBindings );

		if ( pShaderBindings->m_ConstantBufferSizeBytes > 0 ) {
			const UINT desiredByteWidth = ( pShaderBindings->m_ConstantBufferSizeBytes + 15 ) & 0xfffffff0;
			if ( m_ConstantBuffers.find( desiredByteWidth ) == m_ConstantBuffers.end() ) {
				D3D11_BUFFER_DESC matrixBufferDesc;
				matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;

				matrixBufferDesc.ByteWidth = desiredByteWidth;
				matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				matrixBufferDesc.MiscFlags = 0;
				matrixBufferDesc.StructureByteStride = 0;

				ID3D11Buffer * pConstantBuffer = nullptr;
				hr = m_pD3DDevice->CreateBuffer( &matrixBufferDesc, nullptr, &pConstantBuffer );
				kbErrorCheck( SUCCEEDED(hr), "Failed to create matrix buffer" );

				m_ConstantBuffers.insert( std::pair<size_t, ID3D11Buffer*>( desiredByteWidth, pConstantBuffer ) );
			}
		}
//	}

	// Compile vertex shader
	const UINT shaderFlags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_ALL_RESOURCES_BOUND | D3DCOMPILE_WARNINGS_ARE_ERRORS;

	int numTries = 0;
	do {
		numTries++;

		SAFE_RELEASE( localBlobs.errorMessage );
		hr = D3DCompile( shaderText.c_str(), shaderText.length(), nullptr, nullptr, nullptr, vertexShaderFunc.c_str(), "vs_5_0", shaderFlags, 0, &localBlobs.vertexShaderBuffer, &localBlobs.errorMessage );
		if ( FAILED(hr)  ) {
			Sleep( 250 );
			SAFE_RELEASE( localBlobs.vertexShaderBuffer );

		}
	} while ( FAILED(hr) && numTries < 4 );

	if ( FAILED(hr) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to load vertex shader : %s\n%s", fileName.c_str() , ( localBlobs.errorMessage != nullptr ) ? ( localBlobs.errorMessage->GetBufferPointer() ) : ( "No error message given" ) );
		return;
	}
	SAFE_RELEASE( localBlobs.errorMessage );

    // Geometry Shader
    if ( shaderText.find( "void geometryShader") != std::string::npos ) {
        numTries = 0;
        do {
		    numTries++;

		    SAFE_RELEASE( localBlobs.errorMessage );
		    hr = D3DCompile( shaderText.c_str(), shaderText.length(), nullptr, nullptr, nullptr, "geometryShader", "gs_5_0", shaderFlags, 0, &localBlobs.geometryShaderBuffer, &localBlobs.errorMessage );
		    if ( FAILED(hr)  ) {
			    Sleep( 250 );
			    SAFE_RELEASE( localBlobs.geometryShaderBuffer );

		    }
	    } while ( FAILED(hr) && numTries < 4 );

        if ( SUCCEEDED(hr) ) {
    	    hr = m_pD3DDevice->CreateGeometryShader( localBlobs.geometryShaderBuffer->GetBufferPointer(), localBlobs.geometryShaderBuffer->GetBufferSize(), nullptr, &geometryShader );
	        if ( FAILED(hr) ) {
		        kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create geometry shader %s", fileName.c_str() );
		        SAFE_RELEASE( vertexShader );
		        return;
	        }
        } else {
			kbWarning( "kbRenderer_DX11::LoadShader() - Failed to load geometry shader : %s\n%s", fileName.c_str() , ( localBlobs.errorMessage != nullptr ) ? ( localBlobs.errorMessage->GetBufferPointer() ) : ( "No error message given" ) );
		}
    }
    SAFE_RELEASE( localBlobs.errorMessage );

	// Compile pixel shader
	numTries = 0;
	do {
		numTries++;

		SAFE_RELEASE( localBlobs.errorMessage )
		hr = D3DCompile( shaderText.c_str(), shaderText.length(), nullptr, nullptr, nullptr, pixelShaderFunc.c_str(), "ps_5_0", shaderFlags, 0, &localBlobs.pixelShaderBuffer, &localBlobs.errorMessage );
		if ( FAILED(hr) ) {
			Sleep( 250 );
			SAFE_RELEASE( localBlobs.pixelShaderBuffer );
		}
	} while ( FAILED(hr) && numTries < 4 );

	if ( FAILED(hr) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to load pixel shader : %s\n%s", fileName.c_str(), ( localBlobs.errorMessage != nullptr ) ? localBlobs.errorMessage->GetBufferPointer() : ( "No error message given " ) );
		return;
	}

	SAFE_RELEASE( localBlobs.errorMessage );

	hr = m_pD3DDevice->CreateVertexShader( localBlobs.vertexShaderBuffer->GetBufferPointer(), localBlobs.vertexShaderBuffer->GetBufferSize(), nullptr, &vertexShader );
	if ( FAILED(hr) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create vertex shader %s", fileName.c_str() );
		return;
	}

	hr = m_pD3DDevice->CreatePixelShader( localBlobs.pixelShaderBuffer->GetBufferPointer(), localBlobs.pixelShaderBuffer->GetBufferSize(), nullptr, &pixelShader );
	if ( FAILED(hr) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create pixel shader %s", fileName.c_str() );
		SAFE_RELEASE( vertexShader );
		return;
	}

	std::vector<D3D11_INPUT_ELEMENT_DESC> polygonLayout;

	if ( fileName.find( "grass" ) != std::string::npos ) {
		polygonLayout.insert( polygonLayout.begin(), 3, D3D11_INPUT_ELEMENT_DESC() );

		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "BLENDINDICES";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

	} else if ( fileName.find("particle") != std::string::npos ) {
		polygonLayout.insert( polygonLayout.begin(), 6, D3D11_INPUT_ELEMENT_DESC() );

		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "COLOR";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		polygonLayout[3].SemanticName = "TEXCOORD";
		polygonLayout[3].SemanticIndex = 1;
		polygonLayout[3].Format = DXGI_FORMAT_R32G32_FLOAT;//DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[3].InputSlot = 0;
		polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[3].InstanceDataStepRate = 0;

		polygonLayout[4].SemanticName = "TEXCOORD";
		polygonLayout[4].SemanticIndex = 2;
		polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		polygonLayout[4].InputSlot = 0;
		polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[4].InstanceDataStepRate = 0;

		polygonLayout[5].SemanticName = "TEXCOORD";
		polygonLayout[5].SemanticIndex = 3;
		polygonLayout[5].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[5].InputSlot = 0;
		polygonLayout[5].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[5].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[5].InstanceDataStepRate = 0;
	}
	else if (fileName.find("skinned") != std::string::npos || vertexShaderFunc.find( "skin" ) != std::string::npos || pShaderBindings->ContainsBinding( "boneList" ) ) {
		polygonLayout.insert( polygonLayout.begin(), 5, D3D11_INPUT_ELEMENT_DESC() );

		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "BLENDINDICES";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		polygonLayout[3].SemanticName = "NORMAL";
		polygonLayout[3].SemanticIndex = 0;
		polygonLayout[3].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[3].InputSlot = 0;
		polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[3].InstanceDataStepRate = 0;

		polygonLayout[4].SemanticName = "BLENDWEIGHT";
		polygonLayout[4].SemanticIndex = 0;
		polygonLayout[4].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[4].InputSlot = 0;
		polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[4].InstanceDataStepRate = 0;
	}
	else {
		polygonLayout.insert( polygonLayout.begin(), 5, D3D11_INPUT_ELEMENT_DESC() );

		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "COLOR";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		polygonLayout[3].SemanticName = "NORMAL";
		polygonLayout[3].SemanticIndex = 0;
		polygonLayout[3].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[3].InputSlot = 0;
		polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[3].InstanceDataStepRate = 0;

		polygonLayout[4].SemanticName = "TANGENT";
		polygonLayout[4].SemanticIndex = 0;
		polygonLayout[4].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		polygonLayout[4].InputSlot = 0;
		polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[4].InstanceDataStepRate = 0;
	}

	hr = m_pD3DDevice->CreateInputLayout( &polygonLayout[0], (UINT)polygonLayout.size(), localBlobs.vertexShaderBuffer->GetBufferPointer(), localBlobs.vertexShaderBuffer->GetBufferSize(), &vertexLayout );
	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create input layout for %s", fileName.c_str() );

		SAFE_RELEASE( vertexShader )
		SAFE_RELEASE( pixelShader );
		SAFE_RELEASE( vertexLayout );
	}
}

/**
 *	kbRenderer_DX11::RenderScreenSpaceQuads
 */
void kbRenderer_DX11::RenderScreenSpaceQuads() {

	if ( m_ScreenSpaceQuads_RenderThread.size() == 0 ) {
		return;
	}

	m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

	for ( int i = 0; i < m_ScreenSpaceQuads_RenderThread.size(); i++ ) {
		RenderScreenSpaceQuadImmediate( m_ScreenSpaceQuads_RenderThread[i].m_Pos.x, 
										m_ScreenSpaceQuads_RenderThread[i].m_Pos.y,
										m_ScreenSpaceQuads_RenderThread[i].m_Size.x,
										m_ScreenSpaceQuads_RenderThread[i].m_Size.y,
										m_ScreenSpaceQuads_RenderThread[i].m_TextureIndex,
										m_ScreenSpaceQuads_RenderThread[i].m_pShader );
	}


	m_RenderState.SetDepthStencilState();

	m_RenderState.SetBlendState();
}

/**
 *	kbRenderer_DX11::RenderScreenSpaceImmediate
 */
void kbRenderer_DX11::RenderScreenSpaceQuadImmediate( const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader * pShader ) {
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;
	const float xScale = size_x / m_RenderWindowList[0]->GetFViewPixelWidth();
	const float yScale =  size_y / m_RenderWindowList[0]->GetFViewPixelHeight();
	const float xPos = xScale + start_x / m_RenderWindowList[0]->GetFViewPixelHalfWidth();
	const float yPos = yScale + start_y / m_RenderWindowList[0]->GetFViewPixelHalfHeight();

	if ( m_pTextures[textureIndex] == nullptr ) {
		return;
	}

	if ( pShader == nullptr ) {
		pShader = m_pDebugShader;
	}

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[textureIndex]->GetGPUTexture();

	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );
	m_RenderState.SetBlendState( pShader );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	const auto & varBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderScreenSpaceQuadImmediate() - Failed to map matrix buffer" );
	
	kbMat4 mvpMatrix;
	
	mvpMatrix.MakeIdentity();
	mvpMatrix[0][0] = xScale;
	mvpMatrix[1][1] = yScale;
	mvpMatrix[3][0] = xPos - 1.0f;
	mvpMatrix[3][1] = 1.0f - yPos;
	SetShaderMat4( "mvpMatrix", mvpMatrix, (byte*) mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );
}

/**
 *	kbRenderer_DX11::RenderMesh
 */
void kbRenderer_DX11::RenderMesh( const kbRenderSubmesh *const pRenderMesh, const bool bShadowPass, const bool bSkipMeshBlendSettings ) {

	const kbRenderObject * pRenderObject = pRenderMesh->GetRenderObject();
	const kbModel *const pModel = pRenderObject->m_pModel;

	kbErrorCheck( pRenderObject != nullptr && pRenderObject->m_pModel != nullptr, "kbRenderer_DX11::RenderMesh() - no model found" );
	//kbErrorCheck( pModel->GetMaterials().size() > 0, "kbRenderer_DX11::RenderMesh() - No materials found for model %s", pRenderObject->m_pModel->GetFullName().c_str() );

	const UINT vertexStride = pModel->VertexStride();
	const UINT vertexOffset = 0;

	ID3D11Buffer *const pVertexBuffer = ( ID3D11Buffer * const ) pModel->m_VertexBuffer.GetBufferPtr();
	m_pDeviceContext->IASetVertexBuffers( 0, 1, &pVertexBuffer, &vertexStride, &vertexOffset );

    if ( pModel->IsPointCloud() ) {
	    m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    } else {
	    ID3D11Buffer *const pIndexBuffer = ( ID3D11Buffer * const ) pModel->m_IndexBuffer.GetBufferPtr();
	    m_pDeviceContext->IASetIndexBuffer( pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	    m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    }

	const kbModel::mesh_t & pMesh = pModel->GetMeshes()[pRenderMesh->GetMeshIdx()];
	const kbMaterial & meshMaterial = pModel->GetMaterials()[pMesh.m_MaterialIndex];

	// Get Shader
	const kbShader * pShader = meshMaterial.GetShader();
	
	if ( bShadowPass ) {
		if ( pRenderObject->m_bIsSkinnedModel ) {
			pShader = m_pSkinnedDirectionalLightShadowShader;
		} else {
			pShader = m_pDirectionalLightShadowShader;
		}
	
	} else {
	
		if ( pRenderObject->m_Materials.size() > 0 && pRenderObject->m_Materials.size() > pRenderMesh->GetMeshIdx() ) {
			pShader = pRenderObject->m_Materials[pRenderMesh->GetMeshIdx()].m_pShader;
		}

		if ( pShader == nullptr || pShader->GetPixelShader() == nullptr ) {
			pShader = m_pMissingShader;
		}
	}

	if ( bSkipMeshBlendSettings == false ) {
		m_RenderState.SetBlendState( pShader );
	}

	if ( m_ViewMode == ViewMode_Wireframe ) {
		m_pDeviceContext->RSSetState( m_pWireFrameRasterizerState );
	} else if ( pShader->GetCullMode() == CullMode_BackFaces ) {
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
	} else if ( pShader->GetCullMode() == CullMode_None ) {
		m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );
	} else if ( pShader->GetCullMode() == CullMode_FrontFaces ) {
		m_pDeviceContext->RSSetState( m_pFrontFaceCullingRasterizerState );
	}  else {
		kbError( "kbRenderer_DX11::RenderMesh() - Unsupported culling mode" );
	}

	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	
	if ( pRenderMesh->GetRenderPass() == RP_MousePicker ) {
	
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pMousePickerIdShader->GetPixelShader(), nullptr, 0 );
	
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( 32 );

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
		UINT *const pEntityId = (UINT*)mappedResource.pData;
		*pEntityId = pRenderObject->m_EntityId;

		UINT *const pGroupId = pEntityId + 1;
		*pGroupId = (UINT)pRenderMesh->GetMeshIdx();

		m_pDeviceContext->Unmap( pConstantBuffer, 0 );
		m_pDeviceContext->PSSetConstantBuffers( 1, 1, &pConstantBuffer );
	
	} else {
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );
	}
	
    m_pDeviceContext->GSSetShader( (ID3D11GeometryShader *) pShader->GetGeometryShader(), nullptr, 0 );
	ID3D11SamplerState *const  SamplerStates[] = { m_pNormalMapSamplerState, m_pNormalMapSamplerState };	// todo: Grass uses this for sampling time
	m_pDeviceContext->GSSetSamplers( 0, 2, SamplerStates );

	// Set textures
	const std::vector<const kbTexture*> & textureList = meshMaterial.GetTextureList();

	for ( int i = 0; i < textureList.size(); i++ ) {
		ID3D11ShaderResourceView *const texture = ( textureList[i] != nullptr ) ? (ID3D11ShaderResourceView *)textureList[i]->GetGPUTexture() : (nullptr);
		m_pDeviceContext->PSSetShaderResources( i, 1, &texture );
		m_pDeviceContext->PSSetSamplers( i, 1, &m_pBasicSamplerState );
	}

	// Get a valid constant buffer and bind the kbShader's vars to it
	const kbShaderVarBindings_t & shaderVarBindings = pShader->GetShaderVarBindings();

	ID3D11Buffer * pConstantBuffer = nullptr;
	if ( pRenderObject->m_Materials.size() > 0 ) {
		if ( pRenderObject->m_Materials.size() > pRenderMesh->GetMeshIdx() ) {
			pConstantBuffer = SetConstantBuffer( shaderVarBindings, &pRenderObject->m_Materials[pRenderMesh->GetMeshIdx()], pRenderObject, nullptr );
		} else {
			pConstantBuffer = SetConstantBuffer( shaderVarBindings, &pRenderObject->m_Materials[0], pRenderObject, nullptr );
		}
	}

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

    if ( pShader->GetGeometryShader() != nullptr ) {
        m_pDeviceContext->GSSetConstantBuffers( 0, 1, &pConstantBuffer );
    }

    if ( pModel->IsPointCloud() == true ) {
    	m_pDeviceContext->Draw( (UINT)pModel->NumVertices(), 0 );
    } else {
    	m_pDeviceContext->DrawIndexed( pMesh.m_NumTriangles * 3, pMesh.m_IndexBufferIndex, 0 );
    }
}

/*
 *	kbRenderer_DX11::RenderPretransformedDebugLines
 */
void kbRenderer_DX11::RenderPretransformedDebugLines() {
	if ( m_DebugPreTransformedLines.size() == 0 ) {
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( m_DebugPreTransformedVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderPretransformedDebugLines() - Failed to map debug lines" );

	vertexLayout * vertices = ( vertexLayout * ) mappedResource.pData;
	memcpy( vertices, m_DebugPreTransformedLines.data(), sizeof( vertexLayout ) * m_DebugPreTransformedLines.size() );

	m_pDeviceContext->Unmap( m_DebugPreTransformedVertexBuffer, 0 );
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_DebugPreTransformedVertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[0]->GetGPUTexture();
	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pDebugShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader*) m_pDebugShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader*) m_pDebugShader->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = m_pDebugShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderPretransformedDebugLines() - Failed to map matrix buffer" );

	SetShaderMat4( "mvpMatrix", kbMat4::identity, mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->Draw( ( UINT )m_DebugPreTransformedLines.size(), 0 );
	
	m_DebugPreTransformedLines.clear();	
}

/**
 *	kbRenderer_DX11::RenderDebugLines
 */
void kbRenderer_DX11::RenderDebugLines() {
	
	if ( m_DebugLines.size() == 0 ) {
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( m_DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderDebugLines() - Failed to map debug vertex buffer" );

	vertexLayout * vertices = ( vertexLayout * ) mappedResource.pData;
	memcpy( vertices, m_DebugLines.data(), sizeof( vertexLayout ) * m_DebugLines.size() );

	m_pDeviceContext->Unmap( m_DebugVertexBuffer, 0 );
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_DebugVertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[0]->GetGPUTexture();
	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pDebugShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader*) m_pDebugShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader*) m_pDebugShader->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = m_pDebugShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
	hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderDebugLines() - Failed to map matrix buffer" );

	SetShaderMat4( "mvpMatrix", m_pCurrentRenderWindow->GetViewProjectionMatrix(), mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( ( UINT )m_DebugLines.size(), 0 );
}

/*
 *	kbRenderer_DX11::RenderDebugBillboards
 */
void kbRenderer_DX11::RenderDebugBillboards( const bool bIsEntityIdPass ) {
	
	if ( m_DebugBillboards.size() == 0 || m_bDebugBillboardsEnabled == false ) {
		return;
	}

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout *)m_pDebugShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pDebugShader->GetVertexShader(), nullptr, 0 );

	kbShader * pShader = nullptr;
	if ( bIsEntityIdPass ) {
		pShader = m_pMousePickerIdShader;

	} else {
		pShader = m_pDebugShader;

	}

	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );

	const auto varBindings = pShader->GetShaderVarBindings();
	for ( int i = 0; i < m_DebugBillboards.size(); i++ ) {
		debugDrawObject_t & currBillBoard = m_DebugBillboards[i];
		ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView *)m_pTextures[currBillBoard.m_TextureIndex]->GetGPUTexture();
		m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::RenderDebugBillboards() - Failed to map constans buffer" );

		byte *const pByteBuffer = (byte*) mappedResource.pData;

		const kbMat4 preRotationMatrix = m_pCurrentRenderWindow->GetCameraRotation().ToMat4();
		kbMat4 mvpMatrix;
		mvpMatrix.MakeScale( currBillBoard.m_Scale );
		mvpMatrix[3] = currBillBoard.m_Position;
		mvpMatrix = preRotationMatrix * mvpMatrix * m_pCurrentRenderWindow->GetViewProjectionMatrix();
		SetShaderMat4( "mvpMatrix", mvpMatrix, pByteBuffer, pShader->GetShaderVarBindings() );
		m_pDeviceContext->Unmap( pConstantBuffer, 0 );
		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

		if ( bIsEntityIdPass ) {
			ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( 16 );

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
			UINT * pEntityId = (UINT*)mappedResource.pData;
			*pEntityId = currBillBoard.m_EntityId;

			UINT * pGroupId = pEntityId + 1;
			pGroupId = 0;

			m_pDeviceContext->Unmap( pConstantBuffer, 0 );

			m_pDeviceContext->PSSetConstantBuffers( 1, 1, &pConstantBuffer );
		}
		m_pDeviceContext->Draw( 6, 0 );
	}
}

/**
 *	kbRenderer_DX11::GetEntityIdAtScreenPosition
 */
kbVec2i kbRenderer_DX11::GetEntityIdAtScreenPosition( const uint x, const uint y ) {
	kbErrorCheck( m_RenderThreadSync == 0, "kbRenderer_DX11::GetEntityIdAtScreenPosition() - Function can only be called during the sync." );

	D3D11_BOX box;
	box.left = 0;
	box.right = Back_Buffer_Width;
	box.top = 0;
	box.bottom = Back_Buffer_Height;
	box.front = 0;
	box.back = 1;
	
	m_pDeviceContext->CopyResource( m_pOffScreenRenderTargetTexture, GetRenderTarget_DX11(eReservedRenderTargets::MOUSE_PICKER_BUFFER)->m_pRenderTargetTexture );
	
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_pDeviceContext->Map( m_pOffScreenRenderTargetTexture, 0, D3D11_MAP_READ, 0, &mappedResource );
	
	byte * pixelData = (byte*)mappedResource.pData;
	uint * hitPixel = (uint*)(pixelData + ( y * mappedResource.RowPitch ) + ( x * sizeof(uint) ) );
	ushort * r = (ushort*)hitPixel;
	ushort * g = r + 1;

	kbVec2i retVal( *r, *g );
	m_pDeviceContext->Unmap( m_pOffScreenRenderTargetTexture, 0 );

	return retVal;
}

/**
 *	kbRenderer_DX11::RT_SetRenderTarget
 */
void kbRenderer_DX11::RT_SetRenderTarget( kbRenderTexture *const pRenderTexture ) {
	
	ID3D11ShaderResourceView * pNullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
											   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	// Unbind all textures
	m_pDeviceContext->VSSetShaderResources( 0, 16, pNullSRVs );
	m_pDeviceContext->GSSetShaderResources( 0, 16, pNullSRVs );
	m_pDeviceContext->PSSetShaderResources( 0, 16, pNullSRVs );

	m_pDeviceContext->OMSetRenderTargets(1, &((kbRenderTexture_DX11*)pRenderTexture)->m_pRenderTargetView, nullptr );

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float) pRenderTexture->GetWidth();
	viewport.Height = (float) pRenderTexture->GetHeight();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );
}

/**
 *	kbRenderer_DX11::RT_ClearRenderTarget
 */
void kbRenderer_DX11::RT_ClearRenderTarget( kbRenderTexture *const pRenderTexture, const kbColor & color ) {

	m_pDeviceContext->ClearRenderTargetView( ((kbRenderTexture_DX11*)pRenderTexture)->m_pRenderTargetView, &color.x );
}

/**
 *	kbRenderer_DX11::RT_RenderMesh
 */
void kbRenderer_DX11::RT_RenderMesh( const kbModel *const pModel, kbShader * pShader, const kbShaderParamOverrides_t *const pShaderParams ) {

	if ( pShader == nullptr || pShader->GetVertexShader() == nullptr || pShader->GetPixelShader() == nullptr ) {
		pShader = m_pMissingShader;
	}

	const UINT vertexStride = pModel->VertexStride();
	const UINT vertexOffset = 0;

	ID3D11Buffer *const pVertexBuffer = (ID3D11Buffer * const)pModel->m_VertexBuffer.GetBufferPtr();
	m_pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &vertexStride, &vertexOffset);

	ID3D11Buffer *const pIndexBuffer = (ID3D11Buffer * const)pModel->m_IndexBuffer.GetBufferPtr();
	m_pDeviceContext->IASetIndexBuffer( pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );

	m_pDeviceContext->GSSetShader( (ID3D11GeometryShader *)pShader->GetGeometryShader(), nullptr, 0 );
	m_pDeviceContext->GSSetSamplers(0, 1, &m_pBasicSamplerState );

	m_RenderState.SetBlendState( pShader );

    // Bind textures
	const kbShaderVarBindings_t & shaderVarBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = SetConstantBuffer( shaderVarBindings, pShaderParams, nullptr, nullptr );

	if ( pConstantBuffer != nullptr ) {
		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );
	}

	for ( int i = 0; i < pModel->NumMeshes(); i++ ) {

		const kbModel::mesh_t & pMesh = pModel->GetMeshes()[i];
		const kbMaterial & meshMaterial = pModel->GetMaterials()[pMesh.m_MaterialIndex];
		m_pDeviceContext->DrawIndexed( pMesh.m_NumTriangles * 3, pMesh.m_IndexBufferIndex, 0 );
	}

	m_RenderState.SetBlendState();
}

/**
 *	kbRenderer_DX11::RT_Render2DLine
 */
void kbRenderer_DX11::RT_Render2DLine( const kbVec3 & startPt, const kbVec3 & endPt, const kbColor & color, const float width, const kbShader * pShader, const kbShaderParamOverrides_t *const pShaderParamOverrides ) {

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( m_DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RT_Render2DLine() - Failed to map debug vertex buffer" );

	kbVec3 finalStartPt = kbVec3( startPt.x * 2.0f - 1.0f, -( ( startPt.y * 2.0f) - 1.0f ), 0.0f );
	kbVec3 finalEndPt = kbVec3( endPt.x * 2.0f - 1.0f, -( ( endPt.y * 2.0f) - 1.0f ), 0.0f );
	kbVec3 perpLine( finalEndPt.x - finalStartPt.x, finalEndPt.y - finalStartPt.y, 0.0f );

	finalStartPt.z = startPt.z;
	finalEndPt.z = endPt.z;

	perpLine.Normalize();
	float swap = perpLine.x;
	perpLine.x = perpLine.y;
	perpLine.y = -swap;
	perpLine *= width * 0.5f;

	vertexLayout *const vertices = (vertexLayout *) mappedResource.pData;
	vertices[0].position = finalStartPt + perpLine;
	vertices[0].SetColor( color );
	vertices[0].uv.Set( 1.0f, 0.0f );

	vertices[1].position = finalStartPt - perpLine;
	vertices[1].SetColor( color );
	vertices[1].uv.Set( 0.0f, 0.0f );

	vertices[2].position = finalEndPt - perpLine;
	vertices[2].SetColor( color );
	vertices[2].uv.Set( 0.0f, 1.0f );

	vertices[3].position = finalEndPt - perpLine;
	vertices[3].SetColor( color );
	vertices[3].uv.Set( 0.0f, 1.0f );
	
	vertices[4].position = finalEndPt + perpLine;
	vertices[4].SetColor( color );
	vertices[4].uv.Set( 1.0f, 1.0f );

	vertices[5].position = finalStartPt + perpLine;
	vertices[5].SetColor( color );
	vertices[5].uv.Set( 1.0f, 0.0f );

	m_pDeviceContext->Unmap( m_DebugVertexBuffer, 0 );

	const unsigned int stride = sizeof(vertexLayout);
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_DebugVertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );

	if ( pShader == nullptr ) {
		pShader = m_pDebugShader;
	}

	m_RenderState.SetBlendState( pShader );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[0]->GetGPUTexture();
	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader*) pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader*) pShader->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = SetConstantBuffer( varBindings, pShaderParamOverrides, nullptr, nullptr );

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
}

/**
 *	kbRenderer_DX11::RT_CopyRenderTarget
 */
void kbRenderer_DX11::RT_CopyRenderTarget( kbRenderTexture *const pSrcTexture, kbRenderTexture *const pDstTexture ) {
	kbRenderTexture_DX11 *const pSrc = (kbRenderTexture_DX11*)pSrcTexture;
	kbRenderTexture_DX11 *const pDst = (kbRenderTexture_DX11*)pDstTexture;

	m_pDeviceContext->CopyResource( pDst->m_pRenderTargetTexture, pSrc->m_pRenderTargetTexture );
}

/**
 *	kbRenderer_DX11::RT_Render2DQuad
 */
void kbRenderer_DX11::RT_Render2DQuad( const kbVec2 & origin, const kbVec2 & size, const kbColor & color, const kbShader * pShader, const struct kbShaderParamOverrides_t *const pShaderParamOverrides ) {
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( m_DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

	kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RT_Render2DQuad() - Failed to map debug vertex buffer" );

	const kbVec3 origin3D = kbVec3( (origin.x * 2.0f) - 1.0f, -((origin.y*2.0f) - 1.0f), 0.0f );

	vertexLayout *const vertices = (vertexLayout *) mappedResource.pData;
	vertices[0].position = origin3D + kbVec3( -size.x, -size.y, 0.01f );
	vertices[0].SetColor( color );
	vertices[0].uv.Set( 1.0f, 0.0f );

	vertices[1].position = origin3D + kbVec3( size.x, -size.y, 0.01f );
	vertices[1].SetColor( color );
	vertices[1].uv.Set( 0.0f, 0.0f );

	vertices[2].position = origin3D + kbVec3( size.x, size.y, 0.01f );
	vertices[2].SetColor( color );
	vertices[2].uv.Set( 0.0f, 1.0f );

	vertices[3].position = origin3D + kbVec3( size.x, size.y, 0.01f );
	vertices[3].SetColor( color );
	vertices[3].uv.Set( 0.0f, 1.0f );
	
	vertices[4].position = origin3D + kbVec3( -size.x, size.y, 0.01f );
	vertices[4].SetColor( color );
	vertices[4].uv.Set( 1.0f, 1.0f );

	vertices[5].position = origin3D + kbVec3( -size.x, -size.y, 0.01f );
	vertices[5].SetColor( color );
	vertices[5].uv.Set( 1.0f, 0.0f );

	m_pDeviceContext->Unmap( m_DebugVertexBuffer, 0 );

	const unsigned int stride = sizeof(vertexLayout);
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_DebugVertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );

	if ( pShader == nullptr ) {
		pShader = m_pDebugShader;
	}

	m_RenderState.SetBlendState( pShader );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[0]->GetGPUTexture();
	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader*) pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader*) pShader->GetPixelShader(), nullptr, 0 );

	const auto & varBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer *const pConstantBuffer = SetConstantBuffer( varBindings, pShaderParamOverrides, nullptr, nullptr );

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
}

/**
 *	kbRenderer_DX11::SetConstantBuffer
 */
ID3D11Buffer * kbRenderer_DX11::SetConstantBuffer( const kbShaderVarBindings_t & shaderVarBindings, const kbShaderParamOverrides_t * shaderParamOverrides, const kbRenderObject *const pRenderObject, byte *const pInMappedBufferData ) {
	kbMat4 worldMatrix;
	if ( pRenderObject != nullptr ) {
		worldMatrix.MakeScale( pRenderObject->m_Scale );
		worldMatrix *= pRenderObject->m_Orientation.ToMat4();
		worldMatrix[3] = pRenderObject->m_Position;
	} else {
		worldMatrix = kbMat4::identity;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ID3D11Buffer * pConstantBuffer = nullptr;
	HRESULT hr;

	byte * constantPtr = nullptr;
	if ( pInMappedBufferData == nullptr ) {
		pConstantBuffer = GetConstantBuffer( shaderVarBindings.m_ConstantBufferSizeBytes );
		hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		kbErrorCheck( SUCCEEDED(hr), "kbRenderer_DX11::RenderDebugLines() - Failed to map matrix buffer" );
		constantPtr = (byte*) mappedResource.pData;
	} else {
		constantPtr = pInMappedBufferData;
	}

    const std::vector<kbShaderParamOverrides_t::kbShaderParam_t> * paramOverrides = nullptr;
	if ( shaderParamOverrides != nullptr ) {
		paramOverrides = &shaderParamOverrides->m_ParamOverrides;
	}

	const auto & bindings = shaderVarBindings.m_VarBindings;
	for ( int i = 0; i < bindings.size(); i++ ) {
		const std::string & varName = bindings[i].m_VarName;
		const byte * pVarByteOffset = constantPtr + bindings[i].m_VarByteOffset;
		if ( varName == "billboardedModelMatrix" ) {

			kbVec3 camToObject = ( m_pCurrentRenderWindow->GetCameraPosition() - pRenderObject->m_Position ).Normalized();
			const kbVec3 rightVec = kbVec3::up.Cross( camToObject ).Normalized();
			camToObject = rightVec.Cross( kbVec3::up ).Normalized();
			kbMat4 billBoardedMatrix = kbMat4::identity;
			billBoardedMatrix[0].Set( rightVec.x, rightVec.y, rightVec.z, 0.0f );
			billBoardedMatrix[1].Set( 0.0f, 1.0f, 0.0f, 0.0f );
			billBoardedMatrix[2].Set( camToObject.x, camToObject.y, camToObject.z, 0.0f );
			billBoardedMatrix[3].Set( pRenderObject->m_Position.x, pRenderObject->m_Position.y, pRenderObject->m_Position.z, 1.0f );

			kbMat4 scaleMatrix;
			scaleMatrix.MakeScale( pRenderObject->m_Scale );
			billBoardedMatrix = scaleMatrix * billBoardedMatrix;

			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = billBoardedMatrix;
		} else if ( varName == "mvpMatrix" ) {
			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = worldMatrix * m_pCurrentRenderWindow->GetViewProjectionMatrix();
		} else if ( varName == "worldViewMatrix" ) {
			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = worldMatrix * m_pCurrentRenderWindow->GetViewMatrix();
		} else if ( varName == "vpMatrix" ) {
			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = m_pCurrentRenderWindow->GetViewProjectionMatrix();
        } else if ( varName == "modelMatrix" ) {
			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = worldMatrix;
		} else if ( varName == "cameraPos" ) {
			kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
			*pVecOffset = m_pCurrentRenderWindow->GetCameraPosition();
		} else if ( varName == "viewProjection" ) {
			kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			*pMatOffset = m_pCurrentRenderWindow->GetViewProjectionMatrix();
		} else if ( varName == "boneList" ) {
			if ( pRenderObject != nullptr ) {
				kbMat4 *const boneMatrices = (kbMat4*)pVarByteOffset;
				for ( int i = 0; i < pRenderObject->m_MatrixList.size() && i < Max_Shader_Bones; i++ ) {
					boneMatrices[i].MakeIdentity();
					boneMatrices[i][0] = pRenderObject->m_MatrixList[i].GetAxis(0);
					boneMatrices[i][1] = pRenderObject->m_MatrixList[i].GetAxis(1);
					boneMatrices[i][2] = pRenderObject->m_MatrixList[i].GetAxis(2);
					boneMatrices[i][3] = pRenderObject->m_MatrixList[i].GetAxis(3);
				
					boneMatrices[i][0].w = 0;
					boneMatrices[i][1].w = 0;
					boneMatrices[i][2].w = 0;
				}
			}
		} else if ( varName == "time" ) {

            kbVec4 time;
            time.x = g_GlobalTimer.TimeElapsedSeconds();
            time.y = sin( time.x );
            time.z = sin( time.x * 2.0f );
            time.w = sin( time.x * 3.0f );
            kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
            *pVecOffset = time;
        } else if ( paramOverrides != nullptr ) {
            for ( int iOverride = 0; iOverride < paramOverrides->size(); iOverride++ ) {
                const kbShaderParamOverrides_t::kbShaderParam_t & curOverride = (*paramOverrides)[iOverride];
                const std::string & overrideVarName = curOverride.m_VarName;
                if ( varName == overrideVarName ) {

                    // Check if it doesn't fit
                    const size_t endOffset = curOverride.m_VarSizeBytes + bindings[i].m_VarByteOffset;
                    if ( endOffset > shaderVarBindings.m_ConstantBufferSizeBytes || ( i < bindings.size() - 1 && endOffset > bindings[i+1].m_VarByteOffset ) ) {
                        break;
                    }
               
                    switch( curOverride.m_Type ) {
                        case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_MAT4 : {
                            kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
			                *pMatOffset = curOverride.m_Mat4List[0];
                            break;
                        }

                        case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4 : {
                            kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
			                *pVecOffset = curOverride.m_Vec4List[0];
                            break;
                        }

						case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_MAT4_LIST : {
                            kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
							for ( int i = 0; i < curOverride.m_Mat4List.size(); i++ ) {
								pMatOffset[i] = curOverride.m_Mat4List[i];
							}
                            break;
						}

						case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4_LIST : {
                            kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
							for ( int i = 0; i < curOverride.m_Vec4List.size(); i++ ) {
								pVecOffset[i] = curOverride.m_Vec4List[i];
							}
                            break;
						}
                    }
                }
            }
        }
	}

    // Bind textures
	if ( paramOverrides != nullptr ) {
		for ( int iTex = 0; iTex < shaderVarBindings.m_Textures.size(); iTex++ ) {
			auto textureBinding = shaderVarBindings.m_Textures[iTex];

			ID3D11ShaderResourceView * pShaderResourceView = nullptr;
			if ( textureBinding.m_pDefaultTexture != nullptr ) {
				pShaderResourceView = textureBinding.m_pDefaultTexture->GetGPUTexture();
			} else if ( textureBinding.m_pDefaultRenderTexture != nullptr ) {
				pShaderResourceView = ((kbRenderTexture_DX11*)textureBinding.m_pDefaultRenderTexture)->m_pShaderResourceView;
			}

			for ( int iOverride = 0; iOverride < paramOverrides->size(); iOverride++ ) {
				const kbShaderParamOverrides_t::kbShaderParam_t & curOverride = (*paramOverrides)[iOverride];
				if ( curOverride.m_Type == kbShaderParamOverrides_t::kbShaderParam_t::SHADER_TEX && curOverride.m_VarName == shaderVarBindings.m_Textures[iTex].m_TextureName ) {

					if ( curOverride.m_pTexture != nullptr ) {
						 pShaderResourceView = curOverride.m_pTexture->GetGPUTexture();
					} else if ( curOverride.m_pRenderTexture != nullptr ) {
						pShaderResourceView = ((kbRenderTexture_DX11*)curOverride.m_pRenderTexture)->m_pShaderResourceView;
					}
				}
			}

			m_pDeviceContext->VSSetShaderResources( iTex, 1, &pShaderResourceView );				
			m_pDeviceContext->GSSetShaderResources( iTex, 1, &pShaderResourceView );				
			m_pDeviceContext->PSSetShaderResources( iTex, 1, &pShaderResourceView );				
		}
	}

	if ( pInMappedBufferData == nullptr ) {
		m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	}

	return pConstantBuffer;
}

/**
 *	kbRenderer_DX11::RT_MapRenderTarget
 */
kbRenderTargetMap kbRenderer_DX11::RT_MapRenderTarget( kbRenderTexture *const pTarget ) {

	kbRenderTexture_DX11 *const pDX11Target = (kbRenderTexture_DX11*)pTarget;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_pDeviceContext->Map( pDX11Target->m_pRenderTargetTexture, 0, D3D11_MAP_READ, 0, &mappedResource );

	kbRenderTargetMap returnVal;
	returnVal.m_pData = (byte*)mappedResource.pData;
	returnVal.m_Width = pTarget->GetWidth();
	returnVal.m_Height = pTarget->GetHeight();
	returnVal.m_rowPitch = mappedResource.RowPitch;

	return returnVal;
}

/**
 *	kbRenderer_DX11::RT_UnmapRenderTarget
 */
void kbRenderer_DX11::RT_UnmapRenderTarget( kbRenderTexture *const pTarget ) {
	kbRenderTexture_DX11 *const pDX11Target = (kbRenderTexture_DX11*)pTarget;

	m_pDeviceContext->Unmap( pDX11Target->m_pRenderTargetTexture, 0 );
}