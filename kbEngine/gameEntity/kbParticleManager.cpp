//===================================================================================================
// kbParticleManager.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbParticleManager.h"
#include "kbRenderer.h"

static const uint NumParticleBufferVerts = 10000;
static const uint NumCustomAtlases = 16;
static const uint ComponentPoolSize = 100;

/**
 *	kbParticleManager::kbParticleManager
 */
kbParticleManager::kbParticleManager() {

	m_ComponentPool.resize( ComponentPoolSize );
	for ( int i = 0; i < ComponentPoolSize; i++ ) {
		m_ComponentPool[i] = new kbGameComponent();
	}
}

/**
 *	kbParticleManager::~kbParticleManager
 */
kbParticleManager::~kbParticleManager() {

	for ( int i = 0; i < m_CustomAtlases.size(); i++ ) {
		CustomAtlasParticles_t & curAtlas = m_CustomAtlases[i];
		for ( int iBuffer = 0; iBuffer < NumCustomParticleBuffers; iBuffer++ ) {
			curAtlas.m_RenderModel[iBuffer].Release();
			delete curAtlas.m_RenderObject.m_pComponent;
			curAtlas.m_RenderObject.m_pComponent = nullptr;
		}
	}

	for ( std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>::iterator it = m_ParticlePools.begin(); it != m_ParticlePools.end(); ++it ) {
		std::vector< kbParticleComponent *> & particleList = it->second;
		for ( int i = 0; i < particleList.size(); i++ ) {
			delete particleList[i];
		}
	}

	for ( int i = 0; i < m_ComponentPool.size(); i++ ) {
		delete m_ComponentPool[i];
	}
}

/**
 *	kbParticleManager::SetCustomAtlasTexture
 */
void kbParticleManager::SetCustomAtlasTexture( const uint atlasIdx, const std::string & atlasFileName ) {
	kbErrorCheck( atlasIdx < m_CustomAtlases.size(), "kbParticleManager::SetCustomAtlasTexture() - Atlas index %d is out of range", atlasIdx );
	kbErrorCheck( g_pRenderer->IsRenderingSynced(), "kbParticleManager::SetCustomAtlasTexture() - Rendering not synced" );

	CustomAtlasParticles_t & curAtlas = m_CustomAtlases[atlasIdx];
	curAtlas.m_pAtlasTexture = (kbTexture *)g_ResourceManager.LoadResource( atlasFileName.c_str(), true );

	kbWarningCheck( curAtlas.m_pAtlasTexture  != nullptr, "kbParticleManager::SetCustomAtlasTexture() - Unable to find shader %s", atlasFileName.c_str() );

	UpdateAtlas( curAtlas );
}

/**
 *	kbParticleManager::SetCustomAtlasShader
 */
void kbParticleManager::SetCustomAtlasShader( const uint atlasIdx, const std::string & shaderFileName ) {
	kbErrorCheck( atlasIdx < m_CustomAtlases.size(), "kbParticleManager::SetCustomAtlasShader() - Atlas index %d is out of range", atlasIdx );
	kbErrorCheck( g_pRenderer->IsRenderingSynced(), "kbParticleManager::SetCustomAtlasShader() - Rendering not synced" );

	CustomAtlasParticles_t & curAtlas = m_CustomAtlases[atlasIdx];
	curAtlas.m_pAtlasShader = (kbShader *)g_ResourceManager.LoadResource( shaderFileName.c_str(), true );

	kbWarningCheck( curAtlas.m_pAtlasShader != nullptr, "kbParticleManager::SetCustomAtlasShader() - Unable to find shader %s", shaderFileName.c_str() );
	
	UpdateAtlas( curAtlas );
}

/**
 *	kbParticleManager::PoolParticleComponent
 */
