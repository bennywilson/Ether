/// kbRenderer.h
///
/// 2018-2025 blk 1.0

#pragma once

#include "render_defs.h"
#include "kbRenderer_defs.h"
#include "kbBounds.h"

class kbShader;
class kbModel;

/// kbRenderTexture
enum eTextureFormat {
	KBTEXTURE_NULLFORMAT,
	KBTEXTURE_R8G8B8A8,
	KBTEXTURE_R16G16B16A16,
	KBTEXTURE_R32G32B32A32,
	KBTEXTURE_R32G32,
	KBTEXTURE_R32,
	KBTEXTURE_D24S8,
	KBTEXTURE_R16G16,
	NUM_TEXTURE_FORMATS,
};

enum eReservedRenderTargets {
	COLOR_BUFFER,		// Color in xyz.  Pixel Depth in W
	NORMAL_BUFFER,		// Normal in xyz. W currently unused
	SPECULAR_BUFFER,
	DEPTH_BUFFER,
	ACCUMULATION_BUFFER_1,
	ACCUMULATION_BUFFER_2,
	SHADOW_BUFFER,
	SHADOW_BUFFER_DEPTH,
	RGBA_BUFFER,
	DOWN_RES_BUFFER,
	DOWN_RES_BUFFER_2,
	BLOOM_BUFFER_1,
	BLOOM_BUFFER_2,
	SCRATCH_BUFFER,
	MOUSE_PICKER_BUFFER,
	MAX_HALF_BUFFER,
	NUM_RESERVED_RENDER_TARGETS,
};

class kbRenderTexture {

	friend class kbRenderer;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderTexture() :
													m_Width( 0 ),
													m_Height( 0 ),
													m_TextureFormat( KBTEXTURE_NULLFORMAT ),
													m_bInUse( false ),
													m_bCPUAccessible( false ) { }
   
												kbRenderTexture( const int width, const int height, const eTextureFormat targetFormat, const bool bIsCPUAccessible ) :
													m_Width( width ),
													m_Height( height ),
													m_TextureFormat( targetFormat ),
													m_bCPUAccessible( bIsCPUAccessible ) { }

												void Release() {
													Release_Internal();
												}

	int											GetWidth() const { return m_Width; }
	int											GetHeight() const { return m_Height; }
	eTextureFormat								GetTextureFormat() const { return m_TextureFormat; }
	bool										IsCPUAccessible() const { return m_bCPUAccessible; }

private:

	virtual void								Release_Internal() = 0;

	int											m_Width;
	int											m_Height;

	eTextureFormat								m_TextureFormat;

	bool										m_bCPUAccessible;
	bool										m_bInUse;
};

/// kbRenderSubmesh
class kbRenderSubmesh {

	friend class kbRenderer;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderSubmesh( const kbRenderObject* const pInMesh, const int inMeshIdx, const ERenderPass renderPass, const float distFromCamera ) :
													m_pRenderObject( pInMesh ),
													m_MeshIdx( inMeshIdx ),
													m_render_pass( renderPass ),
													m_DistFromCamera( distFromCamera ) { }

	const kbRenderObject *						GetRenderObject() const { return m_pRenderObject; }
	int											GetMeshIdx() const { return m_MeshIdx; }
	ERenderPass									render_pass() const { return m_render_pass; }
	const kbShader *							get_shader() const;
	float										GetDistFromCamera() const { return m_DistFromCamera; }

private:
	const kbRenderObject *						m_pRenderObject;
	int											m_MeshIdx;
	float										m_DistFromCamera;
	ERenderPass									m_render_pass;
};

/// kbRenderWindow
class kbRenderWindow {

	friend class kbRenderer;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

																kbRenderWindow( HWND inHwnd, const RECT & rect, const float nearPlane, const float farPlane );
	virtual														~kbRenderWindow();

	const HWND													GetHWND() const { return m_Hwnd; }

	void														Release();
	
	virtual void												BeginFrame();
	virtual void												EndFrame();

	float														GetNearPlane() const { return m_NearPlane_RenderThread; }
	float														GetFarPlane() const { return m_FarPlane_RenderThread; }

	uint														GetViewPixelWidth() const { return m_ViewPixelWidth; }
	uint														GetViewPixelHeight() const { return m_ViewPixelHeight; }
	float														GetFViewPixelWidth() const { return m_fViewPixelWidth; }
	float														GetFViewPixelHeight() const { return m_fViewPixelHeight; }
	float														GetFViewPixelHalfWidth() const { return m_fViewPixelHalfWidth; }
	float														GetFViewPixelHalfHeight() const { return m_fViewPixelHalfHeight; }

