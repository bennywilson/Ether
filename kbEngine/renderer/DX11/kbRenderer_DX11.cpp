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
#include "kbSkeletalModelComponent.h"
#include "kbConsole.h"

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
kbRenderer_DX11 * g_pRenderer = nullptr;

const UINT Max_Shader_Bones = 128;
const float	kbRenderer_DX11::Near_Plane = 1.0f;
const float	kbRenderer_DX11::Far_Plane = 20000.0f;
const float g_DebugTextSize = 0.0165f;
const float g_DebugLineSpacing = 0.0165f + 0.007f;

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
 *	kbRenderWindow::kbRenderWindow
 */
kbRenderWindow::kbRenderWindow() :
	m_Hwnd( nullptr ),
	m_pSwapChain( nullptr ),
	m_pRenderTargetView( nullptr ),
	m_ViewPixelWidth( 0 ),
	m_ViewPixelHeight( 0 ),
	m_fViewPixelWidth( 0.0f ),
	m_fViewPixelHeight( 0.0f ),
	m_fViewPixelHalfWidth( 0.0f ),
	m_fViewPixelHalfHeight( 0.0f ) {

	m_ProjectionMatrix.MakeIdentity();
	m_InverseProjectionMatrix.MakeIdentity();
	m_ViewMatrix.MakeIdentity();
	m_ViewProjectionMatrix.MakeIdentity();
	m_InverseViewProjectionMatrix.MakeIdentity();
	m_CameraRotation.x = m_CameraRotation.y = m_CameraRotation.z = m_CameraRotation.w = 0.0f;
}

/**
 *	kbRenderWindow::~kbRenderWindow
 */
kbRenderWindow::~kbRenderWindow() {
	kbErrorCheck( m_pSwapChain == nullptr, "kbRenderWindow::~kbRenderWindow() - Swap chain still exists" );
	kbErrorCheck( m_pRenderTargetView == nullptr, "kbRenderWindow::~kbRenderWindow() - Target view still exists" );
	
	{
		std::map< const kbComponent *, kbRenderObject * >::iterator iter;
	
		for ( iter = m_RenderObjectMap.begin(); iter != m_RenderObjectMap.end(); iter++ ) {
		   delete iter->second;
		}
	}
	
	{
		std::map< const kbLightComponent *, kbRenderLight * >::iterator iter;
	
		for ( iter = m_RenderLightMap.begin(); iter != m_RenderLightMap.end(); iter++ ) {
		   delete iter->second;
		}
	}
}

/**
 *	kbRenderWindow::BeginFrame
 */
void kbRenderWindow::BeginFrame() {
	kbMat4 translationMatrix( kbMat4::identity );
	translationMatrix[3].ToVec3() = -m_CameraPosition;
	kbMat4 rotationMatrix = m_CameraRotation.ToMat4();
	rotationMatrix.TransposeSelf();
	
	m_ViewMatrix = translationMatrix * rotationMatrix;
	m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	
	XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_ViewProjectionMatrix ) );
	m_InverseViewProjectionMatrix = kbMat4FromXMMATRIX( inverseProj );
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
//	kbRenderJob
//-------------------------------------------------------------------------------------------------------------------------------------

/**
 *
 */
void kbRenderJob::Run() {

	SetThreadName( "Render thread" );
	while( m_bRequestShutdown == false ) {
		if ( g_pRenderer != nullptr && g_pRenderer->m_RenderThreadSync == 1 ) {
			g_pRenderer->RenderScene();
			g_pRenderer->m_RenderThreadSync = 0;
		}
	}

	MarkJobAsComplete();
};

