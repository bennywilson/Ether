//===================================================================================================
// kbParticleManager.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbParticleManager.h"
#include "kbRenderer.h"

static const uint NumParticleBufferVerts = 10000;
static const uint NumCustomAtlases = 16;
/**
 *	kbParticleManager::kbParticleManager
 */
kbParticleManager::kbParticleManager() :
	m_CurrentParticleBuffer( 255 ) {
	m_CustomAtlasParticles.resize( NumCustomAtlases );
}

/**
 *	kbParticleManager::~kbParticleManager
 */
kbParticleManager::~kbParticleManager() {
	for ( std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>::iterator it = m_ParticlePools.begin(); it != m_ParticlePools.end(); ++it ) {
		std::vector< kbParticleComponent *> & particleList = it->second;
		for ( int i = 0; i < particleList.size(); i++ ) {
			delete particleList[i];
		}
	}
}

/**
 *	kbParticleManager::SetCustomParticleTextureAtlas
 */
void kbParticleManager::SetCustomParticleTextureAtlas( const uint atlasIdx, const std::string & atlasFileName ) {
	kbErrorCheck( atlasIdx < m_CustomAtlasParticles.size(), "kbParticleManager::SetCustomParticleTextureAtlas() - Atlas index %d is out of range", atlasIdx );

	m_CustomAtlasParticles[atlasIdx].m_pAtlasTexture = ( kbTexture* )g_ResourceManager.GetResource( atlasFileName.c_str(), true );
	if ( m_CustomAtlasParticles[atlasIdx].m_pAtlasTexture == nullptr ) {
		kbWarning( "kbParticleManager::SetCustomParticleTextureAtlas() - Unable to find shader %s", atlasFileName.c_str() );
	}
}

/**
 *	kbParticleManager::SetCustomParticleShader
 */
void kbParticleManager::SetCustomParticleShader( const uint atlasIdx, const std::string & shaderFileName ) {
	kbErrorCheck( atlasIdx < m_CustomAtlasParticles.size(), "kbParticleManager::SetCustomParticleShader() - Atlas index %d is out of range", atlasIdx );

	m_CustomAtlasParticles[atlasIdx].m_pAtlasShader = ( kbShader* )g_ResourceManager.GetResource( shaderFileName.c_str(), true );
	if ( m_CustomAtlasParticles[atlasIdx].m_pAtlasShader == nullptr ) {
		kbWarning( "kbParticleManager::SetCustomParticleShader() - Unable to find shader %s", shaderFileName.c_str() );
	}
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

	g_pRenderer->RemoveParticle( pParticle );
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
 *	kbParticleManager::RenderSync
 */
void kbParticleManager::RenderSync() {

	const bool bFirstRun = (m_CurrentParticleBuffer == 255);
	if ( bFirstRun ) {
		m_CurrentParticleBuffer = 0;
	}

	const uint nextBuffer = (m_CurrentParticleBuffer + 1) % NumCustomParticleBuffers;
	for ( int iAtlas = 0; iAtlas < NumCustomAtlases; iAtlas++ ) {
		CustomAtlasParticles_t & curAtlas = m_CustomAtlasParticles[iAtlas];
		if ( curAtlas.m_pAtlasTexture == nullptr ) {
			curAtlas.m_pAtlasTexture = ( kbTexture* )g_ResourceManager.GetResource( "./assets/FX/laser_beam.jpg", true );
		}

		if ( curAtlas.m_RenderModel[0].NumVertices() == 0 ) {
			for ( int iModel = 0; iModel < NumCustomParticleBuffers; iModel++ ) {
				// todo: Need to dyamically set the number of particles to render since we might not fill up the entire buffer
				curAtlas.m_RenderModel[iModel].CreateDynamicModel( NumParticleBufferVerts, NumParticleBufferVerts, "../../kbEngine/assets/Shaders/basicParticle.kbShader", "", sizeof( kbParticleVertex )  );	// todo

				curAtlas.m_pVertexBuffer = (kbParticleVertex*)curAtlas.m_RenderModel[iModel].MapVertexBuffer();
				for ( int iVert = 0; iVert < NumParticleBufferVerts; iVert++ ) {
					curAtlas.m_pVertexBuffer[iVert].position.Set( 0.0f, 0.0f, 0.0f );
				}
				curAtlas.m_RenderModel[iModel].UnmapVertexBuffer();
			}
		}

		kbModel *const pRenderModels = curAtlas.m_RenderModel;
		if ( bFirstRun == false ) {
			g_pRenderer->RemoveParticle( &pRenderModels[m_CurrentParticleBuffer] );
			pRenderModels[m_CurrentParticleBuffer].UnmapVertexBuffer( curAtlas.m_NumIndices );
			pRenderModels[m_CurrentParticleBuffer].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
		}

		pRenderModels[m_CurrentParticleBuffer].SwapTexture( 0, curAtlas.m_pAtlasTexture, 0 );
		g_pRenderer->AddParticle( &pRenderModels[m_CurrentParticleBuffer], &pRenderModels[m_CurrentParticleBuffer], kbVec3::zero, kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ) );

		curAtlas.m_pVertexBuffer = (kbParticleVertex*)pRenderModels[nextBuffer].MapVertexBuffer();
		curAtlas.m_pIndexBuffer = ( unsigned long * ) pRenderModels[nextBuffer].MapIndexBuffer();

		curAtlas.m_NumIndices = 0;
	}

	m_CurrentParticleBuffer = nextBuffer;
}

/**
 *	kbParticleManager::AddQuad
 */
void kbParticleManager::AddQuad( const uint atlasIdx, const CustomParticleAtlasInfo_t & CustomParticleInfo ) {
	
	kbErrorCheck( atlasIdx < m_CustomAtlasParticles.size(), "kbParticleManager::AddQuad() - Invalid atlasIdx %d", atlasIdx );
	CustomAtlasParticles_t & curAtlas = m_CustomAtlasParticles[atlasIdx];

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

	kbVec3 color = CustomParticleInfo.m_Color;
	curAtlas.m_pVertexBuffer[vertexIndex + 0].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].SetColor( color );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].SetColor( color );

	curAtlas.m_pVertexBuffer[vertexIndex + 0].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 1].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 2].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	curAtlas.m_pVertexBuffer[vertexIndex + 3].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );

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

	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 0] = vertexIndex + 2;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 1] = vertexIndex + 1;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 2] = vertexIndex + 0;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 3] = vertexIndex + 3;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 4] = vertexIndex + 2;
	curAtlas.m_pIndexBuffer[curAtlas.m_NumIndices + 5] = vertexIndex + 0;
	curAtlas.m_NumIndices += 6;
}