	const Mat4 &												GetViewMatrix() const { return m_ViewMatrix; }
	const Mat4 &												GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const Mat4 &												GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	const Mat4 &												GetInverseProjectionMatrix() const { return m_InverseProjectionMatrix; }
	const Mat4 &												GetInverseViewProjection() const { return m_InverseViewProjectionMatrix; }
	const Vec3 &												GetCameraPosition() const { return m_CameraPosition; }
	const Quat4 &												GetCameraRotation() const { return m_CameraRotation; }

	const std::map<const kbGameComponent *, kbRenderObject *> &	GetRenderObjectMap() const { return m_render_objectMap; }
	const std::map<const kbLightComponent *, kbRenderLight *> &	GetRenderLightMap() const { return m_RenderLightMap; }
	const std::map<const void *, kbRenderObject *> &			GetRenderParticleMap() const { return m_RenderParticleMap; }

	std::vector<kbRenderSubmesh> &								GetVisibleSubMeshes( const int renderPass ) { return m_VisibleRenderMeshes[renderPass]; }
 
	void														HackSetViewMatrix( const Mat4 & inViewMat ) { m_ViewMatrix = inViewMat; }
	void														HackSetViewProjectionMatrix( const Mat4 & inViewProjMat ) { m_ViewProjectionMatrix = inViewProjMat; }
	void														HackSetProjectionMatrix( const Mat4 & inProjMat ) { m_ProjectionMatrix = inProjMat; }
	void														HackSetInverseProjectionMatrix( const Mat4 & invProjMat ) { m_InverseProjectionMatrix = invProjMat; }
	void														HackSetInverseViewProjectionMatrix( const Mat4 & invViewProj ) { m_InverseViewProjectionMatrix = invViewProj; }


private:

	virtual void												BeginFrame_Internal() = 0;
	virtual void												EndFrame_Internal() = 0;
	virtual void												Release_Internal() = 0;

	HWND														m_Hwnd;

	unsigned int												m_ViewPixelWidth;
	unsigned int												m_ViewPixelHeight;
	float														m_fViewPixelWidth;
	float														m_fViewPixelHeight;
	float														m_fViewPixelHalfWidth;
	float														m_fViewPixelHalfHeight;

	Mat4														m_ViewMatrix;
	Mat4														m_ProjectionMatrix;
	Mat4														m_ViewProjectionMatrix;
	Mat4														m_InverseProjectionMatrix;
	Mat4														m_InverseViewProjectionMatrix;
	Vec3														m_CameraPosition;
	Quat4														m_CameraRotation;

	Vec3														m_CameraPosition_GameThread;
	Quat4														m_CameraRotation_GameThread;

	float														m_NearPlane_GameThread;
	float														m_NearPlane_RenderThread;
	float														m_FarPlane_GameThread;
	float														m_FarPlane_RenderThread;

	std::map<const kbGameComponent *, kbRenderObject *>			m_render_objectMap;
	std::map<const kbLightComponent *, kbRenderLight *>			m_RenderLightMap;
	std::map<const void *, kbRenderObject *>					m_RenderParticleMap;

	std::vector<kbRenderSubmesh>								m_VisibleRenderMeshes[NUM_RENDER_PASSES];
};

/**
 *	kbRenderHook
 *
 *	Provides an interface for game code to hook into the render thread at various points
 */
class kbRenderHook {

	friend kbRenderer;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderHook() : m_render_pass( RP_InWorldUI ) { } 
												kbRenderHook( const ERenderPass );
												~kbRenderHook();

	ERenderPass									render_pass() const { return m_render_pass; }
	virtual void								RenderHookCallBack( kbRenderTexture* const pSrc, kbRenderTexture* const pDst ) = 0;

protected:

	void										set_render_pass( const ERenderPass nextRenderPass ) { m_render_pass = nextRenderPass; };

private:

	virtual void								RenderSync() { }
	ERenderPass									m_render_pass;
};

/// kbRenderer
class kbRenderer {

	friend kbRenderJob;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbRenderer();
	virtual										~kbRenderer();

	virtual void								Init( HWND, const int width, const int height );

	void										Shutdown();

	void										SetWorldAndEditorIconScale( const float newWorldScale, const float editorIconScale ) { m_GlobalModelScale_GameThread = newWorldScale, m_EditorIconScale_GameThread = editorIconScale; }

	virtual void								LoadTexture( const char* name, int index, int width = -1, int height = -1 );

	virtual int									CreateRenderView( HWND hwnd ) = 0;
	virtual void								SetRenderWindow( HWND hwnd ) = 0;