//-------------------------------------------------------------------------------------------------------------------------------------
//	kbRenderer_DX11
//-------------------------------------------------------------------------------------------------------------------------------------

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
	m_pNoFaceCullingRasterizerState( nullptr ),
	m_pWireFrameRasterizerState( nullptr ),
	m_pCurrentRenderWindow( nullptr ),
	m_pDepthStencilView( nullptr ),
	m_pUnitQuad( nullptr ),
	m_pConsoleQuad( nullptr ),
	m_pOpaqueQuadShader( nullptr ),
	m_pTranslucentShader( nullptr ),
	m_pBasicParticleShader( nullptr ),
	m_pBasicShader( nullptr ),
	m_pDebugShader( nullptr ),
	m_pBasicSkinnedTextureShader( nullptr ),
	m_pDirectionalLightShadowShader( nullptr ),
	m_pPointLightShader( nullptr ),
	m_pCylindricalLightShader( nullptr ),
	m_pLightShaftsShader( nullptr ),
	m_pMissingShader( nullptr ),
	m_pDirectionalLightShader( nullptr ),
	m_pSimpleAdditiveShader( nullptr ),
	m_pMousePickerIdShader( nullptr ),
	m_pBasicSamplerState( nullptr ),
	m_pNormalMapSamplerState( nullptr ),
	m_pShadowMapSamplerState( nullptr ),
	m_pEventMarker( nullptr ),
	m_DebugVertexBuffer( nullptr ),
	m_DebugPreTransformedVertexBuffer( nullptr ),
	m_ViewMode_GameThread( ViewMode_Normal ),
	m_ViewMode( ViewMode_Normal ),
	m_pOffScreenRenderTargetTexture( nullptr ),
	m_ovrSession( nullptr ),
	m_HMDPass( 0 ),
	m_bRenderToHMD( false ),
	m_SensorSampleTime( 0.0 ),
	m_MirrorTexture( nullptr ),
	m_bUsingHMDTrackingOnly( false ),
	Back_Buffer_Width( 1280 ),
	Back_Buffer_Height( 1024 ),
	m_RenderThreadSync( 0 ),
	m_pRenderJob( nullptr ),
	m_bConsoleEnabled( false ),
	m_FrameNum( 0 ),
	m_FogColor_GameThread( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_FogColor_RenderThread( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_FogStartDistance_GameThread( 2100 ),
	m_FogStartDistance_RenderThread( 2200 ),
	m_FogEndDistance_GameThread( 2100 ),
	m_FogEndDistance_RenderThread( 2200 ),
	m_DebugText( nullptr ) {

	m_pSkinnedDirectionalLightShadowShader = new kbShader( "../../kbEngine/assets/Shaders/directionalLightShadow.kbShader" );
	m_pBloomGatherShader = new kbShader( "../../kbEngine/assets/Shaders/bloom.kbShader" );
	m_pBloomBlur = new kbShader( "../../kbEngine/assets/Shaders/bloom.kbShader" );

	ZeroMemory( m_pTextures, sizeof(m_pTextures) );

	m_OculusTexture[0] = m_OculusTexture[1] = nullptr;
	g_pRenderer = this;
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
void kbRenderer_DX11::Init( HWND hwnd, const int frameWidth, const int frameHeight, const bool bUseHMD, const bool bUseHMDTrackingOnly ) {
	kbLog( "Initializing kbRenderer" );

	m_hwnd = hwnd;

	// Create device and swap chain
	UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	if ( GCreateDebugD3DDevice )
		Flags |= D3D11_CREATE_DEVICE_DEBUG;

	HRESULT hr = CreateDXGIFactory1( __uuidof(IDXGIFactory), (void**)&m_pDXGIFactory );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create DXGI Factory" );

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

	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create D3D11 Device and swap chain" );

	g_pD3DDevice = m_pD3DDevice;
	g_pImmediateContext = m_pDeviceContext;
	m_RenderState.SetDeviceAndContext( m_pD3DDevice, m_pDeviceContext );

	// create swap chains
	CreateRenderView( hwnd );

	// create render targets
	const int deferredRTWidth = ( m_bRenderToHMD )?( m_EyeRenderViewport[0].Size.w * 2 ):( Back_Buffer_Width );
	const int deferredRTHeight = ( m_bRenderToHMD )?( m_EyeRenderViewport[0].Size.h ):( Back_Buffer_Height );

	CreateRenderTarget( eRenderTargetTexture::COLOR_BUFFER, deferredRTWidth, deferredRTHeight, DXGI_FORMAT_R16G16B16A16_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::NORMAL_BUFFER, deferredRTWidth, deferredRTHeight, DXGI_FORMAT_R16G16B16A16_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::DEPTH_BUFFER, deferredRTWidth, deferredRTHeight, DXGI_FORMAT_R32G32_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::ACCUMULATION_BUFFER, deferredRTWidth, deferredRTHeight, DXGI_FORMAT_R16G16B16A16_FLOAT );

	const int shadowBufferSize = 2048;
	CreateRenderTarget( eRenderTargetTexture::SHADOW_BUFFER, shadowBufferSize, shadowBufferSize, DXGI_FORMAT_R32_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::SHADOW_BUFFER_DEPTH, shadowBufferSize, shadowBufferSize, DXGI_FORMAT_D24_UNORM_S8_UINT );

	CreateRenderTarget( eRenderTargetTexture::DOWN_RES_BUFFER, deferredRTWidth / 2, deferredRTHeight / 2, DXGI_FORMAT_R16G16B16A16_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::DOWN_RES_BUFFER_2, deferredRTWidth / 2, deferredRTHeight / 2, DXGI_FORMAT_R16G16B16A16_FLOAT );
	CreateRenderTarget( eRenderTargetTexture::SCRATCH_BUFFER, deferredRTHeight / 2, deferredRTHeight / 2, DXGI_FORMAT_R16G16B16A16_FLOAT );

	CreateRenderTarget( eRenderTargetTexture::MOUSE_PICKER_BUFFER, deferredRTWidth, deferredRTHeight, DXGI_FORMAT_R16G16_UINT );

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
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create DepthStencilBuffer" );

	// specify the subresources of a texture that are accesible from a depth-stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory( &depthStencilViewDesc, sizeof( depthStencilViewDesc ) );
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	hr = m_pD3DDevice->CreateDepthStencilView( m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create DepthStencilView" );

	// bind render target view and depth stencil to output render pipeline
	m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderWindowList[0]->m_pRenderTargetView, m_pDepthStencilView );

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
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create default rasterizer state" );

	// Create non-culling rasterizer state
	rasterDesc.CullMode = D3D11_CULL_NONE;
	// Create the rasterizer state from the description we just filled out.
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pNoFaceCullingRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create non-culling rasterizer state" );

	// Create a wireframe rasterizer state
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_pD3DDevice->CreateRasterizerState( &rasterDesc, &m_pWireFrameRasterizerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create wireframe rasterizer state" );

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
	m_pBasicShader = ( kbShader * )g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/BasicShader.kbShader", true );	
	m_pOpaqueQuadShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTexture.kbShader", true );
	m_pTranslucentShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbShader", true );
	m_pBasicParticleShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicParticle.kbShader", true );
	m_pMissingShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/missingShader.kbShader", true );
	m_pDebugShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/debugShader.kbShader", true );
	m_pBasicSkinnedTextureShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicSkinned.kbShader", true );

	m_pUberPostProcess = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/UberPostProcess.kbShader", true );
	m_pDirectionalLightShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/DirectionalLight.kbShader", true );
	m_pPointLightShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/PointLight.kbShader", true );
	m_pCylindricalLightShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/cylindricalLight.kbShader", true );

	m_pDirectionalLightShadowShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/directionalLightShadow.kbShader", true );
	m_pLightShaftsShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/lightShafts.kbShader", true );
	m_pSimpleAdditiveShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/SimpleAdditive.kbShader", true );
	m_pMousePickerIdShader = (kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/MousePicker.kbShader", true );

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
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create basic sampler state" );

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = m_pD3DDevice->CreateSamplerState( &samplerDesc, &m_pNormalMapSamplerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create normal sampler state" );
	
	hr = m_pD3DDevice->CreateSamplerState( &samplerDesc, &m_pShadowMapSamplerState );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create shadowmap sampler state" );

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
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create debug line vertex buffer" );

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
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::Init() - Failed to create pretransformed debug line vertex buffer" );

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

	hr = m_pD3DDevice->CreateTexture2D( &textureDesc, nullptr, &m_pOffScreenRenderTargetTexture);

	LoadTexture( "../../kbEngine/assets/Textures/Editor/white.bmp", 0 );

	kbLog( "  kbRenderer initialized" );

	SetRenderWindow( m_hwnd );

	kbGPUTimeStamp::Init( m_pDeviceContext );

	LoadTexture( "../../kbEngine/assets/Textures/textbackground.dds", 3 );
	LoadTexture( "../../kbEngine/assets/Textures/Font.bmp", 4 );

	m_DebugText = new kbModel();
	m_DebugText->CreateDynamicModel( 10000, 10000 );

	unsigned long * pDebugText = (unsigned long *)m_DebugText->MapIndexBuffer();
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
	kbErrorCheck( SUCCEEDED( hr ), " kbRenderer_DX11::Initialize() - Failed to query user defined annotation" );

	// Kick off render thread
	m_pRenderJob = new kbRenderJob();
	g_pJobManager->RegisterJob( m_pRenderJob );
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
	if ( hwnd == nullptr ) {
		kbError( "nullptr window handle passed into kbRenderer_DX11::CreateRenderView" );
	}

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

	kbRenderWindow * renderView = new kbRenderWindow;

	m_pDXGIFactory->CreateSwapChain( m_pD3DDevice, &sd, &renderView->m_pSwapChain );

	ID3D11Texture2D * pBackBuffer;
	renderView->m_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer );
	m_pD3DDevice->CreateRenderTargetView( pBackBuffer, nullptr, &renderView->m_pRenderTargetView );
	pBackBuffer->Release();

	RECT windowDimensions;
	GetClientRect( hwnd, &windowDimensions );
	renderView->m_Hwnd = hwnd;
	renderView->m_ViewPixelWidth = windowDimensions.right - windowDimensions.left;
	renderView->m_ViewPixelHeight = windowDimensions.bottom - windowDimensions.top;
	renderView->m_fViewPixelWidth = static_cast<float>( renderView->m_ViewPixelWidth );
	renderView->m_fViewPixelHeight = static_cast<float>( renderView->m_ViewPixelHeight );
	renderView->m_fViewPixelHalfWidth = renderView->m_fViewPixelWidth* 0.5f;
	renderView->m_fViewPixelHalfHeight = renderView->m_fViewPixelHeight * 0.5f;
	renderView->m_ProjectionMatrix.CreatePerspectiveMatrix( kbToRadians( 75.0f ), renderView->m_fViewPixelWidth / renderView->m_fViewPixelHeight, Near_Plane, Far_Plane );

	XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( renderView->m_ProjectionMatrix ) );
	renderView->m_InverseProjectionMatrix = kbMat4FromXMMATRIX( inverseProj );

	m_RenderWindowList.push_back( renderView );

	return (int)m_RenderWindowList.size() - 1;
}

/*
 *	kbRenderer_DX11::CreateRenderTarget
 */
void kbRenderer_DX11::CreateRenderTarget( const eRenderTargetTexture index, const int width, const int height, const DXGI_FORMAT targetFormat ) {

	kbRenderTexture & rt = m_RenderTargets[index];
	kbErrorCheck( rt.m_pRenderTargetTexture == nullptr && rt.m_pRenderTargetView == nullptr && 
				  rt.m_pShaderResourceView == nullptr && rt.m_pShaderResourceView == nullptr, "kbRenderer_DX11::CreateRenderTarget() - Called on an existing render target with index %d", (int) index );
	
	rt.m_Width = width;
	rt.m_Height = height;

	if ( index == eRenderTargetTexture::SHADOW_BUFFER_DEPTH ) {
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
		m_pD3DDevice->CreateTexture2D( &depthBufferDesc, nullptr, &m_RenderTargets[index].m_pRenderTargetTexture );

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory( &depthStencilViewDesc, sizeof( depthStencilViewDesc ) );
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags = 0;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		m_pD3DDevice->CreateDepthStencilView( m_RenderTargets[index].m_pRenderTargetTexture, &depthStencilViewDesc, &m_RenderTargets[index].m_pDepthStencilView );

		// Shader resource view
	/*	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	
		shaderResourceViewDesc.Format = depthBufferDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
	
		HRESULT hr = m_pD3DDevice->CreateShaderResourceView( m_RenderTargets[index].m_pRenderTargetTexture, &shaderResourceViewDesc, &m_RenderTargets[index].m_pShaderResourceView );
*/
		return;
	}

	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = targetFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	HRESULT hr = m_pD3DDevice->CreateTexture2D( &textureDesc, nullptr, &rt.m_pRenderTargetTexture );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::CreateRenderTarget() - Failed to create 2D texture for index", (int)index);

	// Render target view
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	hr = m_pD3DDevice->CreateRenderTargetView( m_RenderTargets[index].m_pRenderTargetTexture, &renderTargetViewDesc, &m_RenderTargets[index].m_pRenderTargetView );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::CreateRenderTarget() - Failed to create RTV for index", (int)index);

	// Shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = m_pD3DDevice->CreateShaderResourceView( m_RenderTargets[index].m_pRenderTargetTexture, &shaderResourceViewDesc, &m_RenderTargets[index].m_pShaderResourceView );
	kbErrorCheck( SUCCEEDED( hr ), "kbRenderer_DX11::CreateRenderTarget() - Failed to create SRV texture for index", (int)index);
}

/**
 *	kbRenderer_DX11::Shutdown
 */
void kbRenderer_DX11::Shutdown() {

	kbLog( "Shutting down kbRenderer" );

	// Wait for render thread to become idle
	m_pRenderJob->RequestShutdown();
	while( m_pRenderJob->IsJobFinished() == false ) { }
	delete m_pRenderJob;
	m_pRenderJob = nullptr;

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

	for ( int i = 0; i < eRenderTargetTexture::NUM_RENDER_TARGETS; i++) {
		m_RenderTargets[i].Release();
	}

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
void kbRenderer_DX11::SetRenderTarget( eRenderTargetTexture type ) {
	m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[type].m_pRenderTargetView, m_pDepthStencilView );
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
		if ( m_RenderWindowList[i]->m_Hwnd == hwnd ) {
			m_pCurrentRenderWindow = m_RenderWindowList[i];
			return;
		}
	}

	kbError( "SetRenderWindow called with a bad window handle" );
}

/**
 *	kbRenderer_DX11::AddRenderObject
 */
void kbRenderer_DX11::AddRenderObject( const kbComponent *const pComponent, const kbModel *const pModel, const kbVec3 & pos, const kbQuat & orientation, const kbVec3 & scale, const ERenderPass renderPass, const std::vector<kbShader *> *const pShaderOverrideList, const kbShaderParamOverrides_t *const pShaderParamOverrides ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::AddRenderObject - Error, nullptr Render window found" );
	}

	kbRenderObject newRenderObjectInfo;
	newRenderObjectInfo.m_pModel = pModel;
	newRenderObjectInfo.m_pComponent = pComponent;
	newRenderObjectInfo.m_Position = pos;
	newRenderObjectInfo.m_Orientation= orientation;
	newRenderObjectInfo.m_Scale = scale;
	newRenderObjectInfo.m_bIsFirstAdd = true;
	newRenderObjectInfo.m_bIsRemove = false;
	newRenderObjectInfo.m_RenderPass = renderPass;
	if ( pComponent->GetOwner() != nullptr ) {
		newRenderObjectInfo.m_EntityId = static_cast<kbGameEntity*>( pComponent->GetOwner() )->GetEntityId();
	}

	if ( pComponent->IsA( kbModelComponent::GetType() ) ) {
		newRenderObjectInfo.m_bCastsShadow = static_cast<const kbModelComponent*>( pComponent )->GetCastsShadow();
	}

	if ( pShaderOverrideList != nullptr ) {
		newRenderObjectInfo.m_OverrideShaderList = *pShaderOverrideList;
	}

	if ( pShaderParamOverrides != nullptr ) {
		newRenderObjectInfo.m_ShaderParamOverrides = *pShaderParamOverrides;
	}

	m_RenderObjectList_GameThread.push_back( newRenderObjectInfo );
}

/**
 *	kbRenderer_DX11::UpdateRenderObject
 */
void kbRenderer_DX11::UpdateRenderObject( const kbComponent *const pComponent, const kbModel *const pModel, const kbVec3 & pos, const kbQuat & orientation, const kbVec3 & scale, const ERenderPass renderPass, const std::vector<kbShader *> *const pShaderOverrideList, const kbShaderParamOverrides_t *const pShaderParamsOverride ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::UpdateRenderObject - nullptr Render Window" );
	}

	kbRenderObject newRenderObjectInfo;
	newRenderObjectInfo.m_pModel = pModel;
	newRenderObjectInfo.m_pComponent = pComponent;
	newRenderObjectInfo.m_Position = pos;
	newRenderObjectInfo.m_Orientation = orientation;
	newRenderObjectInfo.m_Scale = scale;
	newRenderObjectInfo.m_bIsFirstAdd = false;
	newRenderObjectInfo.m_bIsRemove = false;
	newRenderObjectInfo.m_RenderPass = renderPass;
	if ( pComponent != nullptr ) {
		newRenderObjectInfo.m_EntityId = static_cast<kbGameEntity*>( pComponent->GetOwner() )->GetEntityId();
	}

	if ( pComponent->IsA( kbModelComponent::GetType() ) ) {
		newRenderObjectInfo.m_bCastsShadow = static_cast<const kbModelComponent*>( pComponent )->GetCastsShadow();
	}

	if ( pShaderOverrideList != nullptr ) {
		newRenderObjectInfo.m_OverrideShaderList = *pShaderOverrideList;
	}

	if ( pShaderParamsOverride != nullptr ) {
		newRenderObjectInfo.m_ShaderParamOverrides = *pShaderParamsOverride;
	}

	m_RenderObjectList_GameThread.push_back( newRenderObjectInfo );
}

/**
 *	kbRenderer_DX11::RemoveRenderObject
 */
void kbRenderer_DX11::RemoveRenderObject( const kbComponent *const pComponent ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::UpdateRenderObject - nullptr Render Window" );
	}

	// Remove duplicates
	for ( int i = 0; i < m_RenderObjectList_GameThread.size(); i++ ) {
		if ( m_RenderObjectList_GameThread[i].m_pComponent == pComponent ) {
			m_RenderObjectList_GameThread.erase(m_RenderObjectList_GameThread.begin() + i);
			i--;
		}
	}

	kbRenderObject RenderObjectToRemove;
	RenderObjectToRemove.m_pComponent = pComponent;
	RenderObjectToRemove.m_bIsRemove = true;

	m_RenderObjectList_GameThread.push_back( RenderObjectToRemove );
}

