//===================================================================================================
// kbWorldGen.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbNetworkingManager.h"
#include "kbGameEntityHeader.h"
#include "kbJobManager.h"
#include "kbWorldGen.h"
#include "DQuadGame.h"

KB_DEFINE_COMPONENT(kbDQuadWorldGenComponent)

const int g_MaxDynamicVertices = 10000;

/**
 *	terrainChunk_t::terrainChunk_t
 */
terrainChunk_t::terrainChunk_t() :
	m_pTerrainEntity( new kbGameEntity() ),
	m_pTerrainModel( new kbModel() ),
	m_pCollisionMesh( NULL ),
	m_pDynamicCollisionMesh( NULL ),
	m_State( Available ) {
}

/**
 *	terrainChunk_t::Initialize
 */
void terrainChunk_t::Initialize( const int dimensions ) {
	const uint NumVertices = dimensions * dimensions * 6 + g_MaxDynamicVertices;
	const uint NumIndices = NumVertices;
	if ( g_pRenderer != NULL ) {
		m_pTerrainModel->CreateDynamicModel( NumVertices, NumIndices );
	}

	m_pCollisionMesh = new kbVec3[NumVertices];
	m_pDynamicCollisionMesh = new kbVec3[g_MaxDynamicVertices];
}

/**
 *	kbDQuadWorldGenComponent::Constructor
 */
void kbDQuadWorldGenComponent::Constructor() {
	m_TerrainDimensions = 4;
	m_NumTerrainChunks = m_TerrainDimensions * m_TerrainDimensions;
	m_ChunkPoolSize = m_NumTerrainChunks * 2;
	m_ChunkDimensions = 2;
	m_ChunkWorldLength = 8192;
	m_HalfChunkWorldLength = m_ChunkWorldLength / 2;
	m_TerrainGenerationNoiseScale = 0.00004f;
	m_MaxTerrainHeight = 70000.0f;
	m_MaxTerrainCellMidPointHeight = 100.0f;
	m_TerrainChunks = NULL;
	m_SecondsInADay = 600;
	m_DebugHour = -1;
	m_SecondsSinceSpawn = 0.0f;

	if ( g_pGame != NULL ) {
		static_cast<DQuadGame*>( g_pGame )->SetWorldGenComponent( this );
	}
}

/**
 *	kbDQuadWorldGenComponent::~kbDQuadWorldGenComponent
 */
kbDQuadWorldGenComponent::~kbDQuadWorldGenComponent() {
	TearDownWorld();
}

/**
 *	kbDQuadWorldGenComponent::InitializeWorld
 */
void kbDQuadWorldGenComponent::InitializeWorld() {

	TearDownWorld();

	m_NumTerrainChunks = m_TerrainDimensions * m_TerrainDimensions;
	m_ChunkPoolSize = m_NumTerrainChunks * 2;
	m_HalfChunkWorldLength = m_ChunkWorldLength / 2;

	m_TerrainChunks = new terrainChunk_t[m_ChunkPoolSize];
	for ( int i = 0; i < m_ChunkPoolSize; i++ ) {
		m_TerrainChunks[i].Initialize( m_ChunkDimensions );

		m_TerrainChunks[i].m_TerrainJob.m_NumTerrainChunks = m_NumTerrainChunks;
		m_TerrainChunks[i].m_TerrainJob.m_ChunkDimensions = m_ChunkDimensions;
		m_TerrainChunks[i].m_TerrainJob.m_ChunkWorldLength = m_ChunkWorldLength;
		m_TerrainChunks[i].m_TerrainJob.m_TerrainGenerationNoiseScale = m_TerrainGenerationNoiseScale;
		m_TerrainChunks[i].m_TerrainJob.m_MaxTerrainHeight = m_MaxTerrainHeight;
		m_TerrainChunks[i].m_TerrainJob.m_MaxTerrainCellMidPointHeight = m_MaxTerrainCellMidPointHeight;
	}

	g_ResourceManager.GetPackage( "./assets/Packages/EnviroData.kbPkg", false );
}

/**
 *	kbDQuadWorldGenComponent::TearDownWorld
 */