void kbParticleManager::PoolParticleComponent( const kbParticleComponent *const pParticleTemplate, const int PoolSize ) {
	if ( pParticleTemplate == nullptr ) {
		kbWarning( "kbParticleManager::PoolParticleComponent() - NULL particle passed in" );
		return;
	}

	if ( PoolSize < 1 ) {
		kbWarning( "kbParticleManager::PoolParticleComponent() - Invalid pool size of %d specified", PoolSize );
		return;
	}

	// Check if this pool already exists
	std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>::iterator it = m_ParticlePools.find( pParticleTemplate );
	if ( it != m_ParticlePools.end() ) {
		kbWarning( "kbParticleManager::PoolParticleComponent() - Particle %s already pooled", pParticleTemplate->GetOwner()->GetName().c_str() );
		return;
	}

	// Create a new pool
	m_ParticlePools[pParticleTemplate] = std::vector< kbParticleComponent *>();
	std::vector< kbParticleComponent *> & ParticlePool = m_ParticlePools[pParticleTemplate];

	ParticlePool.reserve( PoolSize );
	for ( int i = 0; i < PoolSize; i++ ) {
		kbParticleComponent *const pNewParticle = (kbParticleComponent*)pParticleTemplate->Duplicate();
		pNewParticle->Enable( false );
		pNewParticle->m_ParticleTemplate = pParticleTemplate;
		pNewParticle->m_bIsPooled = true;
		ParticlePool.push_back( pNewParticle );
	}
}

/**
 *	kbParticleManager::GetParticleComponent
 */
kbParticleComponent * kbParticleManager::GetParticleComponent( const kbParticleComponent *const pParticleTemplate ) {

	if ( pParticleTemplate == nullptr ) {
		kbWarning( "kbParticleManager::GetParticleComponent() - NULL particle passed in" );
		return nullptr;
	}

	std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>::iterator it = m_ParticlePools.find( pParticleTemplate );
	if ( it == m_ParticlePools.end() ) {
		kbWarning( "kbParticleManager::GetParticleComponent() - Particle %s not found in pooled", pParticleTemplate->GetOwner()->GetName().c_str() );
		return nullptr;
	}

	kbParticleComponent *const retParticle = it->second[it->second.size() - 1];
	it->second.pop_back();

	return retParticle;
}

/**
 *	kbParticleManager::ReturnParticleComponent
 */
void kbParticleManager::ReturnParticleComponent( kbParticleComponent *const pParticle ) {

	if ( pParticle == nullptr ) {
		kbError( "kbParticleManager::ReturnParticleComponent() - NULL particle passed in" );
		return;
	}

	g_pRenderer->RemoveParticle( pParticle->m_RenderObject );
	pParticle->GetOwner()->RemoveComponent( pParticle );
	const kbParticleComponent *const pParticleTemplate = pParticle->m_ParticleTemplate;
	if ( pParticleTemplate == nullptr || pParticle->m_bIsPooled == false ) {
		kbError( "kbParticleManager::ReturnParticleComponent() - Particle does not appear to be pooled" );
		return;
	}

	pParticle->Enable( false );

	std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>::iterator it = m_ParticlePools.find( pParticleTemplate );
	if ( it == m_ParticlePools.end() ) {
		kbError( "kbParticleManager::ReturnParticleComponent() - Particle %s not found in pooled", pParticleTemplate->GetOwner()->GetName().c_str() );
		return;
	}
	
	it->second.push_back( pParticle );
}

/**
 *	kbParticleManager::UpdateAtlas
 */
