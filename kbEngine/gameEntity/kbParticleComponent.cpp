//===================================================================================================
// kbParticleComponent.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbRenderer_defs.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbParticleComponent)

static const uint NumParticleBufferVerts = 10000;
static const uint NumMeshVerts = 10000;

/**
 *	kbParticle_t::bParticle_t
 */
kbParticle_t::kbParticle_t() {
	m_pSrcModelEmitter = nullptr;
}

/**
 *	kbParticle_t::~kbParticle_t
 */
kbParticle_t::~kbParticle_t() {
}

/**
 *	kbParticle_t::Shutdown
 */
void kbParticle_t::Shutdown() {

	if ( m_RenderObject.m_pComponent != nullptr ) {
		g_pRenderer->RemoveRenderObject( m_RenderObject );
		g_pGame->GetParticleManager()->ReturnComponentToPool( m_RenderObject.m_pComponent );
	}

	m_RenderObject.m_pComponent = nullptr;
	m_pSrcModelEmitter = nullptr;
}

/**
 *	kbParticleComponent::Initialize
 */
void kbParticleComponent::Constructor() {
	m_MaxParticlesToEmit = -1;
	m_TotalDuration = -1.0f;
	m_StartDelay = 0.0f;

	m_MinParticleSpawnRate = 1.0f;
	m_MaxParticleSpawnRate = 2.0f;
	m_MinParticleStartVelocity.Set( -2.0f, 5.0f, -2.0f );
	m_MaxParticleStartVelocity.Set( 2.0f, 5.0f, 2.0f );
	m_MinParticleEndVelocity.Set( 0.0f, 0.0f, 0.0f );
	m_MaxParticleEndVelocity.Set( 0.0f, 0.0f, 0.0f );
	m_MinStartRotationRate = 0;
	m_MaxStartRotationRate = 0;
	m_MinEndRotationRate = 0;
	m_MaxEndRotationRate = 0;

	m_MinStart3DRotation = kbVec3::zero;
	m_MaxStart3DRotation = kbVec3::zero;

	m_MinStart3DOffset = kbVec3::zero;
	m_MaxStart3DOffset = kbVec3::zero;

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
	m_StartDelayRemaining = 0;
	m_NumEmittedParticles = 0;
	m_ParticleBillboardType = BT_FaceCamera;
	m_Gravity.Set( 0.0f, 0.0f, 0.0f );
	m_TranslucencySortBias = 0.0f;

	m_LeftOverTime = 0.0f;
	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_CurrentParticleBuffer = 255;
	m_NumIndicesInCurrentBuffer = 0;
	m_bIsSpawning = true;

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

	kbErrorCheck( g_pRenderer->IsRenderingSynced() == true, "kbParticleComponent::StopParticleSystem() - Shutting down particle component even though rendering is not synced" );

	if ( IsModelEmitter() ) {
		return;
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

	//m_ParticleBillboardType = BT_FaceCamera;
}

/**
 *	kbParticleComponent::Update_Internal
 */
void kbParticleComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_StartDelay > 0 ) {

		m_StartDelay -= DeltaTime;
		if ( m_StartDelay < 0 ) {
			m_TimeAlive = 0.0f;
			if ( m_MaxBurstCount > 0 ) {
				m_BurstCount = m_MinBurstCount;
				if ( m_MaxBurstCount > m_MinBurstCount ) {
					m_BurstCount += rand() % ( m_MaxBurstCount - m_MinBurstCount );
				}
			}
		} else {
			return;
		}

	}

	const float eps = 0.00000001f;
	if ( IsModelEmitter() == false && ( m_pVertexBuffer == nullptr || m_pIndexBuffer == nullptr ) ) {
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
	byte iBillboardType = 0;
	switch( m_ParticleBillboardType ) {
		case EBillboardType::BT_FaceCamera : iBillboardType = 0; break;
		case EBillboardType::BT_AxialBillboard : iBillboardType = 1; break;
		case EBillboardType::BT_AlignAlongVelocity : iBillboardType = 1; break;
		default: kbWarning( "kbParticleComponent::Update_Internal() - Invalid billboard type specified" ); break;
	}

	for ( int i = (int)m_Particles.size() - 1; i >= 0 ; i-- ) {
		kbParticle_t & particle = m_Particles[i];

		if ( particle.m_LifeLeft >= 0.0f ) {
			particle.m_LifeLeft -= DeltaTime;

			if ( particle.m_LifeLeft <= 0.0f || ( IsModelEmitter() && particle.m_RenderObject.m_pComponent  == nullptr ) ) {
				particle.Shutdown();
				VectorRemoveFastIndex( m_Particles, i );
				continue;
			}
		}

		const float normalizedTime = ( particle.m_TotalLife - particle.m_LifeLeft ) / particle.m_TotalLife;
		kbVec3 curVelocity = kbVec3::zero;

		if ( m_VelocityOverLifeTimeCurve.size() == 0 ) {
			curVelocity = kbLerp( particle.m_StartVelocity, particle.m_EndVelocity, normalizedTime );
		} else {
			const float velCurve = kbAnimEvent::Evaluate( m_VelocityOverLifeTimeCurve, normalizedTime );
			curVelocity = particle.m_StartVelocity * velCurve;
		}

		curVelocity += m_Gravity * ( particle.m_TotalLife - particle.m_LifeLeft );

		particle.m_Position = particle.m_Position + curVelocity * DeltaTime;

		const float curRotationRate = kbLerp( particle.m_StartRotation, particle.m_EndRotation, normalizedTime );
		particle.m_Rotation += curRotationRate * DeltaTime;

		const kbVec2 curSize = kbLerp( particle.m_StartSize * scale.x, particle.m_EndSize * scale.y, normalizedTime );

		kbVec4 curColor = kbVec4::zero;
		if ( m_ColorOverLifeTimeCurve.size() == 0 ) {
			curColor = kbLerp( m_ParticleStartColor, m_ParticleEndColor, normalizedTime );
		} else {
			curColor = kbVectorAnimEvent::Evaluate( m_ColorOverLifeTimeCurve, normalizedTime );
		}

		if ( m_AlphaOverLifeTimeCurve.size() == 0 ) {
			curColor.w = kbLerp( m_ParticleStartColor.w, m_ParticleEndColor.w, normalizedTime );
		} else {
			curColor.w = kbAnimEvent::Evaluate( m_AlphaOverLifeTimeCurve, normalizedTime );
		}

		byte byteColor[4] = { ( byte )kbClamp( curColor.x * 255.0f, 0.0f, 255.0f  ), ( byte )kbClamp( curColor.y * 255.0f, 0.0f, 255.0f ), ( byte )kbClamp( curColor.z * 255.0f, 0.0f, 255.0f ), ( byte )kbClamp( curColor.w * 255.0f, 0.0f, 255.0f ) };

		if ( IsModelEmitter() ) {

			kbRenderObject & renderObj = particle.m_RenderObject;

			renderObj.m_Position = particle.m_Position;
			//renderObj.m_Orientation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );	TODO
			renderObj.m_Scale.Set( curSize.x, curSize.x, curSize.x );

			for ( int iMat = 0; iMat < renderObj.m_Materials.size(); iMat++ ) {
				renderObj.m_Materials[iMat].SetVec4( "particleColor", curColor );
			}

			g_pRenderer->UpdateRenderObject( renderObj );
			continue;
		}

		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 2] = ( curVBPosition * 4 ) + 0;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 1] = ( curVBPosition * 4 ) + 1;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 0] = ( curVBPosition * 4 ) + 2;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 5] = ( curVBPosition * 4 ) + 0;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 4] = ( curVBPosition * 4 ) + 2;
		m_pIndexBuffer[m_NumIndicesInCurrentBuffer + 3] = ( curVBPosition * 4 ) + 3;
		m_NumIndicesInCurrentBuffer += 6;

		m_pVertexBuffer[iVertex + 0].position = particle.m_Position;
		m_pVertexBuffer[iVertex + 1].position = particle.m_Position;
		m_pVertexBuffer[iVertex + 2].position = particle.m_Position;
		m_pVertexBuffer[iVertex + 3].position = particle.m_Position;

		m_pVertexBuffer[iVertex + 0].uv.Set( 0.0f, 0.0f );
		m_pVertexBuffer[iVertex + 1].uv.Set( 1.0f, 0.0f );
		m_pVertexBuffer[iVertex + 2].uv.Set( 1.0f, 1.0f );
		m_pVertexBuffer[iVertex + 3].uv.Set( 0.0f, 1.0f );


		m_pVertexBuffer[iVertex + 0].size = kbVec2( -curSize.x,  curSize.y );
		m_pVertexBuffer[iVertex + 1].size = kbVec2(  curSize.x,  curSize.y );
		m_pVertexBuffer[iVertex + 2].size = kbVec2(  curSize.x, -curSize.y );
		m_pVertexBuffer[iVertex + 3].size = kbVec2( -curSize.x, -curSize.y );

		memcpy(&m_pVertexBuffer[iVertex + 0].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 1].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 2].color, byteColor, sizeof(byteColor));
		memcpy(&m_pVertexBuffer[iVertex + 3].color, byteColor, sizeof(byteColor));

		if ( m_ParticleBillboardType == EBillboardType::BT_AlignAlongVelocity ) {
			kbVec3 alignVec = kbVec3::up;
			if ( curVelocity.LengthSqr() > 0.01f ) {
				alignVec = curVelocity.Normalized();
				m_pVertexBuffer[iVertex + 0].direction = alignVec;
				m_pVertexBuffer[iVertex + 1].direction = alignVec;
				m_pVertexBuffer[iVertex + 2].direction = alignVec;
				m_pVertexBuffer[iVertex + 3].direction = alignVec;
			}
		} else {
			m_pVertexBuffer[iVertex + 0].direction = direction;
			m_pVertexBuffer[iVertex + 1].direction = direction;
			m_pVertexBuffer[iVertex + 2].direction = direction;
			m_pVertexBuffer[iVertex + 3].direction = direction;
		}

		m_pVertexBuffer[iVertex + 0].rotation = particle.m_Rotation;
		m_pVertexBuffer[iVertex + 1].rotation = particle.m_Rotation;
		m_pVertexBuffer[iVertex + 2].rotation = particle.m_Rotation;
		m_pVertexBuffer[iVertex + 3].rotation = particle.m_Rotation;

		m_pVertexBuffer[iVertex + 0].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 1].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 2].billboardType[0] = iBillboardType;
		m_pVertexBuffer[iVertex + 3].billboardType[0] = iBillboardType;

		m_pVertexBuffer[iVertex + 0].billboardType[1] = (byte)kbClamp( particle.m_Randoms[0] * 255.0f, 0.0f, 255.0f );
		m_pVertexBuffer[iVertex + 1].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];
		m_pVertexBuffer[iVertex + 2].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];
		m_pVertexBuffer[iVertex + 3].billboardType[1] = m_pVertexBuffer[iVertex + 0].billboardType[1];

		m_pVertexBuffer[iVertex + 0].billboardType[2] = (byte)kbClamp( particle.m_Randoms[1] * 255.0f, 0.0f, 255.0f );
		m_pVertexBuffer[iVertex + 1].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];
		m_pVertexBuffer[iVertex + 2].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];
		m_pVertexBuffer[iVertex + 3].billboardType[2] = m_pVertexBuffer[iVertex + 0].billboardType[2];

		m_pVertexBuffer[iVertex + 0].billboardType[3] = (byte)kbClamp( particle.m_Randoms[2] * 255.0f, 0.0f, 255.0f );
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
	kbVec3 MyPosition = GetPosition();
	while ( m_bIsSpawning && ( ( m_MaxParticleSpawnRate > 0 && TimeLeft >= NextSpawn ) || m_BurstCount > 0 ) && ( m_MaxParticlesToEmit <= 0 || m_NumEmittedParticles < m_MaxParticlesToEmit ) ) {

		if ( m_MinStart3DOffset.Compare( kbVec3::zero ) == false || m_MaxStart3DOffset.Compare( kbVec3::zero ) == false ) {
			const kbVec3 startingOffset = kbVec3Rand( m_MinStart3DOffset, m_MaxStart3DOffset );
			MyPosition += startingOffset;
		}

		kbParticle_t newParticle;
		newParticle.m_StartVelocity = kbVec3Rand( m_MinParticleStartVelocity, m_MaxParticleStartVelocity ) * ownerMatrix;
		newParticle.m_EndVelocity = kbVec3Rand( m_MinParticleEndVelocity, m_MaxParticleEndVelocity ) * ownerMatrix;

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

		newParticle.m_StartRotation = kbfrand( m_MinStartRotationRate, m_MaxStartRotationRate );
		newParticle.m_EndRotation = kbfrand( m_MinEndRotationRate, m_MaxEndRotationRate );

		if ( IsModelEmitter() && m_ModelEmitter.size() ) {
			const kbGameComponent *const pComponent = g_pGame->GetParticleManager()->GetComponentFromPool();
			if ( pComponent != nullptr ) {
				const int randIdx = rand() % m_ModelEmitter.size();
				kbModelEmitter *const pModelEmitter = &m_ModelEmitter[randIdx];
				newParticle.m_pSrcModelEmitter = &m_ModelEmitter[randIdx];

				kbRenderObject & renderObj = newParticle.m_RenderObject;
				renderObj.m_pComponent = pComponent;

				renderObj.m_pModel = pModelEmitter->GetModel();
				renderObj.m_Materials = pModelEmitter->GetShaderParamOverrides();
				renderObj.m_RenderPass = RP_Translucent;
				renderObj.m_TranslucencySortBias = 0;
				renderObj.m_Position = newParticle.m_Position;

				renderObj.m_Scale = kbVec3::one;
				renderObj.m_EntityId = 0;
				renderObj.m_CullDistance = -1;
				renderObj.m_bCastsShadow = false;
				renderObj.m_bIsSkinnedModel  = false;
				renderObj.m_bIsFirstAdd = true;
				renderObj.m_bIsRemove = false;

				renderObj.m_Orientation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );
				if ( m_MinStart3DRotation.Compare( kbVec3::zero ) == false || m_MaxStart3DRotation.Compare( kbVec3::zero ) == false ) {
					const kbVec3 startingRotation = kbVec3Rand( m_MinStart3DRotation, m_MaxStart3DRotation );
					kbQuat xAxis, yAxis, zAxis;
					xAxis.FromAxisAngle( kbVec3( 1.0f, 0.0f, 0.0f ), kbToRadians( startingRotation.x ) );
					yAxis.FromAxisAngle( kbVec3( 0.0f, 1.0f, 0.0f ), kbToRadians( startingRotation.y ) );
					zAxis.FromAxisAngle( kbVec3( 0.0f, 0.0f, 1.0f ), kbToRadians( startingRotation.z ) );

					renderObj.m_Orientation = xAxis * yAxis * zAxis;
				}

				g_pRenderer->AddRenderObject( renderObj );
			} else {
				continue;
			}
		}

		if ( newParticle.m_StartRotation != 0 || newParticle.m_EndRotation != 0 ) {
			newParticle.m_Rotation = kbfrand() * kbPI;
		} else {
			newParticle.m_Rotation = 0;
		}

		if ( m_BurstCount > 0 ) {
			m_BurstCount--;
		} else {
			TimeLeft -= NextSpawn; 
			NextSpawn = invMaxSpawnRate + ( kbfrand() * ( invMinSpawnRate - invMaxSpawnRate ) );
		}

		m_NumEmittedParticles++;
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

	// Editor Hack!
	if ( propertyName == "Materials" ) {
		for ( int i = 0; i < this->m_MaterialList.size(); i++ ) {
			m_MaterialList[i].SetOwningComponent( this );
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

	if ( IsEnabled() == false || ( m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_NumIndicesInCurrentBuffer == 0 ) ) {
		StopParticleSystem();
		Enable( false );
		if ( m_bIsPooled ) {
			g_pGame->GetParticleManager()->ReturnParticleComponent( this );
		}
		return;
	}

	if ( IsModelEmitter() ) {
		return;
	}

	if ( m_ParticleBuffer[0].NumVertices() == 0 ) {
		for ( int i = 0; i < NumParticleBuffers; i++ ) {
			m_ParticleBuffer[i].CreateDynamicModel( NumParticleBufferVerts, NumParticleBufferVerts, nullptr, nullptr, sizeof(kbParticleVertex) );
			m_pVertexBuffer = (kbParticleVertex*)m_ParticleBuffer[i].MapVertexBuffer();
			for ( int iVert = 0; iVert < NumParticleBufferVerts; iVert++ ) {
				m_pVertexBuffer[iVert].position.Set( 0.0f, 0.0f, 0.0f );
			}
			m_ParticleBuffer[i].UnmapVertexBuffer();
		}
	}

	m_RenderObject.m_pComponent = this;
	m_RenderObject.m_pModel = nullptr;
	m_RenderObject.m_RenderPass = RP_Translucent;
	m_RenderObject.m_Position = GetPosition();
	m_RenderObject.m_Orientation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );
	m_RenderObject.m_TranslucencySortBias = m_TranslucencySortBias;

	// Update materials
	m_RenderObject.m_Materials.clear();
	for ( int i = 0; i < m_MaterialList.size(); i++ ) {
		kbMaterialComponent & matComp = m_MaterialList[i];
	
		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_pShader = matComp.GetShader();
	
		auto srcShaderParams = matComp.GetShaderParams();
		for ( int j = 0; j < srcShaderParams.size(); j++ ) {
			if ( srcShaderParams[j].GetTexture() != nullptr ) {
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetTexture() );
			} else if ( srcShaderParams[j].GetRenderTexture() != nullptr ) {
	
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetRenderTexture() );
			} else {
				newShaderParams.SetVec4( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetVector() );
			}
		}
	
		m_RenderObject.m_Materials.push_back( newShaderParams );
	}

	if ( m_CurrentParticleBuffer == 255 ) {
		m_CurrentParticleBuffer = 0;
	} else {
		g_pRenderer->RemoveParticle( m_RenderObject );

		m_ParticleBuffer[m_CurrentParticleBuffer].UnmapVertexBuffer( m_NumIndicesInCurrentBuffer );
		m_ParticleBuffer[m_CurrentParticleBuffer].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
	}

	m_RenderObject.m_pModel = &m_ParticleBuffer[m_CurrentParticleBuffer];
	g_pRenderer->AddParticle( m_RenderObject );

	m_CurrentParticleBuffer++;
	if ( m_CurrentParticleBuffer >= NumParticleBuffers ) {
		m_CurrentParticleBuffer = 0;
	}

	m_pVertexBuffer = (kbParticleVertex*)m_ParticleBuffer[m_CurrentParticleBuffer].MapVertexBuffer();
	m_pIndexBuffer = (ushort*) m_ParticleBuffer[m_CurrentParticleBuffer].MapIndexBuffer();

	m_NumIndicesInCurrentBuffer = 0;
}