	void										RegisterRenderHook( kbRenderHook* const pHook );
	void										UnregisterRenderHook( kbRenderHook* const pHook );

	// View Transform
	virtual void								SetRenderViewTransform( const HWND hwnd, const Vec3& position, const Quat4& rotation );
	virtual void								GetRenderViewTransform( const HWND hwnd, Vec3& position, Quat4& rotation );

	void										SetNearFarPlane( const HWND hwnd, const float near, const float far );

	void										AddRenderObject( const kbRenderObject& renderObjectToAdd );
	void										UpdateRenderObject( const kbRenderObject& renderObjectToUpdate );
	void										RemoveRenderObject( const kbRenderObject& renderObjectToRemove );
	
	// Lights
	void										AddLight( const kbLightComponent* const pLightComponent, const Vec3& pos, const Quat4& orientation );
	void										UpdateLight( const kbLightComponent* pLightComponent, const Vec3& pos, const Quat4& orientation );
	void										RemoveLight( const kbLightComponent* const pLightComponent );
	void										HackClearLight( const kbLightComponent* const pLightComponent );

	// Fog
	void										UpdateFog( const kbColor& color, const float startDistance, const float endDistance );

	// Particles
	void										AddParticle( const kbRenderObject& renderObject );
	void										RemoveParticle( const kbRenderObject& renderObject );

	// Light Shafts
	void										AddLightShafts( const kbLightShaftsComponent* const pComponent, const Vec3& pos, const Quat4& orientation );
	void										UpdateLightShafts( const kbLightShaftsComponent* const pComponent, const Vec3& pos, const Quat4& orientation );
	void										RemoveLightShafts( const kbLightShaftsComponent* const pComponent );

	virtual void								SetGlobalShaderParam( const kbShaderParamOverrides_t::kbShaderParam_t & shaderParam ) = 0;
	virtual void								SetGlobalShaderParam( const kbShaderParamOverrides_t& shaderParam ) = 0;

	// Debug Drawing
	virtual void								EnableDebugBillboards( const bool bEnable ) { m_bDebugBillboardsEnabled = bEnable; }
	bool										DebugBillboardsEnabled() const { return m_bDebugBillboardsEnabled; }

	void										EnableConsole( const bool bEnable ) { m_bConsoleEnabled = bEnable; }
	void										DrawDebugText( const std::string& theString, const float X, const float Y, const float ScreenCharWidth, 
															   const float ScreenCharHeight, const kbColor& color );

	virtual Vec2i								GetEntityIdAtScreenPosition( const uint x, const uint y ) = 0;

	// Other
	int											GetBackBufferWidth() const { return Back_Buffer_Width; }
	int											GetBackBufferHeight() const { return Back_Buffer_Height; }

	// Render Syncing
	void										RenderSync();
	void										WaitForRenderingToComplete() const { while ( m_RenderThreadSync == 1 ) { } };
	void										SetReadyToRender() { m_RenderThreadSync = 1; }
	bool										IsRenderingSynced() const { return m_RenderThreadSync == 0; }

	// Various Drawing commands
	void										DrawScreenSpaceQuad( const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader* const pShader = nullptr );
	void										DrawLine( const Vec3& start, const Vec3& end, const kbColor& color, const bool bDepthTest = true );
	void										DrawBox( const kbBounds& bounds, const kbColor& color, const bool bDepthTest = true );
	void										DrawPreTransformedLine( const std::vector<Vec3>& vertList, const kbColor& color );
	void										DrawSphere( const Vec3& origin, const float radius, const int NumSegments, const kbColor& color );
	void										DrawBillboard( const Vec3& position, const Vec2& size, const int textureIndex, kbShader* const pShader, const int entityId = -1 );
	void										DrawModel( const kbModel* const pModel, const std::vector<kbShaderParamOverrides_t>& materials, const Vec3& start, const Quat4& orientation, const Vec3& scale, const int entityId );

	//
	enum kbViewMode_t {
		ViewMode_Shaded,
		ViewMode_Wireframe,
		ViewMode_Color,
		ViewMode_Normals,
		ViewMode_Specular,
		ViewMode_Depth
	};
	void										SetViewMode( const kbViewMode_t newViewMode ) { m_ViewMode_GameThread = newViewMode; }


	kbViewMode_t								m_ViewMode_GameThread;
	kbViewMode_t								m_ViewMode;

