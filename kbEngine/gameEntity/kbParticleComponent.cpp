//===================================================================================================
// kbParticleComponent.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbRenderer_defs.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbParticleComponent)

static const uint NumParticleBufferVerts = 10000;


/**
 *	kbParticleComponent::Initialize
 */
void kbParticleComponent::Constructor() {
	m_TotalDuration = -1.0f;
	m_MinParticleSpawnRate = 1.0f;
	m_MaxParticleSpawnRate = 2.0f;
	m_MinParticleStartVelocity.Set( -2.0f, 5.0f, -2.0f );
	m_MaxParticleStartVelocity.Set( 2.0f, 5.0f, 2.0f );
	m_MinParticleEndVelocity.Set( 0.0f, 0.0f, 0.0f );
	m_MaxParticleEndVelocity.Set( 0.0f, 0.0f, 0.0f );
	m_MinParticleStartSize.Set( 3.0f, 3.0f, 3.0f );
	m_MaxParticleStartSize.Set( 3.0f, 3.0f, 3.0f );
	m_MinParticleEndSize.Set( 3.0f, 3.0f, 3.0f );
	m_MaxParticleEndSize.Set( 3.0f, 3.0f, 3.0f );
	m_ParticleMinDuration = 3.0f;
	m_ParticleMaxDuration = 3.0f;
	m_ParticleStartColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_ParticleEndColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_MinBurstCount = 0;
	m_MaxBurstCount = 0;
	m_BurstCount = 0;
	m_ParticleBillboardType = BT_FaceCamera;
	m_Gravity.Set( 0.0f, 0.0f, 0.0f );
	m_RenderPassBucket = 0;
	m_bLockVelocity = false;

	m_LeftOverTime = 0.0f;
	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_CurrentParticleBuffer = 255;
	m_NumIndicesInCurrentBuffer = 0;

	m_pParticleTexture = (kbTexture*)g_ResourceManager.GetResource( "../../kbEngine/assets/Textures/Editor/white.bmp" );
	m_pParticleShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicParticle.kbShader", true );

	m_bIsPooled = false;
	m_ParticleTemplate = nullptr;
}

/**
 *	~kbParticleComponent
 */
kbParticleComponent::~kbParticleComponent() {
	StopParticleSystem();

	for ( int i = 0; i < NumParticleBuffers; i++ ) {
		m_ParticleBuffer[i].Release();
	}
}

/**
 *	kbParticleComponent::StopParticleSystem
 */
void kbParticleComponent::StopParticleSystem() {

	if ( g_pRenderer->IsRenderingSynced() == false ) {
		kbError( "Shutting down particle component even though rendering is not synced" );
	}

	g_pRenderer->RemoveParticle( m_RenderObject );
	for ( int i = 0; i < NumParticleBuffers; i++ ) {
		if ( m_ParticleBuffer[i].IsVertexBufferMapped() ) {
			m_ParticleBuffer[i].UnmapVertexBuffer( 0 );
		}

		if ( m_ParticleBuffer[i].IsIndexBufferMapped() ) {
			m_ParticleBuffer[i].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
		}
	}
	m_CurrentParticleBuffer = 255;
	m_NumIndicesInCurrentBuffer = 0;
	m_Particles.clear();
	m_LeftOverTime = 0.0f;

	m_ParticleBillboardType = BT_FaceCamera;
}

/**
 *	kbParticleComponent::Update_Internal
 */
void kbParticleComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	const float eps = 0.00000001f;
	if ( m_pVertexBuffer == nullptr || m_pIndexBuffer == nullptr ) {
		return;
	}

	if ( m_MaxBurstCount <= 0 && ( m_MaxParticleSpawnRate <= eps || m_MinParticleSpawnRate < eps || m_MaxParticleSpawnRate < m_MinParticleSpawnRate || m_ParticleMinDuration <= eps ) ) {
		return;
	}
	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );

	int iVertex = 0;
	int curVBPosition = 0;
	const kbVec3 scale = GetScale();
	const kbVec3 direction = GetOrientation().ToMat4()[2].ToVec3();
	const byte iBillboardType = (int)m_ParticleBillboardType;

	for ( int i = (int)m_Particles.size() - 1; i >= 0 ; i-- ) {
		m_Particles[i].m_LifeLeft -= DeltaTime;

		if ( m_Particles[i].m_LifeLeft <= 0.0f ) {
			std::swap( m_Particles[i], m_Particles.back() );
			m_Particles.pop_back();
			continue;
		}

		const float LerpValue = m_Particles[i].m_LifeLeft / m_Particles[i].m_TotalLife;
		kbVec3 curVelocity = kbLerp( m_Particles[i].m_EndVelocity, m_Particles[i].m_StartVelocity, LerpValue );
		curVelocity += m_Gravity * ( m_Particles[i].m_TotalLife - m_Particles[i].m_LifeLeft );

		m_Particles[i].m_Position = m_Particles[i].m_Position + curVelocity * DeltaTime;

		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 2] = ( curVBPosition * 4 ) + 0;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 1] = ( curVBPosition * 4 ) + 1;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 0] = ( curVBPosition * 4 ) + 2;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 5] = ( curVBPosition * 4 ) + 0;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 4] = ( curVBPosition * 4 ) + 2;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 3] = ( curVBPosition * 4 ) + 3;
		m_NumIndicesInCurrentBuffer += 6;

		m_pVertexBuffer[iVertex + 0].position = m_Particles[i].m_Position;
		m_pVertexBuffer[iVertex + 1].position = m_Particles[i].m_Position;
		m_pVertexBuffer[iVertex + 2].position = m_Particles[i].m_Position;
		m_pVertexBuffer[iVertex + 3].position = m_Particles[i].m_Position;

		m_pVertexBuffer[iVertex + 0].uv.Set( 0.0f, 0.0f );
		m_pVertexBuffer[iVertex + 1].uv.Set( 1.0f, 0.0f );
		m_pVertexBuffer[iVertex + 2].uv.Set( 1.0f, 1.0f );
		m_pVertexBuffer[iVertex + 3].uv.Set( 0.0f, 1.0f );
		kbVec2 curSize = kbLerp( m_Particles[i].m_EndSize, m_Particles[i].m_StartSize, LerpValue );
		curSize.x *= scale.x;
		curSize.y *= scale.y;

		m_pVertexBuffer[iVertex + 0].size = kbVec2( -curSize.x,  curSize.y );
		m_pVertexBuffer[iVertex + 1].size = kbVec2(  curSize.x,  curSize.y );
		m_pVertexBuffer[iVertex + 2].size = kbVec2(  curSize.x, -curSize.y );
		m_pVertexBuffer[iVertex + 3].size = kbVec2( -curSize.x, -curSize.y );

		kbVec4 curColor = kbLerp( m_ParticleEndColor, m_ParticleStartColor, LerpValue );
		byte byteColor[4] = { ( byte )kbClamp( curColor.x * 255.0f, 0.0f, 255.0f  ), ( byte )kbClamp( curColor.y * 255.0f, 0.0f, 255.0f ), ( byte )kbClamp( curColor.z * 255.0f, 0.0f, 255.0f ), ( byte )kbClamp( curColor.w * 255.0f, 0.0f, 255.0f ) };
		memcpy(&m_pVertexBuffer[iVertex + 0].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 1].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 2].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 3].color, byteColor, sizeof(byteColor));

		m_pVertexBuffer[iVertex + 0].direction = direction;
		m_pVertexBuffer[iVertex + 1].direction = direction;
		m_pVertexBuffer[iVertex + 2].direction = direction;
		m_pVertexBuffer[iVertex + 3].direction = direction;

		m_pVertexBuffer[iVertex + 0].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 1].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 2].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 3].billboardType[0] = iBillboardType;

		m_pVertexBuffer[iVertex + 0].billboardType[1] = (byte)kbClamp( m_Particles[i].m_Randoms[0] * 255.0f, 0.0f, 255.0f );
		m_pVertexBuffer[iVertex + 1].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];
		m_pVertexBuffer[iVertex + 2].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];
		m_pVertexBuffer[iVertex + 3].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];

		m_pVertexBuffer[iVertex + 0].billboardType[2] = (byte)kbClamp( m_Particles[i].m_Randoms[1] * 255.0f, 0.0f, 255.0f );
		m_pVertexBuffer[iVertex + 1].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];
		m_pVertexBuffer[iVertex + 2].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];
		m_pVertexBuffer[iVertex + 3].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];

		m_pVertexBuffer[iVertex + 0].billboardType[3] = (byte)kbClamp( m_Particles[i].m_Randoms[2] * 255.0f, 0.0f, 255.0f );
		m_pVertexBuffer[iVertex + 1].billboardType[3] = m_pVertexBuffer[iVertex + 0].billboardType[3];
		m_pVertexBuffer[iVertex + 2].billboardType[3] = m_pVertexBuffer[iVertex + 0].billboardType[3];
		m_pVertexBuffer[iVertex + 3].billboardType[3] = m_pVertexBuffer[iVertex + 0].billboardType[3];
		iVertex += 4;
		curVBPosition++;

		/*kbLog( "Pos = (%f %f %f), (%f %f %f), (%f %f %f ), (%f %f %f )", 
				m_pVertexBuffer[iVertex + 0].position.x, m_pVertexBuffer[iVertex + 0].position.y, m_pVertexBuffer[iVertex + 0].position.z,
				m_pVertexBuffer[iVertex + 1].position.x, m_pVertexBuffer[iVertex + 1].position.y, m_pVertexBuffer[iVertex + 1].position.z,
				m_pVertexBuffer[iVertex + 2].position.x, m_pVertexBuffer[iVertex + 2].position.y, m_pVertexBuffer[iVertex + 2].position.z,
				m_pVertexBuffer[iVertex + 3].position.x, m_pVertexBuffer[iVertex + 3].position.y, m_pVertexBuffer[iVertex + 3].position.z );	*/												
	}

	m_TimeAlive += DeltaTime;
	if ( m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_BurstCount <= 0 ) {
		return;
	}

	const float invMinSpawnRate = ( m_MinParticleSpawnRate > 0.0f ) ? ( 1.0f / m_MinParticleSpawnRate ) : ( 0.0f );
	const float invMaxSpawnRate = ( m_MaxParticleSpawnRate > 0.0f ) ? ( 1.0f / m_MaxParticleSpawnRate ) : ( 0.0f );
	float TimeLeft = DeltaTime - m_LeftOverTime;
	int currentListEnd = (int)m_Particles.size();
	float NextSpawn = 0.0f;

	kbMat4 ownerMatrix = GetOwner()->GetOrientation().ToMat4();

	// Spawn particles
	const kbVec3 MyPosition = GetPosition();
	while ( ( m_MaxParticleSpawnRate > 0 && TimeLeft >= NextSpawn ) || m_BurstCount > 0 ) {
		kbParticle_t newParticle;
		newParticle.m_StartVelocity.x = m_MinParticleStartVelocity.x + ( kbfrand() * ( m_MaxParticleStartVelocity.x - m_MinParticleStartVelocity.x ) );
		newParticle.m_StartVelocity.y = m_MinParticleStartVelocity.y + ( kbfrand() * ( m_MaxParticleStartVelocity.y - m_MinParticleStartVelocity.y ) );
		newParticle.m_StartVelocity.z = m_MinParticleStartVelocity.z + ( kbfrand() * ( m_MaxParticleStartVelocity.z - m_MinParticleStartVelocity.z ) );
		newParticle.m_StartVelocity = newParticle.m_StartVelocity * ownerMatrix;
		
		if ( m_bLockVelocity ) {
			newParticle.m_EndVelocity = newParticle.m_StartVelocity;
		} else {
			newParticle.m_EndVelocity.x = m_MinParticleEndVelocity.x + ( kbfrand() * ( m_MaxParticleEndVelocity.x - m_MinParticleEndVelocity.x ) );
			newParticle.m_EndVelocity.y = m_MinParticleEndVelocity.y + ( kbfrand() * ( m_MaxParticleEndVelocity.y - m_MinParticleEndVelocity.y ) );
			newParticle.m_EndVelocity.z = m_MinParticleEndVelocity.z + ( kbfrand() * ( m_MaxParticleEndVelocity.z - m_MinParticleEndVelocity.z ) );
			newParticle.m_EndVelocity = newParticle.m_StartVelocity * ownerMatrix;
		}

		newParticle.m_Position = MyPosition + newParticle.m_StartVelocity * TimeLeft;
		newParticle.m_LifeLeft = m_ParticleMinDuration + ( kbfrand() * ( m_ParticleMaxDuration - m_ParticleMinDuration ) );
		newParticle.m_TotalLife = newParticle.m_LifeLeft;

		const float startSizeRand = kbfrand();
		newParticle.m_StartSize.x = m_MinParticleStartSize.x + ( startSizeRand * ( m_MinParticleStartSize.x - m_MaxParticleStartSize.x ) );
		newParticle.m_StartSize.y = m_MinParticleStartSize.y + ( startSizeRand * ( m_MinParticleStartSize.y - m_MaxParticleStartSize.y ) );

		const float endSizeRand = kbfrand();
		newParticle.m_EndSize.x = m_MinParticleEndSize.x + ( endSizeRand * ( m_MinParticleEndSize.x - m_MaxParticleEndSize.x ) );
		newParticle.m_EndSize.y = m_MinParticleEndSize.y + ( endSizeRand * ( m_MinParticleEndSize.y - m_MaxParticleEndSize.y ) );

		newParticle.m_Randoms[0] = kbfrand();
		newParticle.m_Randoms[1] = kbfrand();
		newParticle.m_Randoms[2] = kbfrand();

		if ( m_BurstCount > 0 ) {
			m_BurstCount--;
		} else {
			TimeLeft -= NextSpawn; 
			NextSpawn = invMaxSpawnRate + ( kbfrand() * ( invMinSpawnRate - invMaxSpawnRate ) );
		}

		m_Particles.push_back( newParticle );
	}