/**
 *	kbRenderer_DX11::AddLight
 */
void kbRenderer_DX11::AddLight( const kbLightComponent * pLightComponent, const kbVec3 & pos, const kbQuat & orientation ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::AddLight - nullptr Render Window" );
	}

	kbRenderLight newLight;

	newLight.m_pLightComponent = pLightComponent;
	newLight.m_Position = pos;
	newLight.m_Orientation = orientation;
	newLight.m_Radius = pLightComponent->GetRadius();
	newLight.m_Length = pLightComponent->GetLength();

	// If there are empty entries in the splits distance array, a value of FLT_MAX ensures the split won't be selected in the projection shader
	newLight.m_CascadedShadowSplits[0] = FLT_MAX;
	newLight.m_CascadedShadowSplits[1] = FLT_MAX;
	newLight.m_CascadedShadowSplits[2] = FLT_MAX;
	newLight.m_CascadedShadowSplits[3] = FLT_MAX;

	if ( pLightComponent->CastsShadow() && pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
		for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
			newLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	newLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	newLight.m_bCastsShadow = pLightComponent->CastsShadow();
	newLight.m_bIsFirstAdd = true;
	newLight.m_bIsRemove = false;
	m_LightList_GameThread.push_back( newLight );
}

/**
 *	kbRenderer_DX11::UpdateLight
 */
void kbRenderer_DX11::UpdateLight( const kbLightComponent * pLightComponent, const kbVec3 & pos, const kbQuat & orientation ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::UpdateLight - nullptr Render Window" );
	}

	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {
		if ( m_LightList_GameThread[i].m_pLightComponent == pLightComponent ) {
			if ( m_LightList_GameThread[i].m_bIsRemove == false ) {
				m_LightList_GameThread[i].m_Position = pos;
				m_LightList_GameThread[i].m_Orientation = orientation;
				m_LightList_GameThread[i].m_bCastsShadow = pLightComponent->CastsShadow();
				m_LightList_GameThread[i].m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
				m_LightList_GameThread[i].m_Radius = pLightComponent->GetRadius();
				m_LightList_GameThread[i].m_Length = pLightComponent->GetLength();

				memset( &m_LightList_GameThread[i].m_CascadedShadowSplits, 0, sizeof( m_LightList_GameThread[i].m_CascadedShadowSplits ) );
				if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
					const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
					for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
						m_LightList_GameThread[i].m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
					}
				}
			}
			return;
		}
	}
 
	kbRenderLight updateLight;
	updateLight.m_pLightComponent = pLightComponent;
	updateLight.m_Position = pos;
	updateLight.m_Orientation = orientation;
	updateLight.m_bIsFirstAdd = false;
	updateLight.m_bIsRemove = false;
	updateLight.m_bCastsShadow = pLightComponent->CastsShadow();
	updateLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	updateLight.m_Radius = pLightComponent->GetRadius();
	updateLight.m_Length = pLightComponent->GetLength();

	memset( &updateLight.m_CascadedShadowSplits, 0, sizeof( updateLight.m_CascadedShadowSplits ) );
	if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
		for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
			updateLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	m_LightList_GameThread.push_back( updateLight );
}

/**
 *	kbRenderer_DX11::RemoveLight
 */
void kbRenderer_DX11::RemoveLight( const kbLightComponent *const pLightComponent ) {
	
	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::RemoveLight - nullptr Render Window" );
	}

	kbRenderLight lightToRemove;
	lightToRemove.m_pLightComponent = pLightComponent;
	lightToRemove.m_bIsRemove = true;
	m_LightList_GameThread.push_back( lightToRemove );
}

/**
 *	kbRenderer_DX11::HackClearLight
 */
void kbRenderer_DX11::HackClearLight( const kbLightComponent *const pLightComponent ) {
	
	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {
		if ( m_LightList_GameThread[i].m_pLightComponent == pLightComponent ) {
			m_LightList_GameThread.erase( m_LightList_GameThread.begin() + i );
			i--;
		}
	}

}

/**
 *	kbRenderer_DX11::AddParticle
 */
void kbRenderer_DX11::AddParticle( const void *const pParticleComponent, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation ) {
	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_pModel = pModel;
	NewParticle.m_RenderPass = RP_Translucent;
	NewParticle.m_Position = pos;
	NewParticle.m_Orientation = orientation;
	NewParticle.m_bIsFirstAdd = true;
	NewParticle.m_bIsRemove = false;

	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer_DX11::UpdateParticle
 */
void kbRenderer_DX11::UpdateParticle( const void *const pParticleComponent, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation ) {

	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_pModel = pModel;
	NewParticle.m_RenderPass = RP_Translucent;
	NewParticle.m_Position = pos;
	NewParticle.m_Orientation = orientation;
	NewParticle.m_bIsFirstAdd = false;

	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer_DX11::RemoveParticle
 */
void kbRenderer_DX11::RemoveParticle( const void *const pParticleComponent ) {
	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_bIsFirstAdd = false;
	NewParticle.m_bIsRemove = true;
	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer_DX11::AddLightShafts
 */
void kbRenderer_DX11::AddLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation ) {
	kbLightShafts newLightShafts;
	newLightShafts.m_pLightShaftsComponent = pComponent;
	newLightShafts.m_pTexture = pComponent->GetTexture();
	newLightShafts.m_Color = pComponent->GetColor();
	newLightShafts.m_Width = pComponent->GetBaseWidth();
	newLightShafts.m_Height = pComponent->GetBaseHeight();
	newLightShafts.m_NumIterations = pComponent->GetNumIterations();
	newLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	newLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	newLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	newLightShafts.m_Pos = pos;
	newLightShafts.m_Rotation = orientation;
	newLightShafts.m_Operation = ROO_Add;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			m_LightShafts_GameThread.erase( m_LightShafts_GameThread.begin() );
			break;
		}
	}
	m_LightShafts_GameThread.push_back( newLightShafts );
}

/**
 *	kbRenderer_DX11::UpdateLightShafts
 */
void kbRenderer_DX11::UpdateLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation ) {
	kbLightShafts updatedLightShafts;
	updatedLightShafts.m_pLightShaftsComponent = pComponent;
	updatedLightShafts.m_pTexture = pComponent->GetTexture();
	updatedLightShafts.m_Color = pComponent->GetColor();
	updatedLightShafts.m_Width = pComponent->GetBaseWidth();
	updatedLightShafts.m_Height = pComponent->GetBaseHeight();
	updatedLightShafts.m_NumIterations = pComponent->GetNumIterations();
	updatedLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	updatedLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	updatedLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	updatedLightShafts.m_Pos = pos;
	updatedLightShafts.m_Rotation = orientation;
	updatedLightShafts.m_Operation = ROO_Update;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			if ( m_LightShafts_GameThread[i].m_Operation == ROO_Remove ) {
				return;
			}
		}
	}
	m_LightShafts_GameThread.push_back( updatedLightShafts );
}

/**
 *	kbRenderer_DX11::RemoveLightShafts
 */
void kbRenderer_DX11::RemoveLightShafts( const kbLightShaftsComponent *const pComponent ) {
	kbLightShafts removeLightShafts;
	removeLightShafts.m_pLightShaftsComponent = pComponent;
	removeLightShafts.m_Operation = ROO_Remove;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			m_LightShafts_GameThread.erase( m_LightShafts_GameThread.begin() );
			break;
		}
	}
	m_LightShafts_GameThread.push_back( removeLightShafts );
}

/**
 *	kbRenderer_DX11::UpdateFog
 */
void kbRenderer_DX11::UpdateFog( const kbColor & color, const float startDistance, const float endDistance ) {
	m_FogColor_GameThread = color;
	m_FogStartDistance_GameThread = startDistance;
	m_FogEndDistance_GameThread = endDistance;
}

/**
 *	kbRenderer_DX11::SetPostProcessSettings
 */
void kbRenderer_DX11::SetPostProcessSettings( const kbPostProcessSettings_t & postProcessSettings ) {

	m_PostProcessSettings_GameThread = postProcessSettings;
}

/**
 *	kbRenderer_DX11::RenderSync
 */