void kbDQuadWorldGenComponent::TearDownWorld() {

	if ( kbNetworkingManager::GetHostType() != HOST_SERVER ) {
		if ( m_TerrainChunks != NULL ) {
			for ( int i = 0; i < m_ChunkPoolSize; i++ ) {
				m_TerrainChunks[i].m_TerrainJob.WaitForJob();		
				g_pRenderer->RemoveRenderObject( m_TerrainChunks[i].m_pTerrainEntity->GetComponent(0) );
				g_pRenderer->RemoveRenderObject( &m_TerrainChunks[i].m_HackDummyComponent );
			}
		}
		m_StreamingChunks.clear();
	}

	delete[] m_TerrainChunks;
	m_TerrainChunks = NULL;
}

/**
 *	kbDQuadWorldGenComponent::RenderSync
 */
void kbDQuadWorldGenComponent::RenderSync() {
	Super::RenderSync();
	
	UpdateWorldStreaming();
}

/**
 *	kbDQuadWorldGenComponent::Update
 */
void kbDQuadWorldGenComponent::Update( const float DT ) {
	Super::Update( DT );
	
	if ( kbNetworkingManager::GetHostType() == HOST_SERVER ) {
		return;
	}

	m_SecondsSinceSpawn += DT;

	kbDirectionalLightComponent * pDirLight = NULL;
	kbFogComponent * pFog = NULL;
	kbStaticModelComponent * pSky = NULL;
	kbDQuadEnviroComponent * pEnviroComp = NULL;
	kbLightShaftsComponent * pLightShafts = NULL;

	if ( m_EnviroInfo.size() != 0 ) {
		const kbGameEntity * EnviroData = m_EnviroInfo[0].m_EnvironmentData.GetEntity();
		if ( EnviroData != NULL ) {
			for ( int iComp = 0; iComp < EnviroData->NumComponents(); iComp++ ) {
				kbComponent *pComp = EnviroData->GetComponent(iComp);
				if ( pComp->IsA( kbDQuadEnviroComponent::GetType() ) == true ) {
					pEnviroComp = static_cast<kbDQuadEnviroComponent*>( pComp );
					break;
	
				}
			}
		}
	}

	kbGameEntity *const pSunEntity = m_SunEntity.GetEntity();
	if ( pSunEntity != NULL ) {
		for ( int i = 0; i < pSunEntity->NumComponents(); i++ ) {
			if ( pSunEntity->GetComponent(i)->IsA( kbDirectionalLightComponent::GetType() ) ) {
				pDirLight = static_cast<kbDirectionalLightComponent*>( pSunEntity->GetComponent(i) );
				break;
			}
		}
	}

	kbGameEntity *const pFogEntity = m_FogEntity.GetEntity();
	if ( pFogEntity != NULL ) {
		for ( int i = 0; i < pFogEntity->NumComponents(); i++ ) {
			if ( pFogEntity->GetComponent(i)->IsA( kbFogComponent::GetType() ) ) {
				pFog = static_cast<kbFogComponent*>( pFogEntity->GetComponent(i) );
				break;
			}
		}
	}

	kbGameEntity *const pSkyEntity = m_SkyEntity.GetEntity();
	if ( pSkyEntity != NULL ) {
		for ( int i = 0; i < pSkyEntity->NumComponents(); i++ ) {
			if ( pSkyEntity->GetComponent(i)->IsA( kbStaticModelComponent::GetType() ) ) {
				pSky = static_cast<kbStaticModelComponent*>( pSkyEntity->GetComponent(i) );
				break;
			}
		}
	}

	kbGameEntity *const pLightShaftEntity = this->m_LightShaftEntity.GetEntity();
	if ( pLightShaftEntity != NULL ) {
		for ( int i = 0; i < pLightShaftEntity->NumComponents(); i++ ) {
			if ( pLightShaftEntity->GetComponent(i)->IsA( kbLightShaftsComponent::GetType() ) ) {
				pLightShafts = static_cast<kbLightShaftsComponent*>( pLightShaftEntity->GetComponent(i) );
				break;
			}
		}
	}

	if ( pEnviroComp ) {

		float currentHour = 0;
		if ( m_DebugHour >= 0 ) {
			currentHour = m_DebugHour;
		} else {
			float mod = fmod( m_SecondsSinceSpawn, m_SecondsInADay );
			float here = mod / m_SecondsInADay;
			currentHour = 24.0f * fmod( m_SecondsSinceSpawn, m_SecondsInADay ) / m_SecondsInADay;

			//kbLog( "		A = %f, b = %f, c = %f", mod, here, currentHour );
		}
		
		for ( int i = 0; i < pEnviroComp->GetTimeOfDayModifiers().size(); i++ ) {
			int nextTimeOfDayIdx = i + 1;
			float nextHour = 0;
			float prevHour = pEnviroComp->GetTimeOfDayModifiers()[i].GetHour();
		
			if ( i + 1 == pEnviroComp->GetTimeOfDayModifiers().size() ) {
				nextTimeOfDayIdx = 0;
				nextHour = pEnviroComp->GetTimeOfDayModifiers()[0].GetHour() + 24.0f;
float pre = currentHour;
				if ( currentHour < prevHour ) {
					currentHour += 24;
				}
//kbLog( "Pre = %f - current = %f", pre, currentHour );
			} else {
				nextHour = pEnviroComp->GetTimeOfDayModifiers()[nextTimeOfDayIdx].GetHour();
			}
		
			const kbDQuadTimeOfDayModifier & PrevTimeOfDayMod = pEnviroComp->GetTimeOfDayModifiers()[i];
			const kbDQuadTimeOfDayModifier & NextTimeOfDayMod = pEnviroComp->GetTimeOfDayModifiers()[nextTimeOfDayIdx];
		
			if ( currentHour >= prevHour && currentHour < nextHour ) {
				
				const float T = ( currentHour - prevHour ) / ( nextHour - prevHour );
				const kbColor SunColor = kbLerp( PrevTimeOfDayMod.GetSunColor(), NextTimeOfDayMod.GetSunColor(), T );
				const kbColor FogColor = kbLerp( PrevTimeOfDayMod.GetFogColor(), NextTimeOfDayMod.GetFogColor(), T );
				const kbColor SkyColor = kbLerp( PrevTimeOfDayMod.GetSkyColor(), NextTimeOfDayMod.GetSkyColor(), T );
				const kbColor LightShaftsColor = kbLerp( PrevTimeOfDayMod.GetLightShaftColor(), NextTimeOfDayMod.GetLightShaftColor(), T );

				if ( pDirLight != NULL ) {

float A = currentHour;
					float normalizedHour = currentHour;
					if ( normalizedHour >= 24.0f ) {
						normalizedHour -= 24.0f;
					}
					float current = normalizedHour;
					if ( normalizedHour >= 6 && normalizedHour <= 18 ) {
						normalizedHour -= 6.0f;
						normalizedHour *= ( 8.0f / 12.0f );
						normalizedHour += 8.0f;
					} else {
						if ( normalizedHour < 6 ) {
							normalizedHour += 24;
						}

						normalizedHour -= 18.0f;
						normalizedHour *= ( 8.0f / 12.0f );
						normalizedHour = 16 - normalizedHour;
					}

					normalizedHour = ( normalizedHour / 24.0f ) * kbPI;
					kbMat4 sunRotationMatrix;
					sunRotationMatrix.MakeIdentity();
					sunRotationMatrix[1].y = cos( normalizedHour );
					sunRotationMatrix[1].z = sin( normalizedHour );
					sunRotationMatrix[2].y = -sin( normalizedHour );
					sunRotationMatrix[2].z = cos( normalizedHour );
					const kbVec3 finalSunDirection = kbVec3( 0.0f, 0.0f, 1.0f ) * sunRotationMatrix;
					const kbVec3 up = finalSunDirection.Cross( kbVec3( 1.0f, 0.0f, 0.0f ) ).Normalized();
					const kbVec3 right = up.Cross( finalSunDirection ).Normalized();
					sunRotationMatrix[0] = right;
					sunRotationMatrix[1] = up;
					sunRotationMatrix[2] = finalSunDirection;

					pDirLight->SetColor( SunColor );
					pDirLight->Enable( false );
					pDirLight->Enable( true );
					pDirLight->GetParent()->SetOrientation( kbQuatFromMatrix( sunRotationMatrix ) );
				}
		
				if ( pFog != NULL ) {
					pFog->SetColor( FogColor );
				}
		
				if ( pSky != NULL ) {
					pSky->SetShaderParam( 0, FogColor );
					pSky->SetShaderParam( 1, SkyColor );
				}

				if ( pLightShafts != NULL ) {

				float normalizedHour = currentHour;
normalizedHour -= 6.0f;
				if ( normalizedHour >= 24.0f ) {
					normalizedHour -= 24.0f;
				} else if ( normalizedHour < 0 ) {
					normalizedHour += 24.0f;
				}
				normalizedHour = ( normalizedHour / 24.0f ) * kbPI * 2.0f;
				kbMat4 sunRotationMatrix;
				sunRotationMatrix.MakeIdentity();
				sunRotationMatrix[1].y = cos( normalizedHour );
				sunRotationMatrix[1].z = sin( normalizedHour );
				sunRotationMatrix[2].y = -sin( normalizedHour );
				sunRotationMatrix[2].z = cos( normalizedHour );
				const kbVec3 finalSunDirection = kbVec3( 0.0f, 0.0f, 1.0f ) * sunRotationMatrix;
				const kbVec3 up = finalSunDirection.Cross( kbVec3( 1.0f, 0.0f, 0.0f ) ).Normalized();
				const kbVec3 right = up.Cross( finalSunDirection ).Normalized();
				sunRotationMatrix[0] = right;
				sunRotationMatrix[1] = up;
				sunRotationMatrix[2] = finalSunDirection;

					pLightShafts->SetColor( LightShaftsColor );
					pLightShafts->GetParent()->SetOrientation( kbQuatFromMatrix( sunRotationMatrix ) );

	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( NULL, currentCameraPosition, currentCameraRotation );

//kbVec3 sunPos = currentCameraPosition - finalSunDirection * 2000.0f;

//g_pRenderer->DrawBox( kbBounds( sunPos - kbVec3( 5, 5, 5 ), sunPos + kbVec3( 5, 5, 5 ) ), kbColor::red );
				}
				break;
			}
		}
	}
}

