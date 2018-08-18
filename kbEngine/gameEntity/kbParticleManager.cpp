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

/**
 *	kbParticleManager::kbParticleManager
 */
kbParticleManager::kbParticleManager() :
		m_NumIndicesInCurrentBuffer( 0 ),
		m_CurrentParticleBuffer( 255 ),
		m_pVertexBuffer( nullptr ),
		m_pIndexBuffer( nullptr ),
		m_pParticleTexture( nullptr ) {
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
void kbParticleManager::SetCustomParticleTextureAtlas( const std::string & atlasFileName ) {
	m_pParticleTexture = ( kbTexture* )g_ResourceManager.GetResource( atlasFileName.c_str(), true );
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

	if ( m_pParticleTexture == nullptr ) {
		m_pParticleTexture = ( kbTexture* )g_ResourceManager.GetResource( "./assets/FX/laser_beam.jpg", true );
	}

	if ( m_CustomParticleBuffer[0].NumVertices() == 0 ) {
		for ( int i = 0; i < NumCustomParticleBuffers; i++ ) {
			// todo: Need to dyamically set the number of particles to render since we might not fill up the entire buffer
			m_CustomParticleBuffer[i].CreateDynamicModel( NumParticleBufferVerts, NumParticleBufferVerts, "../../kbEngine/assets/Shaders/basicParticle.kbShader", "", sizeof( kbParticleVertex )  );	// todo

			m_pVertexBuffer = (kbParticleVertex*)m_CustomParticleBuffer[i].MapVertexBuffer();
			for ( int iVert = 0; iVert < NumParticleBufferVerts; iVert++ ) {
				m_pVertexBuffer[iVert].position.Set( 0.0f, 0.0f, 0.0f );
			}
			m_CustomParticleBuffer[i].UnmapVertexBuffer();
		}
	}

	if ( m_CurrentParticleBuffer == 255 ) {
		m_CurrentParticleBuffer = 0;
	} else {
		g_pRenderer->RemoveParticle( this );
		m_CustomParticleBuffer[m_CurrentParticleBuffer].UnmapVertexBuffer( m_NumIndicesInCurrentBuffer );

		m_CustomParticleBuffer[m_CurrentParticleBuffer].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
	}

	m_CustomParticleBuffer[m_CurrentParticleBuffer].SwapTexture( 0, m_pParticleTexture, 0 );
	g_pRenderer->AddParticle( this, &m_CustomParticleBuffer[m_CurrentParticleBuffer], kbVec3::zero, kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ) );

	m_CurrentParticleBuffer++;
	if ( m_CurrentParticleBuffer >= NumCustomParticleBuffers ) {
		m_CurrentParticleBuffer = 0;
	}

	m_pVertexBuffer = (kbParticleVertex*)m_CustomParticleBuffer[m_CurrentParticleBuffer].MapVertexBuffer();
	m_pIndexBuffer = ( unsigned long * ) m_CustomParticleBuffer[m_CurrentParticleBuffer].MapIndexBuffer();

	m_NumIndicesInCurrentBuffer = 0;
}

/**
 *	kbParticleManager::AddQuad
 */
void kbParticleManager::AddQuad( const CustomParticleInfo_t & CustomParticleInfo ) {
	
	if ( m_pVertexBuffer == nullptr || m_pIndexBuffer == nullptr ) {
		return;
	}

	const int vertexIndex = m_NumIndicesInCurrentBuffer - ( m_NumIndicesInCurrentBuffer / 3 );
	m_pVertexBuffer[vertexIndex + 0].position = CustomParticleInfo.m_Position;
	m_pVertexBuffer[vertexIndex + 1].position = CustomParticleInfo.m_Position;
	m_pVertexBuffer[vertexIndex + 2].position = CustomParticleInfo.m_Position;
	m_pVertexBuffer[vertexIndex + 3].position = CustomParticleInfo.m_Position;
	
	m_pVertexBuffer[vertexIndex + 0].uv.Set( CustomParticleInfo.m_UVs[0].x, CustomParticleInfo.m_UVs[0].y );
	m_pVertexBuffer[vertexIndex + 1].uv.Set( CustomParticleInfo.m_UVs[1].x, CustomParticleInfo.m_UVs[0].y );
	m_pVertexBuffer[vertexIndex + 2].uv.Set( CustomParticleInfo.m_UVs[1].x, CustomParticleInfo.m_UVs[1].y );
	m_pVertexBuffer[vertexIndex + 3].uv.Set( CustomParticleInfo.m_UVs[0].x, CustomParticleInfo.m_UVs[1].y );

	kbVec3 color = CustomParticleInfo.m_Color;
	m_pVertexBuffer[vertexIndex + 0].SetColor( color );
	m_pVertexBuffer[vertexIndex + 1].SetColor( color );
	m_pVertexBuffer[vertexIndex + 2].SetColor( color );
	m_pVertexBuffer[vertexIndex + 3].SetColor( color );

	m_pVertexBuffer[vertexIndex + 0].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	m_pVertexBuffer[vertexIndex + 1].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	m_pVertexBuffer[vertexIndex + 2].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );
	m_pVertexBuffer[vertexIndex + 3].color[3] = ( CustomParticleInfo.m_Type == BT_FaceCamera ) ? ( 0 ) : ( 0xff );

	const float halfWidth = CustomParticleInfo.m_Width * 0.5f;
	const float halfHeight = CustomParticleInfo.m_Height * 0.5f;

	m_pVertexBuffer[vertexIndex + 0].size = kbVec2( -halfWidth,  halfHeight );
	m_pVertexBuffer[vertexIndex + 1].size = kbVec2(  halfWidth,  halfHeight );
	m_pVertexBuffer[vertexIndex + 2].size = kbVec2(  halfWidth, -halfHeight );
	m_pVertexBuffer[vertexIndex + 3].size = kbVec2( -halfWidth, -halfHeight );

	m_pVertexBuffer[vertexIndex + 0].direction = CustomParticleInfo.m_Direction;
	m_pVertexBuffer[vertexIndex + 1].direction = CustomParticleInfo.m_Direction;
	m_pVertexBuffer[vertexIndex + 2].direction = CustomParticleInfo.m_Direction;
	m_pVertexBuffer[vertexIndex + 3].direction = CustomParticleInfo.m_Direction;

	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 0] = vertexIndex + 2;
	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 1] = vertexIndex + 1;
	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 2] = vertexIndex + 0;
	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 3] = vertexIndex + 3;
	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 4] = vertexIndex + 2;
	m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 5] = vertexIndex + 0;
	m_NumIndicesInCurrentBuffer += 6;
}