	// Render thread functions
	virtual void								RT_SetRenderTarget( kbRenderTexture* const pRenderTexture ) = 0;
	virtual void								RT_ClearRenderTarget( kbRenderTexture* const pRenderTexture, const kbColor& color ) = 0;
	kbRenderTexture *							RT_RenderTexture( const int width, const int height, const eTextureFormat, const bool bRequiresCPUAccess );
	virtual void								RT_ReturnRenderTexture( kbRenderTexture* const pRenderTexture );
	virtual void								RT_RenderMesh( const kbModel* const pModel, kbShader * pShader, const kbShaderParamOverrides_t* const pShaderParams ) = 0;
	virtual void								RT_Render2DLine( const Vec3& startPt, const Vec3& endPt, const kbColor& color, const float width, const kbShader* const pShader, const struct kbShaderParamOverrides_t* const ShaderBindings = nullptr ) = 0;
	virtual void								RT_Render2DQuad( const Vec2& origin, const Vec2& size, const kbColor& color, const kbShader* const pShader, const struct kbShaderParamOverrides_t* const ShaderBindings = nullptr ) = 0;
	virtual void								RT_CopyRenderTarget( kbRenderTexture* const pSrcTexture, kbRenderTexture* const pDstTexture ) = 0;
	virtual kbRenderTargetMap					RT_MapRenderTarget( kbRenderTexture* const pDstTexture ) = 0;
	virtual void								RT_UnmapRenderTarget( kbRenderTexture* const pDstTexture ) = 0;
 
private:

	virtual void								Init_Internal( HWND, const int width, const int height ) = 0;
	virtual bool								LoadTexture_Internal( const char* const name, int index, int width = -1, int height = -1 ) = 0;
	virtual void								RenderSync_Internal() = 0;
	virtual void								Shutdown_Internal() = 0;
	
	virtual kbRenderTexture *					GetRenderTexture_Internal( const int width, const int height, const eTextureFormat texFormat, const bool bRequiresCPUAccess ) = 0;
	virtual void								ReturnRenderTexture_Internal( const kbRenderTexture* const ) = 0;

	virtual void								RenderScene() = 0;

protected:

	int											Back_Buffer_Width;
	int											Back_Buffer_Height;

	float										m_GlobalModelScale_GameThread;
	float										m_GlobalModelScale_RenderThread;

	float										m_EditorIconScale_GameThread;
	float										m_EditorIconScale_RenderThread;

	kbRenderWindow *							m_pCurrentRenderWindow;    // the render window BeginScene was called with
	std::vector<kbRenderWindow *>				m_RenderWindowList;

	std::vector<kbRenderTexture	*>				m_pRenderTargets;

	std::vector<kbRenderHook*>					m_RenderHooks[NUM_RENDER_PASSES];

	struct ScreenSpaceQuad_t {
												ScreenSpaceQuad_t() { }

		Vec2i									m_Pos;
		Vec2i									m_Size;
		int										m_TextureIndex;
		class kbShader *						m_shader;
	};
	std::vector<ScreenSpaceQuad_t>				m_ScreenSpaceQuads_GameThread;
	std::vector<ScreenSpaceQuad_t>				m_ScreenSpaceQuads_RenderThread;

	std::vector<kbRenderObject>					m_render_objectList_GameThread;
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

	kbRenderTexture *							m_pAccumBuffers[2];
	int											m_iAccumBuffer;

	// Text
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

	// Debug
	std::vector<vertexLayout>					m_DepthLines_GameThread;
	std::vector<vertexLayout>					m_DepthLines_RenderThread;
	std::vector<vertexLayout>					m_NoDepthLines_GameThread;
	std::vector<vertexLayout>					m_NoDepthLines_RenderThread;

	std::vector<vertexLayout>					m_DebugPreTransformedLines;

	struct debugDrawObject_t {
		Vec3									m_Position;
		Quat4									m_Orientation;
		Vec3									m_Scale;
		const kbModel *							m_model;
		std::vector<kbShaderParamOverrides_t>	m_Materials;
		kbShader *								m_shader;
		int										m_TextureIndex;
		int										m_EntityId;
	};
	std::vector<debugDrawObject_t>				m_DebugBillboards;
	std::vector<debugDrawObject_t>				m_DebugModels;
	std::vector<debugDrawObject_t>				m_DebugBillboards_GameThread;
	std::vector<debugDrawObject_t>				m_DebugModels_GameThread;

	// Threading
	kbRenderJob *								m_pRenderJob;
	volatile int								m_RenderThreadSync;

	bool										m_bConsoleEnabled;
	bool										m_bDebugBillboardsEnabled;
};

extern class kbRenderer * g_pRenderer;
extern const float g_DebugTextSize;
extern const float g_DebugLineSpacing;
