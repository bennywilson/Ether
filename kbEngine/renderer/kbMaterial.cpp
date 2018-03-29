//===================================================================================================
// kbMaterial.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <Wincodec.h>
#include <memory>
#include "kbCore.h"
#include "kbRenderer_defs.h"
#include "kbMaterial.h"

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

    bool IsNull() const { return (!_pointer); }

    T& operator*() { return *_pointer; }
    T* operator->() { return _pointer; }
    T** operator&() { return &_pointer; }

    void Reset(T *const p = 0) { if ( _pointer ) { _pointer->Release(); } _pointer = p; }

    T* Get() const { return _pointer; }

private:
    ScopedObject(const ScopedObject&);
    ScopedObject& operator=(const ScopedObject&);
        
    T * _pointer;
};

struct WICTranslate {
    GUID                wic;
    DXGI_FORMAT         format;
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

struct WICConvert
{
    GUID        source;
    GUID        target;
};


static WICConvert g_WICConvert[] = 
{
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


static HRESULT CreateTextureFromWIC( ID3D11Device *const d3dDevice,
                                     ID3D11DeviceContext *const d3dContext,
                                     IWICBitmapFrameDecode *const frame,
                                     ID3D11Resource ** texture,
                                     ID3D11ShaderResourceView ** textureView,
                                     size_t maxsize,
									 const kbString & fileName ) {
    UINT width, height;
    HRESULT hr = frame->GetSize( &width, &height );
    if ( FAILED(hr) ) {
        return hr;
	}

    assert( width > 0 && height > 0 );

    if ( maxsize == 0 ) {
		maxsize = 8192;
    }

    UINT twidth, theight;
    if ( width > maxsize || height > maxsize ) {
        const float aspectRatio = static_cast<float>(height) / static_cast<float>(width);
        if ( width > height ) {
            twidth = static_cast<UINT>( maxsize );
            theight = static_cast<UINT>( static_cast<float>(maxsize) * aspectRatio );
        }
        else {
            theight = static_cast<UINT>( maxsize );
            twidth = static_cast<UINT>( static_cast<float>(maxsize) / aspectRatio );
        }
        assert( twidth <= maxsize && theight <= maxsize );
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
                assert( format != DXGI_FORMAT_UNKNOWN );
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

    if ( !bpp ) {
        return E_FAIL;
	}

    // Verify our target format is supported by the current device
    // (handles WDDM 1.0 or WDDM 1.1 device driver cases as well as DirectX 11.0 Runtime without 16bpp format support)
    UINT support = 0;
    hr = d3dDevice->CheckFormatSupport( format, &support );
    if ( FAILED(hr) || !(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) ) {
        // Fallback to RGBA 32-bit format which is supported by all devices
        memcpy( &convertGUID, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID) );
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bpp = 32;
    }

    // Allocate temporary memory for image
    size_t rowPitch = ( twidth * bpp + 7 ) / 8;
    size_t imageSize = rowPitch * theight;

    std::unique_ptr<uint8_t[]> temp( new uint8_t[ imageSize ] );

    // Load image data
    if ( memcmp( &convertGUID, &pixelFormat, sizeof(GUID) ) == 0 && twidth == width && theight == height ) {
        // No format conversion or resize needed
        hr = frame->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
        if ( FAILED(hr) ) {
            return hr;
		}
    }
    else if ( twidth != width || theight != height ) {
        // Resize
        IWICImagingFactory* pWIC = _GetWIC();
        if ( !pWIC ) {
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
            if ( FAILED(hr) )
                return hr;
        }
        else {
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
        IWICImagingFactory* pWIC = _GetWIC();
        if ( !pWIC ) {
            return E_NOINTERFACE;
		}

        IWICFormatConverter * FC;
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
    if ( d3dContext != 0 && textureView != 0 ) // Must have context and shader-view to auto generate mipmaps
    {
        UINT fmtSupport = 0;
        hr = d3dDevice->CheckFormatSupport( format, &fmtSupport );
        if ( SUCCEEDED(hr) && ( fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) )
        {
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
    hr = d3dDevice->CreateTexture2D( &desc, (autogen) ? nullptr : &initData, &tex );
    if ( SUCCEEDED(hr) && tex != 0 )
    {
        if (textureView != 0)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            memset( &SRVDesc, 0, sizeof( SRVDesc ) );
            SRVDesc.Format = format;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = (autogen) ? -1 : 1;

            hr = d3dDevice->CreateShaderResourceView( tex, &SRVDesc, textureView );
            if ( FAILED(hr) )
            {
                tex->Release();
                return hr;
            }

            if ( autogen )
            {
                assert( d3dContext != 0 );
                d3dContext->UpdateSubresource( tex, 0, nullptr, temp.get(), static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize) );
                d3dContext->GenerateMips( *textureView );
            }
        }

        if (texture != 0)
        {
            *texture = tex;
        }
        else
        {
#if defined(_DEBUG) || defined(PROFILE)
            tex->SetPrivateData( WKPDID_D3DDebugObjectName,
                                 sizeof("WICTextureLoader")-1,
                                 "WICTextureLoader"
                               );
#endif
            tex->Release();
        }
    }

    return hr;
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

kbLog( "Trying to load %s -----------------------------------------", GetFullFileName().c_str());
    // Initialize WIC
	std::wstring wideName = std::wstring( GetFullFileName().begin(), GetFullFileName().end() );

    ScopedObject<IWICBitmapDecoder> decoder;
    HRESULT hr = pWIC->CreateDecoderFromFilename( wideName.c_str(), 0, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder );
    if ( FAILED( hr ) ) {
        return hr;
	}

    ScopedObject<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, &frame );
    if ( FAILED( hr ) ) {
        return hr;
	}

	extern ID3D11DeviceContext * g_pImmediateContext;
    hr = CreateTextureFromWIC( g_pD3DDevice, g_pImmediateContext, frame.Get(), nullptr, &m_pTexture, 0, GetFullName() );
    if ( FAILED( hr ) ) { 
        return hr;
	}

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