void kbRenderer_DX11::RenderSync() {	

	// Copy requested game thread data over to their corresponding render thread structures
	m_DebugLines = m_DebugLines_GameThread;
	m_DebugLines_GameThread.clear();

	m_DebugBillboards = m_DebugBillboards_GameThread;
	m_DebugBillboards_GameThread.clear();

	m_DebugModels = m_DebugModels_GameThread;
	m_DebugModels_GameThread.clear();

	m_DebugStrings = m_DebugStrings_GameThread;
	m_DebugStrings_GameThread.clear();

	m_ScreenSpaceQuads_RenderThread = m_ScreenSpaceQuads_GameThread;
	m_ScreenSpaceQuads_GameThread.clear();

	m_ViewMode = m_ViewMode_GameThread;

	// Add/update render objects
	for ( int i = 0; i < m_RenderObjectList_GameThread.size(); i++ )
	{
		kbRenderObject * renderObject = nullptr;

		if ( m_RenderObjectList_GameThread[i].m_bIsRemove ) {
			kbRenderObject *const pRenderObject = m_pCurrentRenderWindow->m_RenderObjectMap[ m_RenderObjectList_GameThread[i].m_pComponent ];
			m_pCurrentRenderWindow->m_RenderObjectMap.erase( m_RenderObjectList_GameThread[i].m_pComponent );
			delete pRenderObject;
		} else {
			const kbComponent *const pComponent = m_RenderObjectList_GameThread[i].m_pComponent;
			if ( m_RenderObjectList_GameThread[i].m_bIsFirstAdd == false ) {

				// Updating a renderobject 
				std::map< const kbComponent *, kbRenderObject * >::iterator it = m_pCurrentRenderWindow->m_RenderObjectMap.find( m_RenderObjectList_GameThread[i].m_pComponent );
				if ( it == m_pCurrentRenderWindow->m_RenderObjectMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist" );
				}

				renderObject = it->second;
				*renderObject = m_RenderObjectList_GameThread[i];
				if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) && renderObject->m_pModel->NumBones() > 0 ) {
					const kbSkeletalModelComponent *const skelComp = static_cast<const kbSkeletalModelComponent*>( pComponent );
					kbSkinnedRenderObject *const pSkelRenderObject = static_cast<kbSkinnedRenderObject*>( renderObject );
					pSkelRenderObject->m_BoneMatrices = skelComp->GetFinalBoneMatrices();
					pSkelRenderObject->m_bIsSkinnedModel = true;
				}
			} else {

				// Adding new renderobject
				renderObject = m_pCurrentRenderWindow->m_RenderObjectMap[pComponent];

				if ( renderObject == nullptr ) {
					if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) && m_RenderObjectList_GameThread[i].m_pModel->NumBones() > 0 ) {
						kbSkinnedRenderObject *const pSkelRenderObject = new kbSkinnedRenderObject;
						*((kbRenderObject*)pSkelRenderObject) = m_RenderObjectList_GameThread[i];
						renderObject = pSkelRenderObject;
						renderObject->m_bIsSkinnedModel = true;
						const kbSkeletalModelComponent *const skelComp = static_cast<const kbSkeletalModelComponent*>( pComponent );
						pSkelRenderObject->m_BoneMatrices = skelComp->GetFinalBoneMatrices();
					} else {
						renderObject = new kbRenderObject;
						*renderObject = m_RenderObjectList_GameThread[i];
					}

					m_pCurrentRenderWindow->m_RenderObjectMap[m_RenderObjectList_GameThread[i].m_pComponent] = renderObject;
				} else {
					kbError( "kbRenderer_DX11::AddRenderObject - Warning, adding a render object that already exists" );
				}
			}
		}
	}
	m_RenderObjectList_GameThread.clear();

	// Light
	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {

		if ( m_LightList_GameThread[i].m_bIsRemove ) {

			kbRenderLight *const pRenderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];
			m_pCurrentRenderWindow->m_RenderLightMap.erase( m_LightList_GameThread[i].m_pLightComponent );
			delete pRenderLight;
		} else {
			kbRenderLight * renderLight = nullptr;

			bool bIsFirstAdd = m_LightList_GameThread[i].m_bIsFirstAdd;
			if ( bIsFirstAdd ) {
				renderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];

				if ( renderLight != nullptr ) {
					bIsFirstAdd = false;
					kbError( "kbRenderer_DX11::AddLight - Warning, adding a render light that already exists" );
				} else {
					renderLight = new kbRenderLight;
					m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent] = renderLight;
				}
			} else {

				std::map< const kbLightComponent *, kbRenderLight * >::iterator it = m_pCurrentRenderWindow->m_RenderLightMap.find( m_LightList_GameThread[i].m_pLightComponent );

				if ( it == m_pCurrentRenderWindow->m_RenderLightMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateLight - Error, Updating a RenderObject that doesn't exist" );
				} else {
					renderLight = it->second;
				}
			}

			*renderLight = m_LightList_GameThread[i];
		}
	}
	m_LightList_GameThread.clear();

	// Particles
	for ( int i = 0; i < m_ParticleList_GameThread.size(); i++ ) {
		const void *const pComponent = m_ParticleList_GameThread[i].m_pComponent;

		if ( m_ParticleList_GameThread[i].m_bIsRemove ) {
			kbRenderObject * renderParticle = m_pCurrentRenderWindow->m_RenderParticleMap[pComponent];
			m_pCurrentRenderWindow->m_RenderParticleMap.erase( pComponent );
			delete renderParticle;
		} else {
			kbRenderObject * renderParticle = nullptr;

			if ( m_ParticleList_GameThread[i].m_bIsFirstAdd ) {
				renderParticle = m_pCurrentRenderWindow->m_RenderParticleMap[pComponent];

				if ( renderParticle != nullptr ) {
					kbError( "kbRenderer_DX11::AddParticle - Adding a particle that already exists" );
				} else {
					renderParticle = new kbRenderObject;
					m_pCurrentRenderWindow->m_RenderParticleMap[pComponent] = renderParticle;
				}
			} else {
				std::map< const void *, kbRenderObject * >::iterator it = m_pCurrentRenderWindow->m_RenderParticleMap.find( pComponent );
				if ( it == m_pCurrentRenderWindow->m_RenderParticleMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist" );
				}

				 renderParticle = it->second;
			}

			*renderParticle = m_ParticleList_GameThread[i];
		}
	}
	m_ParticleList_GameThread.clear();

	// Light Shafts
	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_Operation == ROO_Add ) {
			bool bAlreadyExists = false;
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					kbError( "kbRenderer_DX11::SetReadyToRender() - Adding light shafts that already exist" );
					bAlreadyExists = true;
					break;
				}
			}

			if ( bAlreadyExists == false ) {
				m_LightShafts_RenderThread.push_back( m_LightShafts_GameThread[i] );
			}
		} else if (  m_LightShafts_GameThread[i].m_Operation == ROO_Remove ) {
			bool bExists = false;
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					m_LightShafts_RenderThread.erase( m_LightShafts_RenderThread.begin() + j );
					bExists = true;
					break;
				}
			}

			if ( bExists == false ) {
				kbError( "kbRenderer_DX11::SetReadyToRender() - Removing light shafts that do not exist" );
			}
		} else {
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					m_LightShafts_RenderThread[j] = m_LightShafts_GameThread[i];
					break;
				}
			}
		}
	}

	m_LightShafts_GameThread.clear();

	// Camera
	for ( int i = 0; i < m_RenderWindowList.size(); i++ ) {

		m_RenderWindowList[i]->m_CameraPosition = m_RenderWindowList[i]->m_CameraPosition_GameThread;
		m_RenderWindowList[i]->m_CameraRotation = m_RenderWindowList[i]->m_CameraRotation_GameThread;
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

	// Fog
	m_FogColor_RenderThread = m_FogColor_GameThread;
	m_FogStartDistance_RenderThread = m_FogStartDistance_GameThread;
	m_FogEndDistance_RenderThread = m_FogEndDistance_GameThread;

	// Post process
	m_PostProcessSettings_RenderThread = m_PostProcessSettings_GameThread;

	// Oculus
	if ( IsRenderingToHMD() || IsUsingHMDTrackingOnly() ) {	

		m_EyeRenderDesc[0] = ovr_GetRenderDesc( m_ovrSession, ovrEye_Left, m_HMDDesc.DefaultEyeFov[0] );
		m_EyeRenderDesc[1] = ovr_GetRenderDesc( m_ovrSession, ovrEye_Right, m_HMDDesc.DefaultEyeFov[1] );

		// Get both eye poses simultaneously, with IPD offset already included. 
		ovrPosef HmdToEyePose[2] = { m_EyeRenderDesc[0].HmdToEyePose, m_EyeRenderDesc[1].HmdToEyePose };
		ovr_GetEyePoses( m_ovrSession, m_FrameNum, ovrTrue, HmdToEyePose, m_EyeRenderPose, &m_SensorSampleTime );

		for ( int iEye = 0; iEye < 2; iEye++ ) {

			const kbMat4 gameCameraMatrix = m_RenderWindowList[0]->m_CameraRotation.ToMat4();
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
										   Vector3f( m_RenderWindowList[0]->m_CameraPosition.x, m_RenderWindowList[0]->m_CameraPosition.y, m_RenderWindowList[0]->m_CameraPosition.z );
			
			const Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos - finalForward, finalUp);
			
			memcpy( &m_RenderWindowList[0]->m_EyeMatrices[iEye], &view, sizeof( Matrix4f ) );
			m_RenderWindowList[0]->m_EyeMatrices[iEye].TransposeSelf();
		}
	}

	kbGPUTimeStamp::UpdateFrameNum();
	m_FrameNum++;
}

/**
 *	kbRenderer_DX11::SetReadyToRender
 */
void kbRenderer_DX11::SetReadyToRender() {	
	m_RenderThreadSync = 1;
}

/*
 *	kbRenderer_DX11:RenderScene
 */