void kbParticleManager::UpdateAtlas( CustomAtlasParticles_t & atlasInfo ) {

	kbErrorCheck( g_pRenderer->IsRenderingSynced(), "kbParticleManager::UpdateAtlas() - Rendering isn't sync'd" );

	if ( atlasInfo.m_iCurParticleModel >= 0 ) {
		g_pRenderer->RemoveParticle( atlasInfo.m_RenderObject );
	}

	for ( uint iModel = 0; iModel < NumCustomParticleBuffers; iModel++ ) {
		kbModel & renderModel = atlasInfo.m_RenderModel[iModel];
		if ( renderModel.IsVertexBufferMapped() ) {
			renderModel.UnmapVertexBuffer();
			renderModel.UnmapIndexBuffer();
		}
		renderModel.Release();

		if ( atlasInfo.m_pAtlasTexture == nullptr ) {
			atlasInfo.m_pAtlasTexture = (kbTexture*)g_ResourceManager.LoadResource( "./assets/FX/laser_beam.jpg", true );
		}

		if ( atlasInfo.m_pAtlasShader == nullptr ) {
			atlasInfo.m_pAtlasShader = (kbShader*)g_ResourceManager.LoadResource( "../../kbEngine/assets/Shaders/basicParticle.kbShader", true );
		}

		renderModel.CreateDynamicModel( NumParticleBufferVerts, NumParticleBufferVerts, atlasInfo.m_pAtlasShader, atlasInfo.m_pAtlasTexture, sizeof(kbParticleVertex) );

		// Update materials
		atlasInfo.m_RenderObject.m_Materials.clear();
		kbShaderParamOverrides_t atlasMaterial;
		atlasMaterial.m_pShader = atlasInfo.m_pAtlasShader;
		atlasMaterial.SetTexture( "shaderTexture", atlasInfo.m_pAtlasTexture );
		atlasInfo.m_RenderObject.m_Materials.push_back( atlasMaterial );

		atlasInfo.m_pVertexBuffer = (kbParticleVertex*)atlasInfo.m_RenderModel[iModel].MapVertexBuffer();
		for ( int iVert = 0; iVert < NumParticleBufferVerts; iVert++ ) {
			atlasInfo.m_pVertexBuffer[iVert].position.Set( 0.0f, 0.0f, 0.0f );
		}
		renderModel.UnmapVertexBuffer();
		atlasInfo.m_pVertexBuffer = nullptr;
	}
	atlasInfo.m_iCurParticleModel = -1;
}

/**
 *	kbParticleManager::RenderSync
 */
void kbParticleManager::RenderSync() {

	if ( m_CustomAtlases.size() == 0 ) {
		m_CustomAtlases.resize( NumCustomAtlases );
		for ( int iAtlas = 0; iAtlas < NumCustomAtlases; iAtlas++ ) {
			UpdateAtlas( m_CustomAtlases[iAtlas] );
		}
	}

	// Map/unmap buffers and pass it to the renderer
	for ( int iAtlas = 0; iAtlas < NumCustomAtlases; iAtlas++ ) {
		CustomAtlasParticles_t & curAtlas = m_CustomAtlases[iAtlas];
		kbRenderObject & curRenderObj = curAtlas.m_RenderObject;
		if ( curRenderObj.m_pComponent == nullptr ) {
			curRenderObj.m_pComponent = new kbTransformComponent();
		}

		kbModel & finishedModel = (curAtlas.m_iCurParticleModel >= 0 ) ? ( curAtlas.m_RenderModel[curAtlas.m_iCurParticleModel] ) : ( curAtlas.m_RenderModel[0] );
		if ( curAtlas.m_iCurParticleModel >= 0 ) {
			g_pRenderer->RemoveParticle( curRenderObj );
			finishedModel.UnmapVertexBuffer( curAtlas.m_NumIndices );
			finishedModel.UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
		} else {
			curAtlas.m_iCurParticleModel = 0;
		}

		finishedModel.SwapTexture( 0, curAtlas.m_pAtlasTexture, 0 );

		curRenderObj.m_Position = kbVec3::zero;
		curRenderObj.m_Orientation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );
		curRenderObj.m_pModel = &finishedModel;
		g_pRenderer->AddParticle( curRenderObj );

		curAtlas.m_iCurParticleModel = ( curAtlas.m_iCurParticleModel + 1 ) % NumCustomParticleBuffers;

		kbModel & nextModel = curAtlas.m_RenderModel[curAtlas.m_iCurParticleModel];
		curAtlas.m_pVertexBuffer = (kbParticleVertex*)nextModel.MapVertexBuffer();
		curAtlas.m_pIndexBuffer = (ushort *)nextModel.MapIndexBuffer();
		curAtlas.m_NumIndices = 0;
	}
}

/**
 *	kbParticleManager::AddQuad
 */