//kbLog( "Num Indices = %d", m_NumIndicesInCurrentBuffer );
	m_LeftOverTime = NextSpawn - TimeLeft;
}

/**
 *	kbParticleComponent::EditorChange
 */
void kbParticleComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "ParticleTexture" ) {
		if ( m_pParticleTexture != nullptr && m_pParticleTexture->GetGPUTexture() == nullptr ) {
			m_pParticleTexture = (kbTexture *)g_ResourceManager.GetResource( m_pParticleTexture->GetFullFileName() );
		}
	}
}

/**
 *  kbParticleComponent::RenderSync
 */
void kbParticleComponent::RenderSync() {
	Super::RenderSync();


	if ( g_UseEditor && IsEnabled() == true && ( m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_BurstCount <= 0 ) ) {
		StopParticleSystem();
		Enable( false );
		Enable( true );
		return;
	}

	if ( ( g_UseEditor && IsEnabled() == false ) || ( m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_NumIndicesInCurrentBuffer == 0 ) ) {
		StopParticleSystem();
		Enable( false );
		if ( m_bIsPooled ) {
			g_pGame->GetParticleManager()->ReturnParticleComponent( this );
		}
		return;
	}

	if ( m_ParticleBuffer[0].NumVertices() == 0 ) {
		for ( int i = 0; i < NumParticleBuffers; i++ ) {
			m_ParticleBuffer[i].CreateDynamicModel( NumParticleBufferVerts, NumParticleBufferVerts, m_pParticleShader, nullptr, sizeof(kbParticleVertex) );
			m_pVertexBuffer = (kbParticleVertex*)m_ParticleBuffer[i].MapVertexBuffer();
			for ( int iVert = 0; iVert < NumParticleBufferVerts; iVert++ ) {
				m_pVertexBuffer[iVert].position.Set( 0.0f, 0.0f, 0.0f );
			}
			m_ParticleBuffer[i].UnmapVertexBuffer();
		}
	}


	m_RenderObject.m_pComponent = static_cast<const kbComponent*>( this );
	m_RenderObject.m_pModel = nullptr;
	m_RenderObject.m_RenderPass = RP_Translucent;
	m_RenderObject.m_Position = GetPosition();
	m_RenderObject.m_Orientation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );
	m_RenderObject.m_RenderPassBucket = this->m_RenderPassBucket;;

	if ( m_CurrentParticleBuffer == 255 ) {
		m_CurrentParticleBuffer = 0;
	} else {
		g_pRenderer->RemoveParticle( m_RenderObject );

		m_ParticleBuffer[m_CurrentParticleBuffer].UnmapVertexBuffer( m_NumIndicesInCurrentBuffer );
		m_ParticleBuffer[m_CurrentParticleBuffer].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
	}

	m_ParticleBuffer[m_CurrentParticleBuffer].SwapTexture( 0, m_pParticleTexture, 0 );

	m_RenderObject.m_pModel = &m_ParticleBuffer[m_CurrentParticleBuffer];
	g_pRenderer->AddParticle( m_RenderObject );

	m_CurrentParticleBuffer++;
	if ( m_CurrentParticleBuffer >= NumParticleBuffers ) {
		m_CurrentParticleBuffer = 0;
	}

	m_pVertexBuffer = (kbParticleVertex*)m_ParticleBuffer[m_CurrentParticleBuffer].MapVertexBuffer();
	m_pIndexBuffer = ( unsigned long * ) m_ParticleBuffer[m_CurrentParticleBuffer].MapIndexBuffer();

	m_NumIndicesInCurrentBuffer = 0;
}

/**
 *	kbParticleComponent::SetEnable_Internal(
 */
void kbParticleComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );


	if ( isEnabled ) {

		m_TimeAlive = 0.0f;
		if ( m_MaxBurstCount > 0 ) {
			m_BurstCount = m_MinBurstCount;
			if ( m_MaxBurstCount > m_MinBurstCount ) {
				m_BurstCount += rand() % ( m_MaxBurstCount - m_MinBurstCount );
			}
		}
	}
}