void kbRenderer_DX11::RenderScene() {
	START_SCOPED_TIMER( RENDER_THREAD );

	kbGPUTimeStamp::BeginFrame( m_pDeviceContext );
	PLACE_GPU_TIME_STAMP( "Begin Frame" );

	const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const int numRenderPasses = ( IsRenderingToHMD() ) ? ( 2 ) : ( 1 );

	m_pCurrentRenderWindow->BeginFrame();

	for ( m_HMDPass = 0; m_HMDPass < numRenderPasses; m_HMDPass++ ) {

		{
			START_SCOPED_RENDER_TIMER( RENDER_THREAD_CLEAR_BUFFERS );
			m_pDeviceContext->ClearRenderTargetView( m_RenderTargets[COLOR_BUFFER].m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( m_RenderTargets[NORMAL_BUFFER].m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( m_RenderTargets[DEPTH_BUFFER].m_pRenderTargetView, color );
			m_pDeviceContext->ClearRenderTargetView( m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, color );
			m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );
		}

		if ( m_ViewMode == ViewMode_Normal ) {
			m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
		} else if ( m_ViewMode == ViewMode_Wireframe ) {
			m_pDeviceContext->RSSetState( m_pWireFrameRasterizerState );
		} else {
			kbError( "kbRenderer_DX11::RenderScene() - Invalid view mode %d", (int) m_ViewMode );
		}

		ID3D11RenderTargetView * RenderTargetViews[] = { m_RenderTargets[COLOR_BUFFER].m_pRenderTargetView, m_RenderTargets[NORMAL_BUFFER].m_pRenderTargetView, m_RenderTargets[DEPTH_BUFFER].m_pRenderTargetView };
	
		m_pDeviceContext->OMSetRenderTargets( 3, RenderTargetViews, m_pDepthStencilView );

		D3D11_VIEWPORT viewport;
	
		if ( IsRenderingToHMD() ) {
			viewport.TopLeftX = ( float )m_EyeRenderViewport[m_HMDPass].Pos.x;
			viewport.TopLeftY = ( float )m_EyeRenderViewport[m_HMDPass].Pos.y;
			viewport.Width = ( float )m_EyeRenderViewport[m_HMDPass].Size.w;
			viewport.Height = ( float )m_EyeRenderViewport[m_HMDPass].Size.h;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
		
			const ovrMatrix4f proj = ovrMatrix4f_Projection( m_EyeRenderDesc[m_HMDPass].Fov, kbRenderer_DX11::Near_Plane, kbRenderer_DX11::Far_Plane, ovrProjection_None );
			memcpy( &m_pCurrentRenderWindow->m_ProjectionMatrix, &proj, sizeof( ovrMatrix4f ) );
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
			m_pCurrentRenderWindow->m_CameraPosition.z = inverseView.m[3][2];


		} else {
			viewport.Width = (float)Back_Buffer_Width;
			viewport.Height = (float)Back_Buffer_Height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;

			if ( IsUsingHMDTrackingOnly() ) {
				m_pCurrentRenderWindow->m_ViewMatrix = m_pCurrentRenderWindow->m_EyeMatrices[m_HMDPass];
				m_pCurrentRenderWindow->m_ViewProjectionMatrix = m_pCurrentRenderWindow->m_ViewMatrix * m_pCurrentRenderWindow->m_ProjectionMatrix;

				XMMATRIX inverseProj = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewProjectionMatrix ) );
				m_pCurrentRenderWindow->m_InverseViewProjectionMatrix = kbMat4FromXMMATRIX( inverseProj );
			}

			if ( this->IsUsingHMDTrackingOnly() ) {
				XMMATRIX inverseView = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( m_pCurrentRenderWindow->m_ViewMatrix ) );
				m_pCurrentRenderWindow->m_CameraPosition.x = inverseView.m[3][0];
				m_pCurrentRenderWindow->m_CameraPosition.y = inverseView.m[3][1];
				m_pCurrentRenderWindow->m_CameraPosition.z = inverseView.m[3][2];
			}
		}

		m_pDeviceContext->RSSetViewports( 1, &viewport );

		std::map< const kbComponent *, kbRenderObject * >::iterator iter;
	
		{
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

			for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
				if ( iter->second->m_RenderPass == RP_FirstPerson ) {
					RenderModel( iter->second, RP_FirstPerson );
				}
			}
	
			// Render models that need to be lit
			m_RenderState.SetDepthStencilState();

			for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
				if ( iter->second->m_RenderPass == RP_Lighting ) {
					RenderModel( iter->second, RP_Lighting );
				}
			}

			m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

			PLACE_GPU_TIME_STAMP( "GBuffer" );
		}

		RenderLights();
	
		{
			START_SCOPED_RENDER_TIMER( RENDER_UNLIT )

			m_RenderState.SetDepthStencilState();
			m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, m_pDepthStencilView );

			if ( m_ViewMode == ViewMode_Wireframe ) {
				m_pDeviceContext->CopyResource( m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetTexture, m_RenderTargets[COLOR_BUFFER].m_pRenderTargetTexture );
			}

			// Post-Lighting Render Pass
			for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
				if ( iter->second->m_RenderPass == RP_PostLighting ) {
					RenderModel( iter->second, RP_PostLighting );
				}
			}
			PLACE_GPU_TIME_STAMP( "Unlit" );
		}

		if ( m_ViewMode == ViewMode_Normal ) {
			RenderLightShafts();
		}
		
		PLACE_GPU_TIME_STAMP( "Light Shafts" );

		RenderTranslucency();
		RenderScreenSpaceQuads();

		if ( m_ViewMode == ViewMode_Normal ) {
			// In World UI Pass
			m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );
			m_RenderState.SetBlendState( false,
										 false,
										 true,
										 kbRenderState::BF_SourceAlpha,
										 kbRenderState::BF_InvSourceAlpha,
										 kbRenderState::BO_Add,
										 kbRenderState::BF_One,
										 kbRenderState::BF_Zero,
										 kbRenderState::BO_Add );

			for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
				if ( iter->second->m_RenderPass == RP_InWorldUI ) {
					RenderModel( iter->second, RP_InWorldUI );
				}
			}

			m_RenderState.SetBlendState();
		}

		PLACE_GPU_TIME_STAMP( "Transparency" );

		//m_pDeviceContext->OMSetDepthStencilState( m_pNoDepthStencilState, 1 );

		RenderPostProcess();

		PLACE_GPU_TIME_STAMP( "Post-Process" );

		m_RenderState.SetDepthStencilState();
	
		if ( m_ViewMode == ViewMode_Normal ) {
			START_SCOPED_RENDER_TIMER( RENDER_DEBUG )
			RenderDebugBillboards( false );
			RenderDebugLines();
			RenderPretransformedDebugLines();

			for ( int i = 0; i < m_DebugModels.size(); i++ ) {
				kbRenderObject renderObject;
				renderObject.m_pModel = m_DebugModels[i].m_pModel;
				renderObject.m_Position = m_DebugModels[i].m_Position;
				renderObject.m_Orientation = m_DebugModels[i].m_Orientation;
				renderObject.m_Scale = m_DebugModels[i].m_Scale;
				RenderModel( &renderObject, RP_Debug );
			}
		}
	
		m_RenderState.SetDepthStencilState();

		if ( IsRenderingToHMD() || this->IsUsingHMDTrackingOnly() ) {
			m_OculusTexture[m_HMDPass]->Commit();
		}
	}

	RenderDebugText();

	if ( g_UseEditor ) {
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
			m_pCurrentRenderWindow->m_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer );
			ovr_GetMirrorTextureBufferDX( m_ovrSession, m_MirrorTexture, IID_PPV_ARGS(&tex) );
			m_pDeviceContext->CopyResource( pBackBuffer, tex );
			tex->Release();
			m_pCurrentRenderWindow->m_pSwapChain->Present( 0, 0 );
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
 *	kbRenderer_DX11::RenderTranslucency
 */
void kbRenderer_DX11::RenderTranslucency() {
	START_SCOPED_RENDER_TIMER( RENDER_TRANSLUCENCY );

	m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, m_pDepthStencilView );

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

	m_RenderState.SetBlendState( false,
								 false,
								 true,
								 kbRenderState::BF_One,
								 kbRenderState::BF_One,
								 kbRenderState::BO_Add,
								 kbRenderState::BF_One,
								 kbRenderState::BF_Zero,
								 kbRenderState::BO_Add,
							     kbRenderState::CW_All );

	for ( std::map< const void *, kbRenderObject * >::iterator iter = m_pCurrentRenderWindow->m_RenderParticleMap.begin(); iter != m_pCurrentRenderWindow->m_RenderParticleMap.end(); iter++ ) {
		RenderModel( iter->second, RP_Translucent );
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
	kbErrorCheck( constantBufferIt != m_ConstantBuffers.end() && constantBufferIt->second != nullptr, "kbRenderer_DX11::RenderModel() - Could not find constant buffer of size %d", requestSize );

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
		m_RenderState.SetBlendState( false,
									 false,
									 true,
									 kbRenderState::BF_SourceAlpha,
									 kbRenderState::BF_InvSourceAlpha,
									 kbRenderState::BO_Add,
									 kbRenderState::BF_One,
									 kbRenderState::BF_Zero,
									 kbRenderState::BO_Add );

		RenderScreenSpaceQuadImmediate( int( Back_Buffer_Width * 0.25f ), int( Back_Buffer_Height * 0.1f ), int( Back_Buffer_Width * 0.51f ), int( Back_Buffer_Height * 0.65f ), 3, m_pTranslucentShader );
		m_RenderState.SetBlendState();
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

	m_RenderState.SetBlendState( false,
								 false,
								 true,
								 kbRenderState::BF_One,
								 kbRenderState::BF_One,
								 kbRenderState::BO_Add,
								 kbRenderState::BF_One,
								 kbRenderState::BF_Zero,
								 kbRenderState::BO_Add,
							     kbRenderState::CW_All );

	m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareLess, false );

	ID3D11Buffer * const vertexBuffer = ( ID3D11Buffer * const ) m_DebugText->m_VertexBuffer.GetBufferPtr();
	ID3D11Buffer * const indexBuffer = ( ID3D11Buffer * const ) m_DebugText->m_IndexBuffer.GetBufferPtr();

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
	m_pDeviceContext->IASetIndexBuffer( indexBuffer, DXGI_FORMAT_R32_UINT, 0 );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );

	ID3D11ShaderResourceView *const pShaderResourceView = (ID3D11ShaderResourceView*)m_pTextures[4]->GetGPUTexture();

	m_pDeviceContext->PSSetShaderResources( 0, 1, &pShaderResourceView );
	m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pSimpleAdditiveShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pSimpleAdditiveShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pSimpleAdditiveShader->GetPixelShader(), nullptr, 0 );

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
	m_pDeviceContext->ClearRenderTargetView( m_RenderTargets[MOUSE_PICKER_BUFFER].m_pRenderTargetView, color );
	m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = ( float )m_RenderTargets[MOUSE_PICKER_BUFFER].m_Width;
	viewport.Height = ( float )m_RenderTargets[MOUSE_PICKER_BUFFER].m_Height;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );
	m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[MOUSE_PICKER_BUFFER].m_pRenderTargetView, m_pDepthStencilView );
	m_RenderState.SetDepthStencilState();

	std::map<const kbComponent *, kbRenderObject *>::iterator iter;
	for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
		if ( iter->second->m_EntityId > 0 ) {
			RenderModel( iter->second, RP_MousePicker );
		}
	}

	RenderDebugBillboards( true );

	for ( int i = 0; i < m_DebugModels.size(); i++ ) {
		kbRenderObject renderObject;
		renderObject.m_pModel = m_DebugModels[i].m_pModel;
		renderObject.m_Position = m_DebugModels[i].m_Position;
		renderObject.m_Orientation = m_DebugModels[i].m_Orientation;
		renderObject.m_Scale = m_DebugModels[i].m_Scale;
		renderObject.m_EntityId = m_DebugModels[i].m_EntityId;

		RenderModel( &renderObject, RP_MousePicker );
	}
}

/**
 *	kbRenderer_DX11::Blit
 */