void kbParticleManager::AddQuad( const uint atlasIdx, const CustomParticleAtlasInfo_t & CustomParticleInfo ) {
	
	kbErrorCheck( atlasIdx < m_CustomAtlases.size(), "kbParticleManager::AddQuad() - Invalid atlasIdx %d", atlasIdx );
	CustomAtlasParticles_t & curAtlas = m_CustomAtlases[atlasIdx];

	if ( curAtlas.m_pVertexBuffer == nullptr || curAtlas.m_pIndexBuffer == nullptr ) {
		return;
	}

	const int vertexIndex = curAtlas.m_NumIndices - ( curAtlas.m_NumIndices / 3 );
	curAtlas.m_pVertexBuffer[vertexIndex + 0].position = CustomParticleInfo.m_Position;
	curAtlas.m_pVertexBuffer[vertexIndex + 1].position = CustomParticleInfo.m_Position;
	curAtlas.m_pVertexBuffer[vertexIndex + 2].position = CustomParticleInfo.m_Position;
	curAtlas.m_pVertexBuffer[vertexIndex + 3].position = CustomParticleInfo.m_Position;
	
	curAtlas.m_pVertexBuffer[vertexIndex + 0].uv.Set( CustomParticleInfo.m_UVs[0].x, CustomParticleInfo.m_UVs[0].y );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].uv.Set( CustomParticleInfo.m_UVs[1].x, CustomParticleInfo.m_UVs[0].y );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].uv.Set( CustomParticleInfo.m_UVs[1].x, CustomParticleInfo.m_UVs[1].y );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].uv.Set( CustomParticleInfo.m_UVs[0].x, CustomParticleInfo.m_UVs[1].y );

	kbVec4 color = CustomParticleInfo.m_Color;
	curAtlas.m_pVertexBuffer[vertexIndex + 0].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].SetColor( color );

	curAtlas.m_pVertexBuffer[vertexIndex + 0].billboardType[0] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].billboardType[0] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].billboardType[0] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].billboardType[0] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );

	const float halfWidth = CustomParticleInfo.m_Width * 0.5f;
	const float halfHeight = CustomParticleInfo.m_Height * 0.5f;

	curAtlas.m_pVertexBuffer[vertexIndex + 0].size = kbVec2( -halfWidth,  halfHeight );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].size = kbVec2(  halfWidth,  halfHeight );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].size = kbVec2(  halfWidth, -halfHeight );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].size = kbVec2( -halfWidth, -halfHeight );

	curAtlas.m_pVertexBuffer[vertexIndex + 0].direction = CustomParticleInfo.m_Direction;
	curAtlas.m_pVertexBuffer[vertexIndex + 1].direction = CustomParticleInfo.m_Direction;
	curAtlas.m_pVertexBuffer[vertexIndex + 2].direction = CustomParticleInfo.m_Direction;
	curAtlas.m_pVertexBuffer[vertexIndex + 3].direction = CustomParticleInfo.m_Direction;

	curAtlas.m_pVertexBuffer[vertexIndex + 0].rotation = CustomParticleInfo.m_Rotation;
	curAtlas.m_pVertexBuffer[vertexIndex + 1].rotation = CustomParticleInfo.m_Rotation;
	curAtlas.m_pVertexBuffer[vertexIndex + 2].rotation = CustomParticleInfo.m_Rotation;
	curAtlas.m_pVertexBuffer[vertexIndex + 3].rotation = CustomParticleInfo.m_Rotation;

	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 0] = vertexIndex + 2;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 1] = vertexIndex + 1;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 2] = vertexIndex + 0;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 3] = vertexIndex + 3;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 4] = vertexIndex + 2;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 5] = vertexIndex + 0;
	curAtlas.m_NumIndices += 6;
}

/**
 *	
 */
kbGameComponent * kbParticleManager::GetComponentFromPool() {
	if ( m_ComponentPool.size() == 0 ) {
		kbWarning( "kbParticleManager::GetComponentFromPool() - Component pool is empty" );
		return nullptr;
	}

	kbGameComponent *const pGameComponent = m_ComponentPool[m_ComponentPool.size() - 1];
	m_ComponentPool.pop_back();
	return pGameComponent;
}

/**
 *	
 */
void kbParticleManager::ReturnComponentToPool( kbGameComponent *const pGameComponent ) {
	if ( pGameComponent == nullptr ) {
		kbWarning( "kbParticleManager::ReturnComponentToPool() - null component passed in" );
		return;
	}

	m_ComponentPool.push_back( pGameComponent );
}