/**
 *	kbParticleComponent::SetEnable_Internal
 */
void kbParticleComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( isEnabled ) {
		m_bIsSpawning = true;
		m_NumEmittedParticles = 0;

		if ( m_StartDelay > 0 ) {
			m_StartDelayRemaining = m_StartDelay;
		} else {
			m_TimeAlive = 0.0f;
			if ( m_MaxBurstCount > 0 ) {
				m_BurstCount = m_MinBurstCount;
				if ( m_MaxBurstCount > m_MinBurstCount ) {
					m_BurstCount += rand() % ( m_MaxBurstCount - m_MinBurstCount );
				}
			}
		}

		for ( int i = 0; i < m_ModelEmitter.size(); i++ ) {
			m_ModelEmitter[i].Init();
		}

	} else {
		g_pRenderer->RemoveParticle( m_RenderObject );
	}
}

/**
 *	kbParticleComponent::EnableNewSpawns
 */
void kbParticleComponent::EnableNewSpawns( const bool bEnable ) {

	if ( m_bIsSpawning == bEnable ) {
		return;
	}

	m_bIsSpawning = bEnable;
	m_LeftOverTime = 0.0f;
}

/**
 *	kbModelEmitter::Constructor
 */
void kbModelEmitter::Constructor() {
	m_pModel = nullptr;

}

/**
 *	kbModelEmitter::Init
 */
void kbModelEmitter::Init() {

	for ( int i = 0; i < m_MaterialList.size(); i++ ) {
		const kbMaterialComponent & matComp = m_MaterialList[i];
	
		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_pShader = matComp.GetShader();
	
		auto srcShaderParams = matComp.GetShaderParams();
		for ( int j = 0; j < srcShaderParams.size(); j++ ) {
			if ( srcShaderParams[j].GetTexture() != nullptr ) {
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetTexture() );
			} else if ( srcShaderParams[j].GetRenderTexture() != nullptr ) {
	
				newShaderParams.SetTexture( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetRenderTexture() );
			} else {
				newShaderParams.SetVec4( srcShaderParams[j].GetParamName().stl_str(), srcShaderParams[j].GetVector() );
			}
		}

		m_ShaderParams.push_back( newShaderParams );
	}
}