void kbRenderer_DX11::Blit( kbRenderTexture *const src, kbRenderTexture *const dest ) {
	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	kbShader *const pShader = m_pDebugShader;

	if ( dest == nullptr ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &m_pCurrentRenderWindow->m_pRenderTargetView, m_pDepthStencilView );
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
 *	kbRenderer_DX11::RenderBloom
 */
void kbRenderer_DX11::RenderBloom() {
	if ( m_bRenderToHMD == true ) {
		return;
	}

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = ( float )m_RenderTargets[DOWN_RES_BUFFER].m_Width;
	viewport.Height = ( float )m_RenderTargets[DOWN_RES_BUFFER].m_Height;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );

	///////////////////////////////
	// Gather
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[DOWN_RES_BUFFER].m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView );
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

	///////////////////////////////
	// Horizontal blur
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[DOWN_RES_BUFFER_2].m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &m_RenderTargets[DOWN_RES_BUFFER].m_pShaderResourceView );
		ID3D11SamplerState * samplerState[] = { m_pNormalMapSamplerState };

		m_pDeviceContext->PSSetSamplers( 0, 1, samplerState );
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pBloomBlur->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pBloomBlur->GetVertexShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pBloomBlur->GetPixelShader(), nullptr, 0 );

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
		SetShaderMat4( "mvpMatrix", mvpMatrix, (byte*) mappedResource.pData, varBindings );
		SetShaderInt( "numSamples", 5, (byte*) mappedResource.pData, varBindings );

		const float texelSize = 1.0f / m_RenderTargets[DOWN_RES_BUFFER_2].m_Width;
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

		ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView, m_RenderTargets[COLOR_BUFFER].m_pShaderResourceView };
		m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
	}

	///////////////////////////////
	// Vertical blur
	///////////////////////////////
	{
		m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[DOWN_RES_BUFFER].m_pRenderTargetView, nullptr );
		const unsigned int stride = sizeof( vertexLayout );
		const unsigned int offset = 0;

		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
		m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

		m_pDeviceContext->PSSetShaderResources( 0, 1, &m_RenderTargets[DOWN_RES_BUFFER_2].m_pShaderResourceView );
		ID3D11SamplerState * samplerState[] = { m_pNormalMapSamplerState };

		// Set constants
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

		SetShaderMat4( "mvpMatrix", mvpMatrix, (byte*) mappedResource.pData, varBindings );
		SetShaderInt( "numSamples", 5, (byte*) mappedResource.pData, varBindings );

		const float texelSize = 1.0f / m_RenderTargets[DOWN_RES_BUFFER_2].m_Width;
		kbVec4 offsetsAndWeights[5];
		offsetsAndWeights[0].Set( 0.0f * texelSize, 0.0f, 0.22702f, 0.0f );
		offsetsAndWeights[1].Set( 1.0f * texelSize, 0.0f, 0.19459f, 0.0f );
		offsetsAndWeights[2].Set( 2.0f * texelSize, 0.0f, 0.12162f, 0.0f );
		offsetsAndWeights[3].Set( 3.0f * texelSize, 0.0f, 0.05405f, 0.0f );
		offsetsAndWeights[4].Set( 4.0f * texelSize, 0.0f, 0.01621f, 0.0f );

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

	m_RenderState.SetBlendState( false,
								 false,
								 true,
								 kbRenderState::BF_One,
								 kbRenderState::BF_One,
								 kbRenderState::BO_Add,
								 kbRenderState::BF_One,
								 kbRenderState::BF_Zero,
								 kbRenderState::BO_Add,
							     kbRenderState::CW_All );

		m_pDeviceContext->RSSetViewports( 1, &viewport );
		m_pDeviceContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, nullptr );

		ID3D11ShaderResourceView *const  RenderTargetViews[] = { m_RenderTargets[DOWN_RES_BUFFER].m_pShaderResourceView };
		ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pSimpleAdditiveShader->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)this->m_pSimpleAdditiveShader->GetVertexShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pSimpleAdditiveShader->GetPixelShader(), nullptr, 0 );
		m_pDeviceContext->PSSetShaderResources( 0, 1, RenderTargetViews );
		m_pDeviceContext->PSSetSamplers( 0, 1, SamplerStates );

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
		Blit( &m_RenderTargets[ACCUMULATION_BUFFER], nullptr );
		return;
	}

	RenderBloom();

	if ( m_bRenderToHMD == false ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &m_pCurrentRenderWindow->m_pRenderTargetView, m_pDepthStencilView );
	} else {
		ID3D11RenderTargetView *const rtv = m_OculusTexture[m_HMDPass]->GetRTV();
		m_pDeviceContext->OMSetRenderTargets( 1, &rtv, nullptr );
	}

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	if ( m_pCurrentRenderWindow->m_RenderLightMap.size() == 0 )
	{
		m_pDeviceContext->PSSetShaderResources( 0, 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView );
	}
	else
	{
		ID3D11ShaderResourceView * RenderTargetViews[] = { m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView, m_RenderTargets[DEPTH_BUFFER].m_pShaderResourceView };
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
	SetShaderMat4( "inverseProjection", m_pCurrentRenderWindow->m_InverseProjectionMatrix, mappedResource.pData, varBindings );	
	SetShaderVec4( "tint", m_PostProcessSettings_RenderThread.m_Tint, mappedResource.pData, varBindings );
	SetShaderVec4( "additiveColor", m_PostProcessSettings_RenderThread.m_AdditiveColor, mappedResource.pData, varBindings );
	SetShaderVec4( "fogColor", m_FogColor_RenderThread, mappedResource.pData, varBindings );
	SetShaderFloat( "fogStartDistance", m_FogStartDistance_RenderThread, mappedResource.pData, varBindings );
	SetShaderFloat( "fogEndDistance", m_FogEndDistance_RenderThread, mappedResource.pData, varBindings );
	m_pDeviceContext->Unmap( pConstantBuffer, 0 );

	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	ID3D11ShaderResourceView * nullArray[] = { nullptr };

	m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );

	ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView, m_RenderTargets[COLOR_BUFFER].m_pShaderResourceView };
	m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
}

/**
 *	kbRenderer_DX11::RenderConsole
 */
void kbRenderer_DX11::RenderConsole() {
	if ( m_bRenderToHMD == false ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &m_pCurrentRenderWindow->m_pRenderTargetView, m_pDepthStencilView );
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

	SetShaderMat4( "modeMatrix", kbMat4::identity, mappedResource.pData, varBindings );
	SetShaderMat4( "modelViewMatrix", kbMat4::identity, mappedResource.pData, varBindings );
	SetShaderMat4( "viewMatrix", m_pCurrentRenderWindow->m_ViewMatrix, mappedResource.pData, varBindings );

	kbMat4 mvpMatrix = kbMat4::identity;
	if ( m_bRenderToHMD ) {
		mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
	}
	SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );
	SetShaderMat4( "projection", m_pCurrentRenderWindow->m_ViewProjectionMatrix, mappedResource.pData, varBindings );
	SetShaderMat4( "inverseProjection", m_pCurrentRenderWindow->m_InverseProjectionMatrix, mappedResource.pData, varBindings );
	SetShaderVec4( "cameraPosition", m_pCurrentRenderWindow->m_CameraPosition, mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	ID3D11ShaderResourceView * nullArray[] = { nullptr };

	m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );

	ID3D11ShaderResourceView * nullarray[] = { nullptr, nullptr };//'{ m_RenderTargets[ACCUMULATION_BUFFER].m_pShaderResourceView, m_RenderTargets[COLOR_BUFFER].m_pShaderResourceView };
	m_pDeviceContext->PSSetShaderResources( 0, 2, nullarray );
}

/**
 *	kbRenderer_DX11::LoadTexture
 */
bool kbRenderer_DX11::LoadTexture( const char * name, int index, int width, int height ) {
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


void ReadShaderFile( const std::string & shaderText, kbShaderVarBindings_t *const pShaderBindings ) {
	
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
	const std::string delimiters = "\t ;\n";

    std::string::size_type startPos = bufferBlock.find_first_not_of( delimiters, 0 );
    std::string::size_type endPos = bufferBlock.find_first_of( delimiters, startPos );

    while ( startPos != std::string::npos || endPos != std::string::npos ) {

        constantBufferStrings.push_back( bufferBlock.substr( startPos, endPos - startPos ) );

        startPos = bufferBlock.find_first_not_of( delimiters, endPos );
        endPos = bufferBlock.find_first_of( delimiters, startPos );
    }

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
		pShaderBindings->m_VarBindings.push_back( kbShaderVarBindings_t::binding_t( varName, currOffset ) );

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

        std::string::size_type startPos = shaderText.find_first_not_of( delimiters, texturePos + 9 );
        std::string::size_type endPos = shaderText.find_first_of( delimiters, startPos );

        if ( startPos == std::string::npos || endPos == std::string::npos ) {
            break;
        }

        texturePos = shaderText.find( "Texture2D", texturePos + 1 );
    }
}

/**
 *	kbRenderer_DX11::LoadShader
 */