/**
 *	kbDQuadWorldGenComponent::UpdateWorldStreaming
 */
void kbDQuadWorldGenComponent::UpdateWorldStreaming() {

	// Check for chunks that need to be streamed.
	for ( int i = 0; i < m_StreamingChunks.size(); i++ ) {

		// Only stream chunks if the assets needed have been loaded
		bool bReadyToStreamChunk = true;
		for ( int iResource = 0; iResource < m_StreamingChunks[i]->m_NeededResources.size(); iResource++ ) {
			if ( g_ResourceManager.AsyncLoadResource( m_StreamingChunks[i]->m_NeededResources[iResource] ) == NULL ) {
				bReadyToStreamChunk = false;
				break;
			}
		}

		if ( bReadyToStreamChunk == false ) {
			continue;
		}
		terrainChunk_t *const pCurChunk = m_StreamingChunks[i];

		pCurChunk->m_State = terrainChunk_t::StreamingIn;
		pCurChunk->m_TerrainJob.m_VertexBufferOutput = ( vertexLayout * ) pCurChunk->m_pTerrainModel->MapVertexBuffer();
		pCurChunk->m_TerrainJob.m_IndexBufferOutput = ( unsigned long * ) pCurChunk->m_pTerrainModel->MapIndexBuffer();
		pCurChunk->m_TerrainJob.m_CollisionMeshOutput = pCurChunk->m_pCollisionMesh;
		pCurChunk->m_TerrainJob.m_DynamicCollisionMeshOutput = pCurChunk->m_pDynamicCollisionMesh;

		pCurChunk->m_TerrainJob.m_Position = pCurChunk->m_pTerrainEntity->GetPosition();

		g_pJobManager->RegisterJob( &pCurChunk->m_TerrainJob );

		pCurChunk->m_State = terrainChunk_t::StreamingIn;

		RemoveItemFromVector( m_StreamingChunks, pCurChunk );
		i--;
	}
	
	// Check for finished jobs
	for ( int i = 0; i < m_ChunkPoolSize; i++ ) {
		if ( m_TerrainChunks[i].m_State == terrainChunk_t::StreamingIn && m_TerrainChunks[i].m_TerrainJob.IsJobFinished() ) {

			// Clear job outputs
			m_TerrainChunks[i].m_TerrainJob.m_VertexBufferOutput = NULL;
			m_TerrainChunks[i].m_TerrainJob.m_IndexBufferOutput = NULL;

			m_TerrainChunks[i].m_TerrainJob.m_CollisionMeshOutput = NULL;

			// Update render object
			m_TerrainChunks[i].m_pTerrainModel->UnmapVertexBuffer();
			m_TerrainChunks[i].m_pTerrainModel->UnmapIndexBuffer();

			g_pRenderer->AddRenderObject( m_TerrainChunks[i].m_pTerrainEntity->GetComponent(0), m_TerrainChunks[i].m_pTerrainModel, kbVec3( 0, 0, 0 ), kbQuat( 0, 0, 0, 1.0f ), kbVec3( 1, 1, 1 ) );

			m_TerrainChunks[i].m_State = terrainChunk_t::Visible;

			 //kbLog( "Terrain Streamed in.  Num Dyn Objs = %d, Num Dyn Verts = %d, Num Dyn Indices = %d", m_TerrainChunks[i].m_TerrainJob.m_NumDynamicObjects, m_TerrainChunks[i].m_TerrainJob.m_NumDynamicVertices, m_TerrainChunks[i].m_TerrainJob.m_NumDynamicIndices );
		}
	}

// Temp hack to let renderer create a valid positions
static int hackWait = 0;
hackWait++;
if ( hackWait < 10 )
{
	return;
}
// end hack

	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( NULL, currentCameraPosition, currentCameraRotation );

	kbVec3i gridAlignedPlayerPos( ( ( ( int ) currentCameraPosition.x / ( int ) m_ChunkWorldLength ) * ( int ) m_ChunkWorldLength ), 0, ( ( ( int ) currentCameraPosition.z / ( int ) m_ChunkWorldLength ) * ( int ) m_ChunkWorldLength ) );
	const kbVec3 minStreamingBounds( gridAlignedPlayerPos.x - ( m_HalfChunkWorldLength * m_TerrainDimensions ) - 10.0f, 0.0f, gridAlignedPlayerPos.z - ( m_HalfChunkWorldLength * m_TerrainDimensions ) - 10.0f );
	const kbVec3 maxStreamingBounds( gridAlignedPlayerPos.x + ( m_HalfChunkWorldLength * m_TerrainDimensions ) + 10.0f, 0.0f, gridAlignedPlayerPos.z + ( m_HalfChunkWorldLength * m_TerrainDimensions ) + 10.0f );

	// find chunks that have "streamed out"
	for ( int i = 0; i < m_ChunkPoolSize; i++ ) {
		if ( m_TerrainChunks[i].m_State != terrainChunk_t::Visible ) {
			continue;
		}

		const kbVec3 terrainPos = m_TerrainChunks[i].m_pTerrainEntity->GetPosition();
		if ( terrainPos.x < minStreamingBounds.x || terrainPos.x > maxStreamingBounds.x || terrainPos.z < minStreamingBounds.z || terrainPos.z > maxStreamingBounds.z ) {
			g_pRenderer->RemoveRenderObject( m_TerrainChunks[i].m_pTerrainEntity->GetComponent(0) );
			g_pRenderer->RemoveRenderObject( &m_TerrainChunks[i].m_HackDummyComponent );

			m_TerrainChunks[i].m_State = terrainChunk_t::Available;
		}
	}

	// find chunks to "stream in"
	float currentChunkZ = gridAlignedPlayerPos.z + m_ChunkWorldLength * m_TerrainDimensions / -2.0f;
	for ( int z = 0; z < m_TerrainDimensions; z++, currentChunkZ += m_ChunkWorldLength ) {
	
		float currentChunkX = gridAlignedPlayerPos.x + m_ChunkWorldLength * m_TerrainDimensions / -2.0f;
		for ( int x = 0; x < m_TerrainDimensions; x++, currentChunkX += m_ChunkWorldLength ) {

			// Check if the chunk is streaming or is already loaded
			bool found = false;
			for ( int l = 0; l < m_ChunkPoolSize; l++ ) {

				if ( m_TerrainChunks[l].m_State == terrainChunk_t::Available ) {
					continue;
				}

				const float xDif = abs( m_TerrainChunks[l].m_pTerrainEntity->GetPosition().x - currentChunkX );
				const float zDif = abs( m_TerrainChunks[l].m_pTerrainEntity->GetPosition().z - currentChunkZ );

				if ( xDif < 0.01f && zDif < 0.01f ) {
					found = true;
					break;
				}
			}

			if ( found ) {
				continue;
			}

			// Check our pool for an available chunk
			int chunkIdx = 0;
			for ( ; chunkIdx < m_ChunkPoolSize; chunkIdx++ ) {
				if ( m_TerrainChunks[chunkIdx].m_State == terrainChunk_t::Available ) {
					break;
				}
			}

			if ( chunkIdx >= m_ChunkPoolSize ) {
				kbAssert( false, "Couldn't find available chunk!" );
				return;
			}

			m_TerrainChunks[chunkIdx].m_pTerrainEntity->SetPosition( kbVec3( currentChunkX, 0.0f, currentChunkZ ) );
			m_TerrainChunks[chunkIdx].m_State = terrainChunk_t::WaitingOnEnviroStreaming;
			m_TerrainChunks[chunkIdx].m_NeededResources.clear();
			m_StreamingChunks.push_back( &m_TerrainChunks[chunkIdx] );


			// Get a list of the environment objects that need to be streamed in
			if ( m_EnviroInfo.size() != 0 ) {
				const kbGameEntity * EnviroData = m_EnviroInfo[0].m_EnvironmentData.GetEntity();
				if ( EnviroData != NULL ) {
					for ( int iComp = 0; iComp < EnviroData->NumComponents(); iComp++ ) {
						kbComponent *pComp = EnviroData->GetComponent(iComp);
						if ( pComp->IsA( kbDQuadEnviroComponent::GetType() ) == false ) {
							continue;
						}
						kbDQuadEnviroComponent *const pEnviroComp = static_cast<kbDQuadEnviroComponent*>(pComp);
						m_TerrainChunks[chunkIdx].m_TerrainJob.m_EnviroComponent = pEnviroComp;

						std::vector<kbDQuadEnviroObject> & EnviroObjs = pEnviroComp->m_EnviroObjects;
						if ( EnviroObjs.size() > 0 ) {
							for ( int iModel = 0; iModel < EnviroObjs.size(); iModel++ ) {
								kbModel *const pModel = EnviroObjs[iModel].m_pModel;
								pModel->SetCPUAccessOnly( true );
								if ( g_ResourceManager.AsyncLoadResource( pModel->GetFullName() ) == NULL ) {
									m_TerrainChunks[chunkIdx].m_NeededResources.push_back( pModel->GetFullName() );
								}
							}
						}

						std::vector<kbDQuadEnviroObject> & CoverObjs = pEnviroComp->m_CoverObjects;
						if ( CoverObjs.size() > 0 ) {
							for ( int iModel = 0; iModel < CoverObjs.size(); iModel++ ) {
								kbModel *const pModel = CoverObjs[iModel].m_pModel;
								pModel->SetCPUAccessOnly( true );
								if ( g_ResourceManager.AsyncLoadResource( pModel->GetFullName() ) == NULL ) {
									m_TerrainChunks[chunkIdx].m_NeededResources.push_back( pModel->GetFullName() );
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 *	kbDQuadWorldGenComponent::SetEnable_Internal
 */
void kbDQuadWorldGenComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	TearDownWorld();

	if ( isEnabled ) {
		InitializeWorld();
	}
}

/**
 *	kbDQuadWorldGenComponent::TraceAgainstWorld
 */
bool kbDQuadWorldGenComponent::TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbWorldGenCollision_t & HitInfo, const bool bTraceAgainstDynamicCollision ) const {
	HitInfo.m_bHitFound = false;

	const int NumVerts = ( m_ChunkDimensions * m_ChunkDimensions * 6 );
	const kbVec3 rayDir = ( endPt - startPt );
	float closestT = FLT_MAX;

	for ( int l = 0; l < m_ChunkPoolSize; l++ ) {
	
		const terrainChunk_t & curChunk = m_TerrainChunks[l];

		if ( m_TerrainChunks[l].m_State != terrainChunk_t::Visible ) {
			continue;
		}
	
		const kbVec3 center = m_TerrainChunks[l].m_pTerrainEntity->GetPosition();
		const kbVec3 MaxExtent = center + kbVec3( (float)m_HalfChunkWorldLength, (float)m_MaxTerrainHeight, (float)m_HalfChunkWorldLength );
		const kbVec3 MinExtent = center - kbVec3( (float)m_HalfChunkWorldLength, 0.0f, (float)m_HalfChunkWorldLength );

		if ( startPt.x < MinExtent.x || startPt.x > MaxExtent.x ||
			 startPt.z < MinExtent.z || startPt.z > MaxExtent.z ) {
			continue;
		}

		for ( int iVert = 0; iVert < NumVerts; iVert += 3 ) {
			const kbVec3 & v0 = m_TerrainChunks[l].m_pCollisionMesh[iVert + 0];
			const kbVec3 & v1 = m_TerrainChunks[l].m_pCollisionMesh[iVert + 1];
			const kbVec3 & v2 = m_TerrainChunks[l].m_pCollisionMesh[iVert + 2];

			float t;
			if ( kbRayTriIntersection( startPt, rayDir, v0, v1, v2, t ) || kbRayTriIntersection( endPt, -rayDir, v0, v1, v2, t ) ) {
				if ( t < closestT && t <= 1.0f && t > 0.0f ) {
					HitInfo.m_bHitFound = true;
					HitInfo.m_HitLocation = startPt + rayDir * t;
					closestT = t;
			//	g_pRenderer->DrawSphere( HitInfo.m_HitLocation, 128.0f, 12, kbColor::green );
				}
			}
		}

		if ( bTraceAgainstDynamicCollision ) {
			for ( int iVert = 0; iVert < curChunk.m_TerrainJob.m_NumDynamicIndices; iVert += 3 ) {
				const kbVec3 & v0 = curChunk.m_pDynamicCollisionMesh[iVert + 0];
				const kbVec3 & v1 = curChunk.m_pDynamicCollisionMesh[iVert + 1];
				const kbVec3 & v2 = curChunk.m_pDynamicCollisionMesh[iVert + 2];

				float t;
				if ( kbRayTriIntersection( startPt, rayDir, v0, v1, v2, t ) || kbRayTriIntersection( endPt, -rayDir, v0, v1, v2, t ) ) {
					if ( t < closestT && t <= 1.0f && t > 0.0f ) {
						HitInfo.m_bHitFound = true;
						HitInfo.m_HitLocation = startPt + rayDir * t;
						closestT = t;
				//	g_pRenderer->DrawSphere( HitInfo.m_HitLocation, 128.0f, 12, kbColor::green );
					}
				}
			}
		}

	//	kbBounds bounds( MinExtent, MaxExtent );
	//	g_pRenderer->DrawBox( bounds, kbColor::red );
		//break;
	}


	return HitInfo.m_bHitFound;
}

/**
 *	kbDQuadEnviroInfo::Constructor
 */
void kbDQuadEnviroInfo::Constructor() {
}

/**
 *	kbDQuadEnviroMaterial::Constructor
 */
void kbDQuadEnviroMaterial::Constructor() {
	m_Texture = NULL;
	m_Color.Set( 0.0f, 1.0f, 0.0f, 1.0f );
}

/**
 *	kbDQuadEnviroObject::Constructor
 */
void kbDQuadEnviroObject::Constructor() {
	m_pModel = NULL;
	m_MinScale.Set( 1.0f, 1.0f, 1.0f );
	m_MaxScale.Set( 1.0f, 1.0f, 1.0f );
	m_MinHealth = 10.0f;
	m_MaxHealth = 10.0f;
}

/**
 *	kbDQuadEnviroComponent::Constructor
 */
void kbDQuadEnviroComponent::Constructor() {
}

/**
 *	kbDQuadTimeOfDayModifier::Constructor
 */
void kbDQuadTimeOfDayModifier::Constructor() {
	m_Hour = 0.0f;
	m_SunColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_FogColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_SkyColor.Set( 0.0f, 0.0f, 1.0f, 1.0f );
	m_LightShaftColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
}