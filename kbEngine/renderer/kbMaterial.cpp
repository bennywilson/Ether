//===================================================================================================
// kbMaterial.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <Wincodec.h>
#include "kbCore.h"
#include "kbRenderer_defs.h"
#include "kbRenderer.h"
#include "DX11/kbRenderer_DX11.h"	//	TODO HACK
#include "kbMaterial.h"

extern ID3D11DeviceContext * g_pImmediateContext;

// Code to initialize a texture using the Windows Imaging Component from https://msdn.microsoft.com/en-us/library/windows/desktop/ff476904(v=vs.85).aspx
template<class T> class ScopedObject {

public:
	explicit ScopedObject( T *const p = nullptr ) : _pointer(p) {}
	~ScopedObject() {
	    if ( _pointer != nullptr ) {
	        _pointer->Release();
	        _pointer = nullptr;
	    }
	}

	T* operator->() { return _pointer; }
	T** operator&() { return &_pointer; }

	T* Get() const { return _pointer; }

private:

	ScopedObject( const ScopedObject & );
	ScopedObject& operator=( const ScopedObject & );
    
	T * _pointer;
};

struct WICTranslate {
	GUID wic;
	DXGI_FORMAT format;
};

static WICTranslate g_WICFormats[] =  {
    { GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

    { GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
    { GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

    { GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
    { GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

    { GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
    { GUID_WICPixelFormat32bppRGBE,             DXGI_FORMAT_R9G9B9E5_SHAREDEXP },

#ifdef DXGI_1_2_FORMATS

    { GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
    { GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

#endif // DXGI_1_2_FORMATS

    { GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
    { GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
    { GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
    { GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

    { GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
    { GUID_WICPixelFormat96bppRGBFloat,         DXGI_FORMAT_R32G32B32_FLOAT },
#endif
};

//-------------------------------------------------------------------------------------
// WIC Pixel Format nearest conversion table
//-------------------------------------------------------------------------------------
struct WICConvert {
	GUID source;
	GUID target;
};

static WICConvert g_WICConvert[] = {
	// Note target GUID in this conversion table must be one of those directly supported formats (above).

	{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
	
	{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

	{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 
	{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 

	{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT 
	{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT 

#ifdef DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

#else

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat16bppBGRA5551,         GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat16bppBGR565,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

#endif // DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM
	
	{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	
	{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	
	{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	
	{ GUID_WICPixelFormat96bppRGBFixedPoint,    GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	
	{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
#endif

    // We don't support n-channel formats
};

//---------------------------------------------------------------------------------
static DXGI_FORMAT _WICToDXGI( const GUID & guid ) {

	for( size_t i=0; i < _countof(g_WICFormats); ++i ) {
		if ( memcmp( &g_WICFormats[i].wic, &guid, sizeof(GUID) ) == 0 ) {
			return g_WICFormats[i].format;
		}
	}
	
	return DXGI_FORMAT_UNKNOWN;
}

static IWICImagingFactory* _GetWIC() {
	static IWICImagingFactory* s_Factory = nullptr;

	if ( s_Factory != nullptr ) {
	    return s_Factory;
	}

	HRESULT hr = CoCreateInstance(
	    CLSID_WICImagingFactory,
	    nullptr,
	    CLSCTX_INPROC_SERVER,
	    __uuidof(IWICImagingFactory),
	    (LPVOID*)&s_Factory
	    );

	if ( FAILED( hr ) ) {
		s_Factory = nullptr;
		return nullptr;
	}

	return s_Factory;
}

//---------------------------------------------------------------------------------
static size_t _WICBitsPerPixel( REFGUID targetGuid ) {
	IWICImagingFactory *const pWIC = _GetWIC();
	if ( pWIC == nullptr ) {
	    return 0;
	}

	ScopedObject<IWICComponentInfo> cinfo;
	if ( FAILED( pWIC->CreateComponentInfo( targetGuid, &cinfo ) ) ) {
	    return 0;
	}

	WICComponentType type;
	if ( FAILED( cinfo->GetComponentType( &type ) ) ) {
	    return 0;
	}

	if ( type != WICPixelFormat ) {
	    return 0;
	}

	ScopedObject<IWICPixelFormatInfo> pfinfo;
	if ( FAILED( cinfo->QueryInterface( __uuidof(IWICPixelFormatInfo), reinterpret_cast<void**>( &pfinfo )  ) ) ) {
	    return 0;
	}

	UINT bpp;
	if ( FAILED( pfinfo->GetBitsPerPixel( &bpp ) ) ) {
	    return 0;
	}

	return bpp;
}


static HRESULT CreateTextureFromWIC( IWICBitmapFrameDecode *const frame,
									 ID3D11Resource ** texture,
									 ID3D11ShaderResourceView ** textureView,
									 size_t maxsize,
									 const kbString & fileName,
									 std::unique_ptr<uint8_t[]> & temp,
									 UINT & twidth,
									 UINT & theight ) {

	twidth = 0;
	theight = 0;

	UINT width, height;
	HRESULT hr = frame->GetSize( &width, &height );
	if ( FAILED( hr ) ) {
		return hr;
	}
	
	kbErrorCheck( width > 0 && height > 0, "CreateTextureFromWIC() - Invalid width and/or height %d x %d", width, height );
	
	if ( maxsize == 0 ) {
		maxsize = 8192;
	}
	
	if ( width > maxsize || height > maxsize ) {
		const float aspectRatio = static_cast<float>(height) / static_cast<float>(width);
		if ( width > height ) {
			twidth = static_cast<UINT>(maxsize);
			theight = static_cast<UINT>(static_cast<float>(maxsize) * aspectRatio);
		}
		else {
			theight = static_cast<UINT>(maxsize);
			twidth = static_cast<UINT>(static_cast<float>(maxsize) / aspectRatio);
		}
		kbErrorCheck( twidth <= maxsize && theight <= maxsize, "CreateTextureFromWIC() - Invalid width and/or height %d x %d", width, height );
	}
	else {
		twidth = width;
		theight = height;
	}

	// Determine format
	WICPixelFormatGUID pixelFormat;
	hr = frame->GetPixelFormat( &pixelFormat );
	if ( FAILED( hr ) ) {
		return hr;
	}
	
	WICPixelFormatGUID convertGUID;
	memcpy( &convertGUID, &pixelFormat, sizeof(WICPixelFormatGUID) );

	size_t bpp = 0;

	DXGI_FORMAT format = _WICToDXGI( pixelFormat );
	if ( format == DXGI_FORMAT_UNKNOWN ) {
		for( size_t i=0; i < _countof(g_WICConvert); ++i ) {
		    if ( memcmp( &g_WICConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 ) {
				memcpy( &convertGUID, &g_WICConvert[i].target, sizeof(WICPixelFormatGUID) );

				format = _WICToDXGI( g_WICConvert[i].target );
				kbErrorCheck( format != DXGI_FORMAT_UNKNOWN, "CreateTextureFromWIC() - Unkown format" );
				bpp = _WICBitsPerPixel( convertGUID );
				break;
		    }
		}

		if ( format == DXGI_FORMAT_UNKNOWN ) {
			return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
		}
	} else {
		bpp = _WICBitsPerPixel( pixelFormat );
	}

	if ( bpp == 0 ) {
		return E_FAIL;
	}
	
	// Verify our target format is supported by the current device
	// (handles WDDM 1.0 or WDDM 1.1 device driver cases as well as DirectX 11.0 Runtime without 16bpp format support)
	UINT support = 0;
	hr = g_pD3DDevice->CheckFormatSupport( format, &support );
	if ( FAILED(hr) || !(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) ) {
		// Fallback to RGBA 32-bit format which is supported by all devices
		memcpy( &convertGUID, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID) );
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bpp = 32;
	}

	// Allocate temporary memory for image
	size_t rowPitch = ( twidth * bpp + 7 ) / 8;
	size_t imageSize = rowPitch * theight;
	
	temp = std::unique_ptr<uint8_t[]>( new uint8_t[ imageSize ] );
	
	// Load image data
	if ( memcmp( &convertGUID, &pixelFormat, sizeof(GUID) ) == 0 && twidth == width && theight == height ) {
		// No format conversion or resize needed
		hr = frame->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
		if ( FAILED(hr) ) {
			return hr;
		}
	} else if ( twidth != width || theight != height ) {
		// Resize
		IWICImagingFactory *const pWIC = _GetWIC();
		if ( pWIC == nullptr ) {
		    return E_NOINTERFACE;
		}

		ScopedObject<IWICBitmapScaler> scaler;
		hr = pWIC->CreateBitmapScaler( &scaler );
		if ( FAILED(hr) ) {
			return hr;
		}
		
		hr = scaler->Initialize( frame, twidth, theight, WICBitmapInterpolationModeFant );
		if ( FAILED(hr) ) {
			return hr;
		}
		
		WICPixelFormatGUID pfScaler;
		hr = scaler->GetPixelFormat( &pfScaler );
		if ( FAILED(hr) ) {
			return hr;
		}
	
		if ( memcmp( &convertGUID, &pfScaler, sizeof(GUID) ) == 0 ) {
			// No format conversion needed
			hr = scaler->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
			if ( FAILED(hr) ) {
				return hr;
			}
		} else {
			ScopedObject<IWICFormatConverter> FC;
			hr = pWIC->CreateFormatConverter( &FC );
			if ( FAILED(hr) ) {
				return hr;
			}

			hr = FC->Initialize( scaler.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
			if ( FAILED(hr) ) {
				return hr;
			}

			hr = FC->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
			if ( FAILED(hr) ) {
				return hr;
			}
	    }
	}
	else {
		// Format conversion but no resize
		IWICImagingFactory *const pWIC = _GetWIC();
		if ( pWIC == nullptr ) {
			return E_NOINTERFACE;
		}

		ScopedObject<IWICFormatConverter> FC;
		hr = pWIC->CreateFormatConverter( &FC );
		if ( FAILED(hr) ) {
			return hr;
		}
		
		hr = FC->Initialize( frame, convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
		if ( FAILED(hr) ) {
			return hr;
		}
		
		hr = FC->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
		if ( FAILED(hr) ) {
			return hr;
		}
	}
	
	// See if format is supported for auto-gen mipmaps (varies by feature level)
	bool autogen = false;
	if ( textureView != nullptr ) {
		UINT fmtSupport = 0;
		hr = g_pD3DDevice->CheckFormatSupport( format, &fmtSupport );
		if ( SUCCEEDED(hr) && ( fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) ) {
			autogen = true;
		}
	}
	
	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = twidth;
	desc.Height = theight;
	desc.MipLevels = (autogen) ? 0 : 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = (autogen) ? (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET) : (D3D11_BIND_SHADER_RESOURCE);
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = (autogen) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
	
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = temp.get();
	initData.SysMemPitch = static_cast<UINT>( rowPitch );
	initData.SysMemSlicePitch = static_cast<UINT>( imageSize );
	
	ID3D11Texture2D* tex = nullptr;
	hr = g_pD3DDevice->CreateTexture2D( &desc, (autogen) ? nullptr : &initData, &tex );
	if ( SUCCEEDED(hr) && tex != nullptr ) {
	    if ( textureView !=  nullptr ) {
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			memset( &SRVDesc, 0, sizeof( SRVDesc ) );
			SRVDesc.Format = format;
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = (autogen) ? -1 : 1;

			hr = g_pD3DDevice->CreateShaderResourceView( tex, &SRVDesc, textureView );
			if ( FAILED(hr) ) {
				tex->Release();
				return hr;
			}
			
			if ( autogen == true ) {
				g_pImmediateContext->UpdateSubresource( tex, 0, nullptr, temp.get(), static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize) );
				g_pImmediateContext->GenerateMips( *textureView );
			}
		}
	
	    if ( texture != nullptr ) {
	        *texture = tex;
	    } else {
#if defined(_DEBUG) || defined(PROFILE)
			tex->SetPrivateData( WKPDID_D3DDebugObjectName,
								 static_cast<UINT>( fileName.stl_str().length() ),
								 fileName.c_str()	);
#endif
			tex->Release();
		}
	}

    return hr;
}

/**
 *	kbTexture:: kbTexture
 */
kbTexture::kbTexture() :
	m_pGPUTexture( nullptr ),
	m_bIsCPUTexture( false ),
	m_TextureWidth( 0 ),
	m_TextureHeight( 0 ) {
}

/**
 *	kbTexture:: kbTexture
 */
kbTexture::kbTexture( const kbString & fileName ) :
	m_pGPUTexture( nullptr ),
	m_bIsCPUTexture( false ),
	m_TextureWidth( 0 ),
	m_TextureHeight( 0 ) {

	m_FullFileName = fileName.stl_str();
	m_FullName = kbString( m_FullFileName );

	Load_Internal();
}

/**
 *	kbTexture::Load_Internal
 */
bool kbTexture::Load_Internal() {
	if ( g_pD3DDevice == nullptr ) {
		return false;
	}

	IWICImagingFactory *const pWIC = _GetWIC();
	if ( pWIC == nullptr ) {
		return false;
	}

    // Initialize WIC
	const std::wstring wideName = std::wstring( GetFullFileName().begin(), GetFullFileName().end() );

    ScopedObject<IWICBitmapDecoder> decoder;
    HRESULT hr = pWIC->CreateDecoderFromFilename( wideName.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder );
    if ( FAILED( hr ) ) {
        return hr;
	}

    ScopedObject<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, &frame );
    if ( FAILED( hr ) ) {
        return hr;
	}

    hr = CreateTextureFromWIC( frame.Get(), nullptr, &m_pGPUTexture, 0, GetFullName(), m_pCPUTexture, m_TextureWidth, m_TextureHeight );
    if ( FAILED( hr ) ) { 
        return hr;
	}

	if ( m_bIsCPUTexture == false ) {
		m_pCPUTexture.reset();
	}

	return true;
}

/**
 *	kbTexture::GetCPUTexture
 */
const uint8_t * kbTexture::GetCPUTexture( unsigned int & width, unsigned int & height ) {

	if ( m_bIsCPUTexture == false ) {

		m_bIsCPUTexture = true;
		Release();
		Load_Internal();
	}

	width = m_TextureWidth;
	height = m_TextureHeight;

	return m_pCPUTexture.get();
}

/**
 *	kbTexture::Release_Internal
 */
void kbTexture::Release_Internal() {
	SAFE_RELEASE( m_pGPUTexture );
}

/**
 *	kbShader::kbShader
 */
kbShader::kbShader() :
	m_pVertexShader( nullptr ),
	m_pPixelShader( nullptr ),
    m_pGeometryShader( nullptr ),
	m_pVertexLayout( nullptr ),
	m_VertexShaderFunctionName( "vertexShader" ),
	m_PixelShaderFunctionName( "pixelShader" ),
	m_bBlendEnabled( false ),
	m_SrcBlend( Blend_One ),
	m_DstBlend( Blend_One ),
	m_BlendOp( BlendOp_Add ),
	m_SrcBlendAlpha( Blend_One ),
	m_DstBlendAlpha( Blend_One ),
	m_BlendOpAlpha( BlendOp_Add ),
	m_ColorWriteEnable( ColorWriteEnable_All ),
	m_CullMode( CullMode_BackFaces ) {
}

/**
 *	kbShader::kbShader
 */
kbShader::kbShader( const std::string & fileName ) :
	m_pVertexShader( nullptr ),
    m_pGeometryShader( nullptr ),
	m_pPixelShader( nullptr ),
	m_pVertexLayout( nullptr ),
	m_VertexShaderFunctionName( "vertexShader" ),
	m_PixelShaderFunctionName( "pixelShader" ),
	m_bBlendEnabled( false ),
	m_SrcBlend( Blend_One ),
	m_DstBlend( Blend_One ),
	m_BlendOp( BlendOp_Add ),
	m_SrcBlendAlpha( Blend_One ),
	m_DstBlendAlpha( Blend_One ),
	m_BlendOpAlpha( BlendOp_Add ),
	m_ColorWriteEnable( ColorWriteEnable_All ),
	m_CullMode( CullMode_BackFaces ) {

	m_FullFileName = fileName;
}

std::unordered_map<std::string, kbColorWriteEnable> g_ColorWriteMap;
kbColorWriteEnable GetColorWriteEnableFromName( const std::string & name ) {

	if ( g_ColorWriteMap.empty() ) {
		typedef std::pair<std::string, kbColorWriteEnable> colorWriteMapPair;

		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_r", ColorWriteEnable_Red ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_rg", ColorWriteEnable_Red | ColorWriteEnable_Green ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_rgb", ColorWriteEnable_Red | ColorWriteEnable_Green | ColorWriteEnable_Blue ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_rgba", ColorWriteEnable_All ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_rb", ColorWriteEnable_Red | ColorWriteEnable_Blue ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_rba", ColorWriteEnable_Red | ColorWriteEnable_Blue | ColorWriteEnable_Alpha ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_ra", ColorWriteEnable_Red | ColorWriteEnable_Alpha) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_g", ColorWriteEnable_Green ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_gb", ColorWriteEnable_Green | ColorWriteEnable_Blue ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_gba", ColorWriteEnable_Green | ColorWriteEnable_Blue | ColorWriteEnable_Alpha ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_ga", ColorWriteEnable_Green | ColorWriteEnable_Alpha ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_b", ColorWriteEnable_Blue ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_ba", ColorWriteEnable_Blue | ColorWriteEnable_Alpha ) );
		g_ColorWriteMap.insert( colorWriteMapPair( "colorwriteenable_a", ColorWriteEnable_Alpha ) );
	}

	auto colorMapIt = g_ColorWriteMap.find( name );
	if ( colorMapIt != g_ColorWriteMap.end() ) {
		return colorMapIt->second;
	}

	kbWarning( "GetColorWriteEnableFromName() - Invalid value %s", name.c_str() );
	return ColorWriteEnable_All;
}

std::unordered_map<std::string, kbBlend> g_BlendMap;
kbBlend GetBlendFromName( const std::string & name ) {

	if ( g_BlendMap.empty() ) {
		typedef std::pair<std::string, kbBlend> blendMapPair;	
		g_BlendMap.insert( blendMapPair( "blend_zero", Blend_Zero ) );
		g_BlendMap.insert( blendMapPair( "blend_one", Blend_One ) );
		g_BlendMap.insert( blendMapPair( "blend_srccolor", Blend_SrcColor ) );
		g_BlendMap.insert( blendMapPair( "blend_invsrccolor", Blend_InvSrcColor ) );
		g_BlendMap.insert( blendMapPair( "blend_srcalpha", Blend_SrcAlpha ) );
		g_BlendMap.insert( blendMapPair( "blend_invsrcalpha", Blend_InvSrcAlpha ) );
		g_BlendMap.insert( blendMapPair( "blend_dstalpha", Blend_DstAlpha ) );
		g_BlendMap.insert( blendMapPair( "blend_invdstalpha", Blend_InvDstAlpha ) );
		g_BlendMap.insert( blendMapPair( "blend_dstcolor", Blend_DstColor ) );
		g_BlendMap.insert( blendMapPair( "blend_invdstcolor", Blend_InvDstColor ) );
	}

	auto blendMapIt = g_BlendMap.find( name );
	if ( blendMapIt != g_BlendMap.end() ) {
		return blendMapIt->second;
	}

	kbWarning( "GetBlendFromName() - Invalid value %s", name.c_str() );
	return Blend_One;
}

std::unordered_map<std::string, kbBlendOp> g_BlendOpMap;
kbBlendOp GetBlendOpFromName( std::string & name ) {

	if ( g_BlendOpMap.empty() ) {
		typedef std::pair<std::string, kbBlendOp> blendOpMapPair;
		g_BlendOpMap.insert( blendOpMapPair( "blendop_add", BlendOp_Add ) );
		g_BlendOpMap.insert( blendOpMapPair( "blendop_subtract", BlendOp_Subtract ) );
		g_BlendOpMap.insert( blendOpMapPair( "blendop_max", BlendOp_Max ) );
		g_BlendOpMap.insert( blendOpMapPair( "blendop_min", BlendOp_Min ) );
	}

	auto blendOpMapIt = g_BlendOpMap.find( name );
	if ( blendOpMapIt != g_BlendOpMap.end() ) {
		return blendOpMapIt->second;
	}

	kbWarning( "GetBlendOpFromName() - Invalid value %s", name.c_str() );
	return BlendOp_Add;
}

/**
 *	kbShader::Load_Internal
 */
bool kbShader::Load_Internal() {

	if ( g_pD3D11Renderer != nullptr ) {		// HACK TODO

		// Load File
		std::ifstream shaderFile;
		shaderFile.open( GetFullFileName().c_str(), std::fstream::in );
		if ( shaderFile.fail() ) {
			return false;
		}

		std::string shaderText( ( std::istreambuf_iterator<char>(shaderFile) ), std::istreambuf_iterator<char>() );
		shaderFile.close();

		kbTextParser shaderParser( shaderText );
		shaderParser.RemoveComments();

		if ( shaderParser.SetBlock( "kbShaderState" ) ) {
			shaderParser.MakeLowerCase();

			std::string value;

			if ( shaderParser.GetValueForKey( value, "srcblend" ) ) {
				m_SrcBlend = GetBlendFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "dstblend" ) ) {
				m_DstBlend = GetBlendFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "blendop" ) ) {
				m_BlendOp = GetBlendOpFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "srcblendalpha" ) ) {
				m_SrcBlendAlpha = GetBlendFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "dstblendalpha" ) ) {
				m_DstBlendAlpha = GetBlendFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "blendopalpha" ) ) {
				m_BlendOpAlpha = GetBlendOpFromName( value );
				m_bBlendEnabled = true;
			}

			if ( shaderParser.GetValueForKey( value, "colorwriteenable" ) ) {
				m_ColorWriteEnable = GetColorWriteEnableFromName( value );
			}

			if ( shaderParser.GetValueForKey( value, "cullmode" ) ) {
				if ( value == "cullmode_none" ) {
					m_CullMode = CullMode_None;
				} else if ( value == "cullmode_frontfaces" ) {
					m_CullMode = CullMode_FrontFaces;
				} else if ( value == " cullmode_backfaces" ) {
					m_CullMode = CullMode_BackFaces;
				}
			}
			shaderParser.ReplaceBlockWithSpaces();
		}

		g_pD3D11Renderer->CreateShaderFromText( GetFullFileName(), shaderText, m_pVertexShader, m_pGeometryShader, m_pPixelShader, m_pVertexLayout, m_VertexShaderFunctionName.c_str(), m_PixelShaderFunctionName.c_str(), &m_ShaderVarBindings );
	}
	return true;
}

/**
 *	kbShader::Release_Internal
 */
void kbShader::Release_Internal() {
	SAFE_RELEASE( m_pVertexShader );
    SAFE_RELEASE( m_pGeometryShader );
	SAFE_RELEASE( m_pPixelShader );
	SAFE_RELEASE( m_pVertexLayout );

	m_ShaderVarBindings.m_VarBindings.clear();
	m_ShaderVarBindings.m_TextureNames.clear();

	m_bBlendEnabled = false;
	m_SrcBlend = Blend_One;
	m_DstBlend = Blend_One;
	m_BlendOp = BlendOp_Add;
	m_SrcBlendAlpha = Blend_One;
	m_DstBlendAlpha = Blend_One;
	m_BlendOpAlpha = BlendOp_Add;
	m_ColorWriteEnable = ColorWriteEnable_All;
	m_CullMode = CullMode_BackFaces;
}

/**
 *	kbShader::CommitShaderParams
 */
void kbShader::CommitShaderParams() {
	kbErrorCheck( g_pRenderer->IsRenderingSynced(), "kbShader::CommitShaderParams() - Can only be called when rendering is synced" );

	m_GlobalShaderParams_RenderThread = m_GlobalShaderParams_GameThread;
}