void kbRenderer_DX11::LoadShader( const std::string & fileName, ID3D11VertexShader *& vertexShader, ID3D11PixelShader *& pixelShader, 
								  ID3D11InputLayout *& vertexLayout, const std::string & vertexShaderFunc, const std::string & pixelShaderFunc, 
								  kbShaderVarBindings_t * pShaderBindings ) {

	HRESULT hr;
	struct shaderBlobs_t {
		~shaderBlobs_t() {
			SAFE_RELEASE(errorMessage)
			SAFE_RELEASE(vertexShaderBuffer)
			SAFE_RELEASE(pixelShaderBuffer)
		}

		ID3D10Blob * errorMessage = nullptr;
		ID3D10Blob * vertexShaderBuffer = nullptr;
		ID3D10Blob * pixelShaderBuffer = nullptr;
	} localBlobs;

	std::ifstream shaderFile;
	shaderFile.open( fileName.c_str(), std::fstream::in );	
	const std::string readBuffer( ( std::istreambuf_iterator<char>(shaderFile) ), std::istreambuf_iterator<char>() );
	shaderFile.close();

	if ( pShaderBindings != nullptr ) {
		ReadShaderFile( readBuffer, pShaderBindings );

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

	// Compile vertex shader
	const UINT shaderFlags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_ALL_RESOURCES_BOUND;

	int numTries = 0;
	do {
		numTries++;

		SAFE_RELEASE( localBlobs.errorMessage );
		hr = D3DCompile( readBuffer.c_str(), readBuffer.length(), nullptr, nullptr, nullptr, vertexShaderFunc.c_str(), "vs_5_0", shaderFlags, 0, &localBlobs.vertexShaderBuffer, &localBlobs.errorMessage );
		if ( FAILED( hr )  ) {
			Sleep( 250 );
			SAFE_RELEASE( localBlobs.vertexShaderBuffer );

		}
	} while ( FAILED( hr ) && numTries < 4 );

	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to load vertex shader : %s", ( localBlobs.errorMessage != nullptr ) ? ( localBlobs.errorMessage->GetBufferPointer() ) : ( "No error message given " ) );
		return;
	}

	SAFE_RELEASE( localBlobs.errorMessage );

	// Compile pixel shader
	numTries = 0;
	do {
		numTries++;

		SAFE_RELEASE( localBlobs.errorMessage )
		hr = D3DCompile( readBuffer.c_str(), readBuffer.length(), nullptr, nullptr, nullptr, pixelShaderFunc.c_str(), "ps_5_0", shaderFlags, 0, &localBlobs.pixelShaderBuffer, &localBlobs.errorMessage );
		if ( FAILED( hr ) ) {
			Sleep( 250 );
			SAFE_RELEASE( localBlobs.pixelShaderBuffer );
		}
	} while ( FAILED( hr ) && numTries < 4 );

	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to load pixel shader : %s", ( localBlobs.errorMessage != nullptr ) ? localBlobs.errorMessage->GetBufferPointer() : ( "No error message given " ) );
		return;
	}

	SAFE_RELEASE( localBlobs.errorMessage );

	hr = m_pD3DDevice->CreateVertexShader( localBlobs.vertexShaderBuffer->GetBufferPointer(), localBlobs.vertexShaderBuffer->GetBufferSize(), nullptr, &vertexShader );
	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create vertex shader %s", fileName.c_str() );
		return;
	}

	hr = m_pD3DDevice->CreatePixelShader( localBlobs.pixelShaderBuffer->GetBufferPointer(), localBlobs.pixelShaderBuffer->GetBufferSize(), nullptr, &pixelShader );
	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create pixel shader %s", fileName.c_str() );
		SAFE_RELEASE( vertexShader );
		return;
	}

	D3D11_INPUT_ELEMENT_DESC polygonLayout[5];

	if ( fileName.find("Particle") != std::string::npos )
	{
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
	}
	else if (fileName.find("Skinned") != std::string::npos || vertexShaderFunc.find( "skin" ) != std::string::npos )
	{
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
		polygonLayout[3].Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
	else
	{
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
		polygonLayout[3].Format = DXGI_FORMAT_B8G8R8A8_UNORM;//DXGI_FORMAT_R8G8B8A8_UNORM;
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

	const int numElements = sizeof( polygonLayout ) / sizeof( polygonLayout[ 0 ] );
	hr = m_pD3DDevice->CreateInputLayout( polygonLayout, numElements, localBlobs.vertexShaderBuffer->GetBufferPointer(), localBlobs.vertexShaderBuffer->GetBufferSize(), &vertexLayout );
	if ( FAILED( hr ) ) {
		kbWarning( "kbRenderer_DX11::LoadShader() - Failed to create input layout" );

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

	m_RenderState.SetBlendState( false,
								 false,
								 true,
								 kbRenderState::BF_SourceAlpha,
								 kbRenderState::BF_InvSourceAlpha,
								 kbRenderState::BO_Add,
								 kbRenderState::BF_One,
								 kbRenderState::BF_Zero,
								 kbRenderState::BO_Add );

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
	const float xScale = size_x / m_RenderWindowList[0]->m_fViewPixelWidth;
	const float yScale =  size_y / m_RenderWindowList[0]->m_fViewPixelHeight;
	const float xPos = xScale + start_x / m_RenderWindowList[0]->m_fViewPixelHalfWidth;
	const float yPos = yScale + start_y / m_RenderWindowList[0]->m_fViewPixelHalfHeight;

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
 *	kbRenderer_DX11::DrawBillboard
 */
void kbRenderer_DX11::DrawBillboard( const kbVec3 & position, const kbVec2 & size, const int textureIndex, kbShader *const pShader, const int entityId ) {
	debugDrawObject_t billboard;
	billboard.m_Position = position;
	billboard.m_Scale.Set( size.x, size.y, size.x );
	billboard.m_pShader = pShader;
	billboard.m_TextureIndex = textureIndex;
	billboard.m_EntityId = entityId;

	m_DebugBillboards_GameThread.push_back( billboard );
}

/**
 *	kbRenderer_DX11::DrawModel
 */
void kbRenderer_DX11::DrawModel( const kbModel * pModel, const kbVec3 & position, const kbQuat & orientation, const kbVec3 & scale, const int entityId ) {
	debugDrawObject_t model;
	model.m_Position = position;
	model.m_Orientation = orientation;
	model.m_Scale = scale;
	model.m_pModel = pModel;
	model.m_EntityId = entityId;

	m_DebugModels_GameThread.push_back( model );
}

/**
 *	kbRenderer_DX11::SetRenderViewTransform
 */
void kbRenderer_DX11::SetRenderViewTransform( const HWND hwnd, const kbVec3 & position, const kbQuat & rotation ) {
	int viewIndex = -1;

	if ( hwnd == nullptr ) {
		viewIndex = 0;
	} else {
		for ( int i = 0 ; i < m_RenderWindowList.size(); i++ ) {
			if ( m_RenderWindowList[i]->m_Hwnd == hwnd ) {
				viewIndex = i;
				break;
			}
		}
	}

	if ( viewIndex < 0 || viewIndex >= m_RenderWindowList.size() ) {
		kbError( "Invalid view index" );
	}

	m_RenderWindowList[viewIndex]->m_CameraPosition_GameThread = position;
	m_RenderWindowList[viewIndex]->m_CameraRotation_GameThread = rotation;
}

/**
 *	kbRenderer_DX11::GetRenderViewTransform
 */
void kbRenderer_DX11::GetRenderViewTransform( const HWND hwnd, kbVec3 & position, kbQuat & rotation ) {
	int viewIndex = -1;

	if ( hwnd == nullptr ) {
		viewIndex = 0;
	} else {
		for ( int i = 0 ; i < m_RenderWindowList.size(); i++ ) {
			if ( m_RenderWindowList[i]->m_Hwnd == hwnd ) {
				viewIndex = i;
				break;
			}
		}
	}

	if ( viewIndex < 0 || viewIndex >= m_RenderWindowList.size() ) {
		kbError( "Invalid view index" );
	}

	position = m_RenderWindowList[viewIndex]->m_CameraPosition;
	rotation = m_RenderWindowList[viewIndex]->m_CameraRotation;
}

/**
 *	kbRenderer_DX11::RenderModel
 */
void kbRenderer_DX11::RenderModel( const kbRenderObject *const pRenderObject, const ERenderPass renderpass, const bool bShadowPass ) {
	kbErrorCheck( pRenderObject != nullptr && pRenderObject->m_pModel != nullptr, "kbRenderer_DX11::RenderModel() - no model found" );
	kbErrorCheck( pRenderObject->m_pModel->GetMaterials().size() > 0, "kbRenderer_DX11::RenderModel() - No materials found for model %s", pRenderObject->m_pModel->GetFullName() );

	const kbModel *const modelToRender = pRenderObject->m_pModel;

	kbMat4 worldMatrix;
	worldMatrix.MakeScale( pRenderObject->m_Scale );
	worldMatrix *= pRenderObject->m_Orientation.ToMat4();
	worldMatrix[3] = pRenderObject->m_Position;

	const UINT vertexStride = pRenderObject->m_pModel->VertexStride();
	const UINT vertexOffset = 0;	
	ID3D11Buffer *const vertexBuffer = ( ID3D11Buffer * const ) modelToRender->m_VertexBuffer.GetBufferPtr();
	ID3D11Buffer *const indexBuffer = ( ID3D11Buffer * const ) modelToRender->m_IndexBuffer.GetBufferPtr();

	m_pDeviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &vertexStride, &vertexOffset );
	m_pDeviceContext->IASetIndexBuffer( indexBuffer, DXGI_FORMAT_R32_UINT, 0 );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	for ( int i = 0; i < modelToRender->NumMeshes(); i++ ) {
		const kbMaterial & modelMaterial = modelToRender->GetMaterials()[modelToRender->GetMeshes()[i].m_MaterialIndex];

		if ( m_ViewMode == ViewMode_Wireframe ) {
			m_pDeviceContext->RSSetState( m_pWireFrameRasterizerState );
		} else if ( modelMaterial.GetCullingMode() == kbMaterial::CM_BackFaces ) {
			m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );
		} else if ( modelMaterial.GetCullingMode() == kbMaterial::CM_None ) {
			m_pDeviceContext->RSSetState( m_pNoFaceCullingRasterizerState );
		} else {
			kbError( "kbRenderer_DX11::RenderModel() - Unsupported culling mode" );
		}

		// Get Shader
		const kbShader * pShader = modelMaterial.GetShader();
		const std::vector<kbShader *> *const pShaderOverrideList = &pRenderObject->m_OverrideShaderList;	
	
		if ( bShadowPass ) {
			if ( pRenderObject->m_bIsSkinnedModel ) {
				pShader = m_pSkinnedDirectionalLightShadowShader;
			} else {
				pShader = m_pDirectionalLightShadowShader;
			}
	
		} else {
			if ( pShaderOverrideList != nullptr && pShaderOverrideList->size() > i ) {
				pShader = (*pShaderOverrideList)[i];
			}
	
			if ( pShader == nullptr || pShader->GetPixelShader() == nullptr ) {
				pShader = m_pMissingShader;
			}
		}
	
		m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
		m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	
		if ( renderpass == RP_MousePicker ) {
	
			m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pMousePickerIdShader->GetPixelShader(), nullptr, 0 );
	
			ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( 32 );

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			kbErrorCheck( SUCCEEDED(hr), "Failed to map matrix buffer" );
			UINT *const pEntityId = (UINT*)mappedResource.pData;
			*pEntityId = pRenderObject->m_EntityId;

			UINT *const pGroupId = pEntityId + 1;
			*pGroupId = (UINT)i;

			m_pDeviceContext->Unmap( pConstantBuffer, 0 );
			m_pDeviceContext->PSSetConstantBuffers( 1, 1, &pConstantBuffer );
	
		} else {
			m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );
		}
	

		// Set textures
		ID3D11ShaderResourceView *const texture = (modelMaterial.GetTexture() != nullptr)?(ID3D11ShaderResourceView *)modelMaterial.GetTexture()->GetGPUTexture() : ( nullptr );
		m_pDeviceContext->PSSetShaderResources( 0, 1, &texture );
		m_pDeviceContext->PSSetSamplers( 0, 1, &m_pBasicSamplerState );

		// Get a valid constant buffer and bind the kbShader's vars to it
		const kbShaderVarBindings_t & shaderVarBindings = pShader->GetShaderVarBindings();
		std::map<size_t, ID3D11Buffer *>::iterator constantBufferIt = m_ConstantBuffers.find( shaderVarBindings.m_ConstantBufferSizeBytes );
		kbErrorCheck( constantBufferIt != m_ConstantBuffers.end() && constantBufferIt->second != nullptr, "kbRenderer_DX11::RenderModel() - Could not find constant buffer for shader %s", pShader->GetFullFileName() );

		ID3D11Buffer *const pConstantBuffer = constantBufferIt->second;
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

		const auto & bindings = shaderVarBindings.m_VarBindings;
		byte * constantPtr = (byte*) mappedResource.pData;
		for ( int i = 0; i < bindings.size(); i++ ) {
			const std::string & varName = bindings[i].m_VarName;
			const byte * pVarByteOffset = constantPtr + bindings[i].m_VarByteOffset;
			if ( varName == "mvpMatrix" ) {
				kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
				*pMatOffset = worldMatrix * m_pCurrentRenderWindow->m_ViewProjectionMatrix;
			} else if ( varName == "modelMatrix" ) {
				kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
				*pMatOffset = worldMatrix;
			} else if ( varName == "cameraPos" ) {
				kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
				*pVecOffset = m_pCurrentRenderWindow->m_CameraPosition;
			} else if ( varName == "viewProjection" ) {
				kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
				*pMatOffset = m_pCurrentRenderWindow->m_ViewProjectionMatrix;
			} else if ( varName == "BoneMatrices" ) {
				if ( pRenderObject->m_bIsSkinnedModel ) {
					kbMat4 *const boneMatrices = (kbMat4*)pVarByteOffset;
					const kbSkinnedRenderObject *const skinnedRenderObj = static_cast<const kbSkinnedRenderObject*>( pRenderObject );
					for ( int i = 0; i < skinnedRenderObj->m_BoneMatrices.size() && i < Max_Shader_Bones; i++ ) {
						boneMatrices[i].MakeIdentity();
						boneMatrices[i][0] = skinnedRenderObj->m_BoneMatrices[i].GetAxis(0);
						boneMatrices[i][1] = skinnedRenderObj->m_BoneMatrices[i].GetAxis(1);
						boneMatrices[i][2] = skinnedRenderObj->m_BoneMatrices[i].GetAxis(2);
						boneMatrices[i][3] = skinnedRenderObj->m_BoneMatrices[i].GetAxis(3);
						
						boneMatrices[i][0].w = 0;
						boneMatrices[i][1].w = 0;
						boneMatrices[i][2].w = 0;
					}
				}
			} else {
                const std::vector<kbShaderParamOverrides_t::kbShaderParam_t> & paramOverrides = pRenderObject->m_ShaderParamOverrides.m_ParamOverrides;
                for ( int iOverride = 0; iOverride < paramOverrides.size(); iOverride++ ) {
                    const kbShaderParamOverrides_t::kbShaderParam_t & curOverride = paramOverrides[iOverride];
                    const std::string & overrideVarName = curOverride.m_VarName;
                    if ( varName == overrideVarName ) {

                        // Check if it doesn't fit
                        const size_t endOffset = curOverride.m_VarSizeBytes + bindings[iOverride].m_VarByteOffset ;
                        if ( endOffset > shaderVarBindings.m_ConstantBufferSizeBytes || ( i < bindings.size() - 1 && endOffset > bindings[i+1].m_VarByteOffset ) ) {
                            break;
                        }
                   
                        switch( curOverride.m_Type ) {
                            case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_MAT4 : {
                                kbMat4 *const pMatOffset = (kbMat4*)pVarByteOffset;
				                *pMatOffset = curOverride.m_Mat4;
                                break;
                            }

                            case kbShaderParamOverrides_t::kbShaderParam_t::SHADER_VEC4 : {
                                kbVec4 *const pVecOffset = (kbVec4*)pVarByteOffset;
				                *pVecOffset = curOverride.m_Vec4;
                                break;
                            }
                        }
                    }
                }
            }
		}

        // Bind textures
        const std::vector<kbShaderParamOverrides_t::kbShaderParam_t> & paramOverrides = pRenderObject->m_ShaderParamOverrides.m_ParamOverrides;
        for ( int iOverride = 0; iOverride < paramOverrides.size(); iOverride++ ) {
            const kbShaderParamOverrides_t::kbShaderParam_t & curOverride = paramOverrides[iOverride];
            if ( curOverride.m_Type == kbShaderParamOverrides_t::kbShaderParam_t::SHADER_TEX ) {
                ID3D11ShaderResourceView *const pShaderResourceView = ( curOverride.m_pTexture != nullptr ) ? ( curOverride.m_pTexture->GetGPUTexture() ) : ( nullptr );
	            m_pDeviceContext->PSSetShaderResources( iOverride, 1, &pShaderResourceView );
            }
        }

		m_pDeviceContext->Unmap( pConstantBuffer, 0 );
		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );
		m_pDeviceContext->DrawIndexed( modelToRender->GetMeshes()[i].m_NumTriangles * 3, modelToRender->GetMeshes()[i].m_IndexBufferIndex, 0 );
	}
}
	
/**
 *	kbRenderer_DX11::DrawScreenSpaceQuad
 */
void kbRenderer_DX11::DrawScreenSpaceQuad( const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader *const pShader ) {
	ScreenSpaceQuad_t quadToAdd;
	quadToAdd.m_Pos.x = start_x;
	quadToAdd.m_Pos.y = start_y;
	quadToAdd.m_Size.x = size_x;
	quadToAdd.m_Size.y = size_y;
	quadToAdd.m_pShader = pShader;
	quadToAdd.m_TextureIndex = textureIndex;

	m_ScreenSpaceQuads_GameThread.push_back( quadToAdd );
}

#define AddVert( vert ) drawVert.position = vert; m_DebugLines_GameThread.push_back( drawVert );

/**
 *	kbRenderer_DX11::DrawLine
 */
void kbRenderer_DX11::DrawLine( const kbVec3 & start, const kbVec3 & end, const kbColor & color ) {

	/*if ( m_DebugLines_GameThread.size() >= m_DebugLines_GameThread.capacity() - 2 ) {
		return;
	}*/

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	AddVert( start );
	AddVert( end );
}

/**
 *	kbRenderer_DX11::DrawBox
 */
void kbRenderer_DX11::DrawBox( const kbBounds & bounds, const kbColor & color ) {

	const kbVec3 maxVert = bounds.Max();
	const kbVec3 minVert = bounds.Min();

	const kbVec3 LTF( minVert.x, maxVert.y, maxVert.z );
	const kbVec3 RTF( maxVert.x, maxVert.y, maxVert.z );
	const kbVec3 RBF( maxVert.x, minVert.y, maxVert.z );
	const kbVec3 LBF( minVert.x, minVert.y, maxVert.z );
	const kbVec3 LTB( minVert.x, maxVert.y, minVert.z );
	const kbVec3 RTB( maxVert.x, maxVert.y, minVert.z );
	const kbVec3 RBB( maxVert.x, minVert.y, minVert.z );
	const kbVec3 LBB( minVert.x, minVert.y, minVert.z );

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	AddVert( LTF ); AddVert( RTF );
	AddVert( RTF ); AddVert( RBF );
	AddVert( RBF ); AddVert( LBF );
	AddVert( LBF ); AddVert( LTF );

	AddVert( LTB ); AddVert( RTB );
	AddVert( RTB ); AddVert( RBB );
	AddVert( RBB ); AddVert( LBB );
	AddVert( LBB ); AddVert( LTB );

	AddVert( LTF ); AddVert( LTB );
	AddVert( RTF ); AddVert( RTB );
	AddVert( LBF ); AddVert( LBB );
	AddVert( RBF ); AddVert( RBB );
}

/**
 *	kbRenderer_DX11::DrawSphere
 */
void kbRenderer_DX11::DrawSphere( const kbVec3 & origin, const float radius, const int InNumSegments, const kbColor & color ) {
	const int numSegments = max( InNumSegments, 4 );
	const float angleInc = 2.0f * kbPI / (float) numSegments;
	float latitude = angleInc;
	float curSin = 0, curCos = 1.0f;
	//float cosX, sinX;
	kbVec3 pt1, pt2, pt3, pt4;

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	for ( int curYSeg = 0; curYSeg < numSegments; curYSeg++ ) {
		const float nextSin = sin( latitude );
		const float nextCos = cos( latitude );

		pt1 = kbVec3( curSin, curCos, 0.0f ) * radius + origin;
		pt3 = kbVec3( nextSin, nextCos, 0.0f ) * radius + origin;
		float longitude = angleInc;
		for ( int curXSeg = 0; curXSeg < numSegments; curXSeg++ ) {
			float sinX = sin( longitude );
			float cosX = cos( longitude );

			pt2 = kbVec3( cosX * curSin, curCos, sinX * curSin ) * radius + origin;
			pt4 = kbVec3( cosX * nextSin, nextCos, sinX * nextSin ) * radius + origin;
			AddVert( pt1 ); AddVert( pt2 );
			AddVert( pt1 ); AddVert( pt3 );
			pt1 = pt2;
			pt3 = pt4;
			longitude += angleInc;
		}

		curSin = nextSin;
		curCos = nextCos;
		latitude += angleInc;
	}
}


/*
 *	kbRenderer_DX11::DrawPreTransformedLine
 */
void kbRenderer_DX11::DrawPreTransformedLine( const std::vector<kbVec3> & vertList, const kbColor & color ) {
	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	for ( int i = 0; i < vertList.size(); i++ ) {
		drawVert.position = vertList[i];
		m_DebugPreTransformedLines.push_back( drawVert );
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

	if ( FAILED( hr ) ) {
		kbError( "Failed to map debug lines" );
	}

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

	if ( FAILED( hr ) ) {
		kbError( "Failed to map debug lines" );
	}

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

	SetShaderMat4( "mvpMatrix", m_pCurrentRenderWindow->m_ViewProjectionMatrix, mappedResource.pData, varBindings );

	m_pDeviceContext->Unmap( pConstantBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );

	m_pDeviceContext->Draw( ( UINT )m_DebugLines.size(), 0 );
}

/*
 *	kbRenderer_DX11::RenderDebugBillboards
 */
void kbRenderer_DX11::RenderDebugBillboards( const bool bIsEntityIdPass ) {
	
	if ( m_DebugBillboards.size() == 0 ) {
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

		const kbMat4 preRotationMatrix = m_pCurrentRenderWindow->m_CameraRotation.ToMat4();
		kbMat4 mvpMatrix;
		mvpMatrix.MakeScale( currBillBoard.m_Scale );
		mvpMatrix[3] = currBillBoard.m_Position;
		mvpMatrix = preRotationMatrix * mvpMatrix * m_pCurrentRenderWindow->m_ViewProjectionMatrix;
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

/*
 *	kbRenderer_DX11::DrawDebugText
 */
void kbRenderer_DX11::DrawDebugText( const std::string & theString, const float X, const float Y, const float ScreenCharW, const float ScreenCharH, const kbColor & Color ) {

	m_DebugStrings_GameThread.push_back( kbTextInfo_t() );

	kbTextInfo_t & newTextInfo = m_DebugStrings_GameThread[m_DebugStrings_GameThread.size() - 1];
	newTextInfo.TextInfo = theString;
	newTextInfo.screenX = X;
	newTextInfo.screenY = Y;
	newTextInfo.screenW = ScreenCharW;
	newTextInfo.screenH = ScreenCharH;
	newTextInfo.color = Color;
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
	
	m_pDeviceContext->CopyResource( m_pOffScreenRenderTargetTexture, m_RenderTargets[eRenderTargetTexture::MOUSE_PICKER_BUFFER].m_pRenderTargetTexture );
	
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
