//===================================================================================================
// EtherWorldGen.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbIntersectionTests.h"
#include "kbGameEntityHeader.h"
#include "kbJobManager.h"
#include "EtherWorldGen.h"
#include "EtherGame.h"

KB_DEFINE_COMPONENT(EtherWorldGenComponent)

const int g_MaxDynamicVertices = 10000;

kbConsoleVariable g_ShowTerrainCollision( "showterraincollision", false, kbConsoleVariable::Console_Bool, "Shows terrain collision", "" );
kbConsoleVariable g_SetHourOfDay( "sethourofday", -1.0f, kbConsoleVariable::Console_Float, "Sets the hour of the day (0-23)", "" );
kbConsoleVariable g_ShowCoverObjects( "showcoverobjects", false, kbConsoleVariable::Console_Bool, "Draws debug shapes around the cover objects", "" );
kbConsoleVariable g_ProcGenInfo( "procgeninfo", false, kbConsoleVariable::Console_Bool, "Displays info about the procedurally generated world", "" );


/**
 *	EtherCoverObject::EtherCoverObject
 */
EtherCoverObject::EtherCoverObject( const kbBounds & inBounds, const float inHealth ) :
	m_Position( inBounds.Center() ),
	m_Health( inHealth ),
	m_Bounds( inBounds ) {
}

/**
 *	EtherTerrainChunk::EtherTerrainChunk
 */
EtherTerrainChunk::EtherTerrainChunk( const int numTrisPerSide ) :
	m_pTerrainEntity( new kbGameEntity() ),
	m_pTerrainModel( new kbModel() ),
	m_ChunkState( Available ) {

	const uint NumVertices = numTrisPerSide * numTrisPerSide * 6 + g_MaxDynamicVertices;
	const uint NumIndices = NumVertices;
	if ( g_pRenderer != nullptr ) {
		m_pTerrainModel->CreateDynamicModel( NumVertices, NumIndices );
	}
}

/**
 *	EtherTerrainChunk::~EtherTerrainChunk
 */
EtherTerrainChunk::~EtherTerrainChunk() {
	delete m_pTerrainEntity;
	delete m_pTerrainModel;
}

/**
 *	EtherTerrainChunk::MarkAsAvailable
 */
void EtherTerrainChunk::MarkAsAvailable() {
	m_ChunkState = Available;
	m_StaticCollisionMesh.resize( 0 );
	m_DynamicCollisionMesh.resize( 0 );
	m_NeededResources.resize( 0 );

	kbErrorCheck( m_TerrainJob.IsJobFinished(), "EtherTerrainChunk::MarkAsAvailable() - Trying to free up a job that is still running" );
	m_TerrainJob.Reset();

	m_CoverObjects.resize( 0 );
}

/**
 *	EtherWorldGenComponent::Constructor
 */
void EtherWorldGenComponent::Constructor() {
	m_ChunksPerTerrainSide = 4;
	m_TrisPerChunkSide = 2;
	m_ChunkWorldLength = 8192;
	m_HalfChunkWorldLength = m_ChunkWorldLength / 2;
	m_TerrainGenNoiseScale = 0.00004f;
	m_MaxTerrainHeight = 70000.0f;
	m_MaxTerrainCellMidPointHeight = 100.0f;

	m_SecondsInADay = 600;
	m_DebugHour = -1;
	m_SecondsSinceSpawn = 0.0f;
	m_pTerrainShader = nullptr;

	if ( g_pGame != nullptr ) {
		static_cast<EtherGame*>( g_pGame )->SetWorldGenComponent( this );
	}
}

/**
 *	EtherWorldGenComponent::~EtherWorldGenComponent
 */
EtherWorldGenComponent::~EtherWorldGenComponent() {
	TearDownWorld();
}

/**
 *	EtherWorldGenComponent::InitializeWorld
 */
void EtherWorldGenComponent::InitializeWorld() {

	TearDownWorld();

	m_HalfChunkWorldLength = m_ChunkWorldLength / 2;

	const int NumPooledChunks = ( m_ChunksPerTerrainSide + 2 ) * ( m_ChunksPerTerrainSide + 2 );
	m_TerrainChunksPool.reserve( NumPooledChunks );
	for ( int i = 0; i < NumPooledChunks; i++ ) {
		m_TerrainChunksPool.push_back( new EtherTerrainChunk( m_TrisPerChunkSide ) );
	}

	g_ResourceManager.GetPackage( "./assets/Packages/EnviroData.kbPkg", false );

	m_VisibleTerrainMap.Shutdown();

	m_pTerrainShader = (kbShader*)g_ResourceManager.GetResource( "./assets/Shaders/terrain.kbShader", true );
	kbErrorCheck( m_pTerrainShader != nullptr, "EtherWorldGenComponent::InitializeWorld() - Failed to load terrain shader" );
}

/**
 *	EtherWorldGenComponent::TearDownWorld
 */
void EtherWorldGenComponent::TearDownWorld() {

	g_pRenderer->WaitForRenderingToComplete();

	int numChunksFreed = 0;

	for ( int i = 0; i < m_TerrainChunksPool.size(); i++ ) {
		m_TerrainChunksPool[i]->m_TerrainJob.WaitForJob();
		g_pRenderer->RemoveRenderObject( m_TerrainChunksPool[i]->m_pTerrainEntity->GetComponent(0) );
		delete m_TerrainChunksPool[i];
	
		numChunksFreed++;
	}
	
	m_StreamingInChunks.clear();

	EtherChunkArray & TerrainMap = m_VisibleTerrainMap.Get2DMap();
	for ( int i = 0; i < TerrainMap.size(); i++ ) {
		EtherTerrainChunk *const pCurChunk = TerrainMap[i];
		if ( pCurChunk != nullptr ) {
			m_VisibleTerrainMap.RemoveChunk( pCurChunk );
			g_pRenderer->RemoveRenderObject( pCurChunk->m_pTerrainEntity->GetComponent(0) );
			delete pCurChunk;
			numChunksFreed++;
		}
	}

	const int NumPooledChunks = ( m_ChunksPerTerrainSide + 2 ) * ( m_ChunksPerTerrainSide + 2 );
	kbErrorCheck( numChunksFreed == 0 || numChunksFreed == NumPooledChunks, "EtherWorldGenComponent::TearDownWorld() - All Terrain chunks didn't make it back into the pool" );

	m_TerrainChunksPool.resize( 0 );
}

/**
 *	EtherWorldGenComponent::RenderSync
 */
void EtherWorldGenComponent::RenderSync() {
	Super::RenderSync();
	
	RenderSyncStreaming();
}

/**
 *	EtherWorldGenComponent::Update_Internal
 */
void EtherWorldGenComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	if ( g_SetHourOfDay.GetFloat() >= 0.0f ) {
		m_SecondsSinceSpawn = ( g_SetHourOfDay.GetFloat() / 24.0f ) * m_SecondsInADay;
	} else if ( m_DebugHour > 0.0f ) {
		m_SecondsSinceSpawn = ( m_DebugHour / 24.0f ) * m_SecondsInADay;
	} else {
		m_SecondsSinceSpawn += DT;
	}

	UpdateTimeOfDayFX();
	UpdateDebug();
}

/**
 *	EtherWorldGenComponent::UpdateTimeOfDayFX
 */
void EtherWorldGenComponent::UpdateTimeOfDayFX() {

	kbDirectionalLightComponent * pDirLight = nullptr;
	kbFogComponent * pFog = nullptr;
	kbStaticModelComponent * pSky = nullptr;
	EtherEnviroComponent * pEnviroComp = nullptr;
	kbLightShaftsComponent * pLightShafts = nullptr;

	if ( m_EnviroInfo.size() != 0 ) {
		const kbGameEntity * EnviroData = m_EnviroInfo[0].m_EnvironmentData.GetEntity();
		if ( EnviroData != nullptr ) {
			pEnviroComp = static_cast<EtherEnviroComponent*>( EnviroData->GetComponentByType( EtherEnviroComponent::GetType() ) );
		}
	}

	if ( m_SunEntity.GetEntity() != nullptr ) {
		pDirLight = static_cast<kbDirectionalLightComponent*>( m_SunEntity.GetEntity()->GetComponentByType( kbDirectionalLightComponent::GetType() ) );
	}

	if ( m_FogEntity.GetEntity() != nullptr ) {
		pFog = static_cast<kbFogComponent*>( m_FogEntity.GetEntity()->GetComponentByType( kbFogComponent::GetType() ) );
	}

	if ( m_SkyEntity.GetEntity() != nullptr ) {
		pSky = static_cast<kbStaticModelComponent*>( m_SkyEntity.GetEntity()->GetComponentByType( kbStaticModelComponent::GetType() ) );
	}

	if ( m_LightShaftEntity.GetEntity() != nullptr ) {
		pLightShafts = static_cast<kbLightShaftsComponent*>( m_LightShaftEntity.GetEntity()->GetComponentByType( kbLightShaftsComponent::GetType() ) );
	}

	if ( pEnviroComp != nullptr ) {

		float currentHour = 24.0f * fmod( m_SecondsSinceSpawn, m_SecondsInADay ) / m_SecondsInADay;
		
		for ( int i = 0; i < pEnviroComp->GetTimeOfDayModifiers().size(); i++ ) {
			int nextTimeOfDayIdx = i + 1;
			float nextHour = 0;
			float prevHour = pEnviroComp->GetTimeOfDayModifiers()[i].GetHour();
		
			if ( i + 1 == pEnviroComp->GetTimeOfDayModifiers().size() ) {
				// At the end of the modifier, the next modifier is at the beginning of the list
				nextTimeOfDayIdx = 0;
				nextHour = pEnviroComp->GetTimeOfDayModifiers()[0].GetHour() + 24.0f;

				if ( currentHour < prevHour ) {
					currentHour += 24;
				}

			} else {
				nextHour = pEnviroComp->GetTimeOfDayModifiers()[nextTimeOfDayIdx].GetHour();
			}
		
			if ( currentHour < prevHour || currentHour >= nextHour ) {
				continue;
			}

			const EtherTimeOfDayModifier & PrevTimeOfDayMod = pEnviroComp->GetTimeOfDayModifiers()[i];
			const EtherTimeOfDayModifier & NextTimeOfDayMod = pEnviroComp->GetTimeOfDayModifiers()[nextTimeOfDayIdx];
		
			const float T = ( currentHour - prevHour ) / ( nextHour - prevHour );
			const kbColor SunColor = kbLerp( PrevTimeOfDayMod.GetSunColor(), NextTimeOfDayMod.GetSunColor(), T );
			const kbColor FogColor = kbLerp( PrevTimeOfDayMod.GetFogColor(), NextTimeOfDayMod.GetFogColor(), T );
			const kbColor SkyColor = kbLerp( PrevTimeOfDayMod.GetSkyColor(), NextTimeOfDayMod.GetSkyColor(), T );
			const kbColor LightShaftsColor = kbLerp( PrevTimeOfDayMod.GetLightShaftColor(), NextTimeOfDayMod.GetLightShaftColor(), T );

			if ( pDirLight != nullptr ) {

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
		
			if ( pFog != nullptr ) {
				pFog->SetColor( FogColor );
			}
		
			if ( pSky != nullptr ) {
				pSky->SetShaderParam( 0, FogColor );
				pSky->SetShaderParam( 1, SkyColor );
			}

			if ( pLightShafts != nullptr ) {

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
			}
			break;
		}
	}
}

/**
 *	EtherWorldGenComponent::UpdateDebug
 */
void EtherWorldGenComponent::UpdateDebug() {

	const EtherChunkArray & TerrainMap = m_VisibleTerrainMap.Get2DMap();

	if ( g_ShowTerrainCollision.GetBool() ) {

		// Draw terrain collision
		kbVec3 currentCameraPosition;
		kbQuat currentCameraRotation;
		g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );

		for ( int l = 0; l < TerrainMap.size(); l++ ) {
	
			const EtherTerrainChunk *const pCurChunk = TerrainMap[l];
			if ( pCurChunk == nullptr ) {
				continue;
			}

			if ( pCurChunk->m_ChunkState != EtherTerrainChunk::Visible ) {
				continue;
			}

			for ( int iVert = 0; iVert < pCurChunk->m_StaticCollisionMesh.size(); iVert += 3 ) {
				const kbVec3 & v0 = pCurChunk->m_StaticCollisionMesh[iVert + 0];
				const kbVec3 & v1 = pCurChunk->m_StaticCollisionMesh[iVert + 1];
				const kbVec3 & v2 = pCurChunk->m_StaticCollisionMesh[iVert + 2];
				g_pRenderer->DrawLine( v0, v1, kbColor::red );
				g_pRenderer->DrawLine( v1, v2, kbColor::red );
				g_pRenderer->DrawLine( v0, v2, kbColor::red );
			}

			for ( int iVert = 0; iVert < pCurChunk->m_DynamicCollisionMesh.size(); iVert += 3 ) {
				const kbVec3 & v0 = pCurChunk->m_DynamicCollisionMesh[iVert + 0];
				const kbVec3 & v1 = pCurChunk->m_DynamicCollisionMesh[iVert + 1];
				const kbVec3 & v2 = pCurChunk->m_DynamicCollisionMesh[iVert + 2];
				const kbVec3 center = ( v0 + v1+ v2 ) / 3;
				if ( ( center - currentCameraPosition ).Length() > 400.0f ) {
					continue;
				}
				g_pRenderer->DrawLine( v0, v1, kbColor::red );
				g_pRenderer->DrawLine( v1, v2, kbColor::red );
				g_pRenderer->DrawLine( v0, v2, kbColor::red );
			}
		}
	}

	if ( g_ShowCoverObjects.GetBool() ) {

		// Draw the bounds of the cover objects
		for ( int l = 0; l < TerrainMap.size(); l++ ) {

			const EtherTerrainChunk *const pCurChunk = TerrainMap[l];
			if ( pCurChunk == nullptr ) {
				continue;
			}

			if ( pCurChunk->m_ChunkState != EtherTerrainChunk::Visible ) {
				continue;
			}

			for ( int iCover = 0; iCover < pCurChunk->m_CoverObjects.size(); iCover++ ) {
				const EtherCoverObject & CoverObj = pCurChunk->m_CoverObjects[iCover];
				g_pRenderer->DrawBox( CoverObj.GetBounds(), kbColor::red );
			}
		}
	}

	if ( g_ProcGenInfo.GetBool() ) {

		// Debug proc gen info
		int numStreamedInTerrain = 0;
		EtherChunkArray & TerrainMap = m_VisibleTerrainMap.Get2DMap();
		for ( int i = 0; i < TerrainMap.size(); i++ ) {
			if ( TerrainMap[i] != nullptr ) {
				numStreamedInTerrain++;
			}
		}

		kbVec3 cameraPos;
		kbQuat cameraRot;
		g_pRenderer->GetRenderViewTransform( nullptr, cameraPos, cameraRot );

		const kbVec3 gridAlignedPos = GetTerrainAlignedPos( cameraPos );
		g_pRenderer->DrawDebugText( "Terrain Aligned Pos - x: " + std::to_string( gridAlignedPos.x ) + " z: " + std::to_string( (long double ) gridAlignedPos.z ), 0, 0.1f, g_DebugTextSize, 0.1f, kbColor::green );
		g_pRenderer->DrawDebugText( "Pool Chunks Left " + std::to_string( m_TerrainChunksPool.size() ), 0, 0.1f + g_DebugTextSize * 1.0f,g_DebugTextSize, 0.1f, kbColor::green );
		g_pRenderer->DrawDebugText( "Num Streamed in Terrain: " + std::to_string( numStreamedInTerrain ), 0, 0.1f + g_DebugTextSize * 2.01f, g_DebugTextSize, 0.1f, kbColor::green );

		const kbVec3 ChunkCornerExtent = kbVec3( m_HalfChunkWorldLength * m_ChunksPerTerrainSide, 0.0f, m_HalfChunkWorldLength * m_ChunksPerTerrainSide );
		const kbVec3 LowerLeftCornerPos = gridAlignedPos - ChunkCornerExtent;
		const int LowerLeftArrayIndex = m_VisibleTerrainMap.GetChunkIndexFromPosition( LowerLeftCornerPos );
		g_pRenderer->DrawDebugText( "Lower Left Corner Visible Terrain Map Idx = " + std::to_string( LowerLeftArrayIndex ), 0, 0.1f + g_DebugTextSize * 3.0f, g_DebugTextSize, 0.1f, kbColor::green );

		const float cellWidth = 0.05f;
		const float spacing = g_DebugTextSize;
		const float ySpacing = g_DebugTextSize * 0.86f;
		float startY = 0.6f;
		float startX = 0.8f;

		const int playerChunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( cameraPos );
		const int playerX_idx = playerChunkIdx % m_VisibleTerrainMap.Get2DMapDimensions();
		const int playerY_idx = playerChunkIdx / m_VisibleTerrainMap.Get2DMapDimensions();

		for ( int y = 0; y < m_VisibleTerrainMap.Get2DMapDimensions(); y++ ) {
			std::string line;

			for ( int x = 0; x < m_VisibleTerrainMap.Get2DMapDimensions(); x++ ) {

				int idx = x + ( y * m_VisibleTerrainMap.Get2DMapDimensions() );
				if ( TerrainMap[idx] == nullptr ) {
					line += "O";
				} else {

					if ( x == playerX_idx && y == playerY_idx ) {
						line += "P";
					} else {
						line += "+";
					}
					const kbVec3 chunkPos = TerrainMap[idx]->m_pTerrainEntity->GetPosition();
					g_pRenderer->DrawBox( kbBounds( chunkPos - kbVec3( 15.0f, 15.0f, 15.0f ), chunkPos + kbVec3( 15.0f, 15.0f, 15.0f ) ), kbColor::red );
					g_pRenderer->DrawLine( chunkPos + kbVec3( 0.0f, 15.0f, 0.0f ), chunkPos + kbVec3( 0.0f, 15.0f, 7.5f ), kbColor::green );

					g_pRenderer->DrawLine( chunkPos + kbVec3( -m_HalfChunkWorldLength, 1.0f, m_HalfChunkWorldLength ), chunkPos + kbVec3( m_HalfChunkWorldLength, 1.0f, m_HalfChunkWorldLength ), kbColor::green );
					g_pRenderer->DrawLine( chunkPos + kbVec3( -m_HalfChunkWorldLength, 1.0f, -m_HalfChunkWorldLength ), chunkPos + kbVec3( m_HalfChunkWorldLength, 1.0f, -m_HalfChunkWorldLength ), kbColor::green );
					g_pRenderer->DrawLine( chunkPos + kbVec3( -m_HalfChunkWorldLength, 1.0f, -m_HalfChunkWorldLength ), chunkPos + kbVec3( -m_HalfChunkWorldLength, 1.0f, m_HalfChunkWorldLength ), kbColor::green );
					g_pRenderer->DrawLine( chunkPos + kbVec3( m_HalfChunkWorldLength, 1.0f, -m_HalfChunkWorldLength ), chunkPos + kbVec3( m_HalfChunkWorldLength, 1.0f, m_HalfChunkWorldLength ), kbColor::green );
				}
			}
			
			kbVec3 currentCameraPosition;
			kbQuat currentCameraRotation;
			g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );

			g_pRenderer->DrawDebugText( line, startX, startY - ySpacing * y, spacing, 0.1f, kbColor::red );
		}
	}
}

/**
 *	EtherWorldGenComponent::GetTerrainAlignedPos
 */
kbVec3 EtherWorldGenComponent::GetTerrainAlignedPos( const kbVec3 & worldPosition ) const {

	kbVec3 normalizedTerrainPos = worldPosition / m_ChunkWorldLength;
	normalizedTerrainPos.x = floor( normalizedTerrainPos.x );		// Clamp to int boundaries
	normalizedTerrainPos.z = floor( normalizedTerrainPos.z );		// 

	return kbVec3( m_HalfChunkWorldLength, 0.0f, m_HalfChunkWorldLength ) + kbVec3( normalizedTerrainPos.x, 0.0f, normalizedTerrainPos.z ) * m_ChunkWorldLength;
}

/**
 *	EtherWorldGenComponent::RenderSyncStreaming
 */
void EtherWorldGenComponent::RenderSyncStreaming() {

	// Hack to let renderer create valid positions
	static int hackWait = 0;
	hackWait++;
	if ( hackWait < 10 )
	{
		return;
	}
	// end hack

	if ( m_TerrainChunksPool.size() == 0 ) {
		return;
	}

	std::vector<kbShader *> ShaderOverrideList;
	ShaderOverrideList.push_back( m_pTerrainShader );
	m_pTerrainShader->CommitShaderParams();

	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );

	const kbVec3 terrainAlignedPlayerPos = GetTerrainAlignedPos( currentCameraPosition );
	const kbVec3 terrainAligned_LL_PlayerPos = terrainAlignedPlayerPos - kbVec3( m_HalfChunkWorldLength, 0.0f, m_HalfChunkWorldLength );

	const kbVec3 ChunkCornerExtent = kbVec3( m_HalfChunkWorldLength * m_ChunksPerTerrainSide, 0.0f, m_HalfChunkWorldLength * m_ChunksPerTerrainSide );
	const kbVec3 LL_CornerPos = terrainAligned_LL_PlayerPos - ChunkCornerExtent;
	const kbVec3 UR_CornerPos = terrainAligned_LL_PlayerPos + ChunkCornerExtent;

	const float TerrainStreamFudge = ( m_HalfChunkWorldLength * 0.25f );
	const kbVec3 LowerLeftStreamOutBounds = LL_CornerPos;
	const kbVec3 UpperRightStreamOutBounds = UR_CornerPos;

	const kbVec3 LowerLeftStreamInBounds = LL_CornerPos + m_HalfChunkWorldLength - TerrainStreamFudge;
	const kbVec3 UpperRightStreamInBounds = UR_CornerPos - m_HalfChunkWorldLength + TerrainStreamFudge;

	// Update the Visible Terrain Map if player has moved far enough
	if ( m_VisibleTerrainMap.IsInitialized() == false ) {
		m_VisibleTerrainMap.Initialize( currentCameraPosition, m_ChunkWorldLength, m_ChunksPerTerrainSide );
	} else {
		m_VisibleTerrainMap.Update( terrainAlignedPlayerPos );
	}

	// Iterate over streaming array and update status
	for ( int streamingChunkIdx = (int)m_StreamingInChunks.size() - 1; streamingChunkIdx >= 0; streamingChunkIdx-- ) {
		EtherTerrainChunk *const pCurChunk = m_StreamingInChunks[streamingChunkIdx];

		if ( pCurChunk->m_ChunkState == EtherTerrainChunk::LoadingResources ) {

			bool bReadyToStreamChunk = true;
			for ( int iResource = 0; iResource < pCurChunk->m_NeededResources.size(); iResource++ ) {
				if ( g_ResourceManager.AsyncLoadResource( pCurChunk->m_NeededResources[iResource] ) == nullptr ) {
					bReadyToStreamChunk = false;
					break;
				}
			}

			if ( bReadyToStreamChunk == false ) {
				continue;
			}

			// All needed resources are loaded.  This chunk is ready for proc gen
			pCurChunk->m_ChunkState = EtherTerrainChunk::StreamingIn;
			pCurChunk->m_TerrainJob.m_VertexBufferOutput = ( vertexLayout * ) pCurChunk->m_pTerrainModel->MapVertexBuffer();
			pCurChunk->m_TerrainJob.m_IndexBufferOutput = ( unsigned long * ) pCurChunk->m_pTerrainModel->MapIndexBuffer();
			pCurChunk->m_TerrainJob.m_CollisionMeshOutput = &pCurChunk->m_StaticCollisionMesh;
			pCurChunk->m_TerrainJob.m_DynamicCollisionMeshOutput = &pCurChunk->m_DynamicCollisionMesh;
			pCurChunk->m_TerrainJob.m_pCoverObjects = &pCurChunk->m_CoverObjects;
			pCurChunk->m_TerrainJob.m_Position = pCurChunk->m_pTerrainEntity->GetPosition();
			pCurChunk->m_TerrainJob.m_TrisPerChunkSide = m_TrisPerChunkSide;
			pCurChunk->m_TerrainJob.m_ChunkWorldLength = m_ChunkWorldLength;
			pCurChunk->m_TerrainJob.m_TerrainGenNoiseScale = m_TerrainGenNoiseScale;
			pCurChunk->m_TerrainJob.m_MaxTerrainHeight = m_MaxTerrainHeight;
			pCurChunk->m_TerrainJob.m_MaxTerrainCellMidPointHeight = m_MaxTerrainCellMidPointHeight;

			g_pJobManager->RegisterJob( &pCurChunk->m_TerrainJob );

			continue;

		} else if ( pCurChunk->m_ChunkState == EtherTerrainChunk::StreamingIn ) {

			if ( pCurChunk->m_TerrainJob.IsJobFinished() == false ) {
				continue;
			}

			// Chunk is finished streaming.  Add it to the renderer and Visible Terrain Map
			pCurChunk->m_ChunkState = EtherTerrainChunk::Visible;
			pCurChunk->m_pTerrainModel->UnmapVertexBuffer();
			pCurChunk->m_pTerrainModel->UnmapIndexBuffer();

			m_StreamingInChunks[streamingChunkIdx] = m_StreamingInChunks[m_StreamingInChunks.size() - 1];
			m_StreamingInChunks.pop_back();

			const kbVec3 curChunkPos = pCurChunk->m_pTerrainEntity->GetPosition();
			if ( curChunkPos.x < LowerLeftStreamInBounds.x || curChunkPos.z < LowerLeftStreamOutBounds.z ||
				 curChunkPos.x > UpperRightStreamOutBounds.x || curChunkPos.z > UpperRightStreamOutBounds.z ) {
				pCurChunk->MarkAsAvailable();
				m_TerrainChunksPool.push_back( pCurChunk );
				continue;
			}

			g_pRenderer->AddRenderObject( pCurChunk->m_pTerrainEntity->GetComponent(0), pCurChunk->m_pTerrainModel, kbVec3( 0, 0, 0 ), kbQuat( 0, 0, 0, 1.0f ), kbVec3( 1, 1, 1 ), RP_Lighting, &ShaderOverrideList );
			m_VisibleTerrainMap.AddChunk( pCurChunk );
		} else {
			kbError( "EtherWorldGenComponent::RenderSyncStreaming() - Invalid chunk state while iterating over m_StreamingInChunks" );
		}
	}

	const int TerrainMapDimensions = m_VisibleTerrainMap.Get2DMapDimensions();
	const int LowerLeftX = m_VisibleTerrainMap.GetLowerLeftIdx() % TerrainMapDimensions;
	const int LowerLeftZ = m_VisibleTerrainMap.GetLowerLeftIdx() / TerrainMapDimensions;

	// Iterate over visible chunk map and determine if chunks need to be streamed in or out
	std::vector<EtherTerrainChunk *> & TerrainMap = m_VisibleTerrainMap.Get2DMap();
	for ( int z = 0; z < TerrainMapDimensions; z++ ) {
		for ( int x = 0; x < TerrainMapDimensions; x++ ) {

			const int chunkIdx = x + ( z * TerrainMapDimensions );
			const int adjustedZ = ( z < LowerLeftZ ) ? ( z + TerrainMapDimensions ) : ( z );
			const int adjustedX = ( x < LowerLeftX ) ? ( x + TerrainMapDimensions ) : ( x );

			const int xDif = adjustedX - LowerLeftX;
			const int zDif = adjustedZ - LowerLeftZ;
			const kbVec3 LLToDesiredChunkVec = kbVec3( ( xDif * m_ChunkWorldLength ) + m_HalfChunkWorldLength, 0.0f, ( zDif * m_ChunkWorldLength ) + m_HalfChunkWorldLength );
			const kbVec3 desiredChunkPosition =  kbVec3( m_VisibleTerrainMap.GetLowerLeftPos().x + LLToDesiredChunkVec.x, 0.0f, m_VisibleTerrainMap.GetLowerLeftPos().z + LLToDesiredChunkVec.z );
			const int desiredIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( desiredChunkPosition );

			EtherTerrainChunk * pCurChunk = TerrainMap[chunkIdx];

			// Stream out
			if ( pCurChunk != nullptr ) {
				const kbVec3 curChunkPos = pCurChunk->m_pTerrainEntity->GetPosition();
				if ( curChunkPos.Compare( desiredChunkPosition, TerrainStreamFudge ) == false ||					// Check if the chunks pos doesn't match the desired pos (eg. player teleported to a distance location )
					 curChunkPos.x < LowerLeftStreamOutBounds.x || curChunkPos.z < LowerLeftStreamOutBounds.z ||	// Check if the chunk pos is outside of the streaming bounds
					 curChunkPos.x > UpperRightStreamOutBounds.x || curChunkPos.z > UpperRightStreamOutBounds.z ) {

						g_pRenderer->RemoveRenderObject( pCurChunk->m_pTerrainEntity->GetComponent(0) );
						m_VisibleTerrainMap.RemoveChunk( pCurChunk );
						pCurChunk->MarkAsAvailable();
						m_TerrainChunksPool.push_back( pCurChunk );
						TerrainMap[chunkIdx] = nullptr;
						continue;
				}
			} 

			// Stream in
			if ( desiredChunkPosition.x > LowerLeftStreamInBounds.x && desiredChunkPosition.z > LowerLeftStreamInBounds.z &&
				desiredChunkPosition.x < UpperRightStreamInBounds.x && desiredChunkPosition.z < UpperRightStreamInBounds.z ) {

				// Check if the proper chunk is already at the correct index				
				if ( TerrainMap[desiredIdx] != nullptr && TerrainMap[desiredIdx]->m_pTerrainEntity->GetPosition().Compare( desiredChunkPosition,TerrainStreamFudge ) ) {
					continue;
				} 

				bool chunkAlreadyStreaming = false;
				for ( int streamingChunkIdx = 0; streamingChunkIdx < m_StreamingInChunks.size(); streamingChunkIdx++ ) {
					if ( m_StreamingInChunks[streamingChunkIdx]->m_pTerrainEntity->GetPosition().Compare( desiredChunkPosition, 1.0f ) ) {
						chunkAlreadyStreaming = true;
					}
				}

				if ( chunkAlreadyStreaming == true ) {
					continue;
				}

				kbErrorCheck( m_TerrainChunksPool.size() >= 1, "EtherWorldGenComponent::RenderSyncStreaming() - Terrain chunk pool is empty" );
				pCurChunk = m_TerrainChunksPool[m_TerrainChunksPool.size() - 1];
				m_TerrainChunksPool.pop_back();

				pCurChunk->m_ChunkState = EtherTerrainChunk::LoadingResources;
				pCurChunk->m_pTerrainEntity->SetPosition( desiredChunkPosition );
				m_StreamingInChunks.push_back( pCurChunk );

				// Get a list of environment object resources that need to be streamed in
				if ( m_EnviroInfo.size() == 0 ) {
					continue;
				}

				const kbGameEntity *const EnviroData = m_EnviroInfo[0].m_EnvironmentData.GetEntity();
				if ( EnviroData == nullptr ) {
					continue;
				}

				EtherEnviroComponent *const pEnviroComp = static_cast<EtherEnviroComponent*>( EnviroData->GetComponentByType( EtherEnviroComponent::GetType() ) );
				if ( pEnviroComp == nullptr ) {
					continue;
				}

				pCurChunk->m_TerrainJob.m_EnviroComponent = pEnviroComp;

				std::vector<EtherEnviroObject> & EnviroObjs = pEnviroComp->m_EnviroObjects;
				if ( EnviroObjs.size() > 0 ) {
					for ( int iModel = 0; iModel < EnviroObjs.size(); iModel++ ) {
						kbModel *const pModel = EnviroObjs[iModel].m_pModel;
						pModel->SetCPUAccessOnly( true );
						if ( g_ResourceManager.AsyncLoadResource( pModel->GetFullName() ) == nullptr ) {
							pCurChunk->m_NeededResources.push_back( pModel->GetFullName() );
						}
					}
				}
				
				std::vector<EtherEnviroObject> & CoverObjs = pEnviroComp->m_CoverObjects;
				if ( CoverObjs.size() > 0 ) {
					for ( int iModel = 0; iModel < CoverObjs.size(); iModel++ ) {
						kbModel *const pModel = CoverObjs[iModel].m_pModel;
						pModel->SetCPUAccessOnly( true );
						if ( g_ResourceManager.AsyncLoadResource( pModel->GetFullName() ) == nullptr ) {
							pCurChunk->m_NeededResources.push_back( pModel->GetFullName() );
						}
					}
				}
			}	// End of streamed in
		}	// End X loop
	}	// End Z Loop
}

/**
 *	EtherWorldGenComponent::SetEnable_Internal
 */
void EtherWorldGenComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	TearDownWorld();

	if ( isEnabled ) {
		InitializeWorld();
	}
}

/**
 *	EtherWorldGenComponent::TraceAgainstWorld
 */
bool EtherWorldGenComponent::TraceAgainstWorld( EtherWorldGenCollision_t & OutHitInfo, const kbVec3 & startPt, const kbVec3 & endPt, const bool bTraceAgainstDynamicCollision ) const {

	OutHitInfo.m_bHitFound = false;
	OutHitInfo.m_DistFromOrigin = FLT_MAX;

	if ( m_VisibleTerrainMap.IsInitialized() == false ) {
		return false;
	}

	kbVec3 cameraPos;
	kbQuat cameraRot;
	g_pRenderer->GetRenderViewTransform( nullptr, cameraPos, cameraRot );
		
	const kbVec3 gridAlignedPos = GetTerrainAlignedPos( cameraPos );
	const kbVec3 LowerLeftCornerPos = gridAlignedPos - kbVec3( m_HalfChunkWorldLength, 0.0f, m_HalfChunkWorldLength ) - kbVec3( m_HalfChunkWorldLength * m_ChunksPerTerrainSide, 0.0f, m_HalfChunkWorldLength * m_ChunksPerTerrainSide );
	kbVec3 Dir = endPt - startPt;
	const float TraceLen = Dir.Normalize();

	TraceAgainstTerrain_Recurse( OutHitInfo, startPt, Dir, TraceLen, bTraceAgainstDynamicCollision, kbVec2( LowerLeftCornerPos.x, LowerLeftCornerPos.z ), m_ChunkWorldLength * m_ChunksPerTerrainSide );

	if ( OutHitInfo.m_bHitFound ) {
		OutHitInfo.m_HitLocation = startPt + Dir * TraceLen * OutHitInfo.m_DistFromOrigin;
		OutHitInfo.m_bHitFound = true;

		return true;
	}

	return false;
}

// Ray-AABB test from Real-Time Rendering by Tomas Akenine-Möller and Eric Haines
template<class T>
bool IntersectsAABB( float & outT, const T & rayOrigin, const T & rayDir,const T & minExtent, const T maxExtent ) {
	float tMin = -FLT_MAX;
	float tMax = FLT_MAX;
	const T boxCenter = ( maxExtent + minExtent ) * 0.5f;
	const T p = boxCenter - rayOrigin;

	const int Dimensions = sizeof(T)/sizeof(float);
	bool bInside = true;

	for ( int i = 0; i < Dimensions; i++ ) {
		if ( rayOrigin[i] < minExtent[i] || rayOrigin[i] > maxExtent[i] ) {
			bInside = false;
			break;
		}
	}

	if ( bInside ) {
		outT = 0.0f;
		return true;
	}

	const T halfLengths = maxExtent - boxCenter;
	for ( int i = 0; i < Dimensions; i++ ) {
		const float e = p[i];
		const float f = rayDir[i];
		const float h = halfLengths[i];
		if ( abs( (double) f ) > 0.00000001f ) {
			float t1 = ( e + h ) / f;
			float t2 = ( e - h ) / f;
			if ( t1 > t2 ) { std::swap( t1, t2 ); }
			if ( t1 > tMin ) { tMin = t1; }
			if ( t2 < tMax ) { tMax = t2; }
			if ( tMin > tMax ) { return false; }
			if ( tMax < 0 ) { return false; }			
		} else if ( -e - h > 0 || - e + h < 0 ) {
			return false; 
		}
	}

	if ( tMin > 0 ) {
 		outT = tMin;
		return true;
	}

	outT = tMax;
	return true;
}

/**
 *	EtherWorldGenComponent::MoveActorAlongGround
 */
void EtherWorldGenComponent::MoveActorAlongGround( EtherActorComponent *const pActor, const kbVec3 & StartPt, const kbVec3 & EndPt ) const {

	if ( m_VisibleTerrainMap.IsInitialized() == false ) {
		return;
	}

	std::vector<const EtherTerrainChunk*> ChunkList;
	static float spread = 32.0f;

	const kbVec3 offsets[] = { kbVec3( spread, 0.0f, spread ), kbVec3( -spread, 0.0f, spread ), kbVec3( -spread, 0.0f, -spread ), kbVec3( spread, 0.0f, -spread ) };

	for ( int i = 0; i < 4; i++ ) {
		const int StartChunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( StartPt + offsets[i] );
		if ( StartChunkIdx >= 0 && StartChunkIdx < m_VisibleTerrainMap.Get2DMap().size() ) {
			const EtherTerrainChunk *const pStartChunk = m_VisibleTerrainMap.Get2DMap()[StartChunkIdx];
			if ( pStartChunk != nullptr && pStartChunk->m_ChunkState == EtherTerrainChunk::Visible ) {
				if ( VectorFind( ChunkList, pStartChunk ) == false ) {
					ChunkList.push_back( pStartChunk );
				}
			}
		}

		const int EndChunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( EndPt + offsets[i] );
		if ( EndChunkIdx >= 0 && EndChunkIdx < m_VisibleTerrainMap.Get2DMap().size() ) {
			const EtherTerrainChunk *const pEndChunk = m_VisibleTerrainMap.Get2DMap()[StartChunkIdx];
			if ( pEndChunk != nullptr && pEndChunk->m_ChunkState == EtherTerrainChunk::Visible ) {
				if ( VectorFind( ChunkList, pEndChunk ) == false ) {
					ChunkList.push_back( pEndChunk );
				}
			}
		}
	}

	if ( ChunkList.size() == 0 ) {
		return;
	}

	float T = FLT_MAX;
	bool bFoundHit = false;
	const kbVec3 moveVec = ( EndPt - StartPt );
	const float moveDist = moveVec.Length();
	const kbVec3 moveDir = moveVec / moveDist;
	const EtherCoverObject * pHitCoverObj = nullptr;

	for ( int iChunk = 0; iChunk < ChunkList.size() && bFoundHit == false; iChunk++ ) {
		const EtherTerrainChunk *const pChunk = ChunkList[iChunk];

		for ( int iCover = 0; iCover < pChunk->m_CoverObjects.size() && bFoundHit == false; iCover++ ) {
			const EtherCoverObject & CoverObj = pChunk->m_CoverObjects[iCover];
			const kbBounds & CoverBounds = CoverObj.GetBounds();
		
			float box_T;
			kbVec3 hitPoint;

			bool oppositeDir = ( moveVec.Dot( ( CoverBounds.Center() - StartPt ).Normalized() ) < 0.001f );
			if ( oppositeDir == false && IntersectsAABB( box_T, StartPt, moveDir, CoverBounds.Min(), CoverBounds.Max() ) ) {//IntersectsAABB_3D( hitPoint, StartPt, moveDir, CoverBounds ) ) {

				if ( box_T < T && box_T <= moveDist ) {
					T = box_T;
					bFoundHit = true;
					pHitCoverObj = &CoverObj;
				}

				// g_pRenderer->DrawBox( CoverBounds, kbColor::blue );
				// IntersectsAABB_3D( hitPoint, StartPt, moveDir, CoverBounds );
				// if ( pActor == nullptr )  g_pRenderer->DrawDebugText( "Collision Detected.  T is " + std::to_string( (long double) box_T ), 0.25f, 0.25f, g_DebugTextSize, g_DebugTextSize, kbColor::red );
			}
		}
	}

	kbVec3 finalPlayerPos = StartPt;
	if ( bFoundHit ) {
		finalPlayerPos = StartPt + moveDir * T;

		// Slide if necessary
		const kbVec3 boxToPlayer = pHitCoverObj->GetPosition() - finalPlayerPos;
		const kbVec3 boxHalfLen = pHitCoverObj->GetBounds().Max() - pHitCoverObj->GetPosition();
		const float xDist = boxToPlayer.x / boxHalfLen.x;
		const float zDist = boxToPlayer.z / boxHalfLen.z;

		kbVec3 slideDir;

		if ( abs( xDist ) > abs( zDist ) ) {
			if ( xDist > 0 ) {
				slideDir = kbVec3( 0.0f, 0.0f, 1.0f );
			} else {
				slideDir = kbVec3( 0.0f, 0.0f, 1.0f );
			}
		} else {
			if ( zDist > 0 ) {
				slideDir = kbVec3( -1.0f, 0.0f, 0.0f );
			} else {
				slideDir = kbVec3( -1.0f, 0.0f, 0.0f );
			}
		}

		const float desiredSlideOffset = moveDir.Dot( slideDir );
		float currentSlideOffset = 0.0f;

		if ( finalPlayerPos.Compare( StartPt ) == false ) {
			currentSlideOffset = ( finalPlayerPos - StartPt ).Normalized().Dot( slideDir );
		}
		const float dif = desiredSlideOffset - currentSlideOffset;

		finalPlayerPos += slideDir * dif;

	} else {
		finalPlayerPos = EndPt;
	}

	EtherWorldGenCollision_t HitInfo;
	TraceAgainstWorld( HitInfo, finalPlayerPos - kbVec3( 0.0f, 999999.0f, 0.0f ), finalPlayerPos + kbVec3( 0.0f, 999999.0f, 0.0f ), false );

	if ( HitInfo.m_bHitFound ) {
		finalPlayerPos = HitInfo.m_HitLocation;
		finalPlayerPos.y += 20.0f;
	}

	if ( pActor != nullptr ) {
		pActor->GetParent()->SetPosition( finalPlayerPos );
	}
}

/**
 *	EtherWorldGenComponent::TraceAgainstTerrain_Recurse
 */
bool EtherWorldGenComponent::TraceAgainstTerrain_Recurse( EtherWorldGenCollision_t & OutHitInfo, const kbVec3 & StartPt, const kbVec3 & Dir, const float TraceLen, const bool bTraceAgainstDynamicCollision, const kbVec2 & LowerLeftPos, const float CurNodeLength ) const {

	const float HalfNodeLength = CurNodeLength * 0.5f;
	const float QuarterNodeLen = HalfNodeLength * 0.5f;


	const kbVec2 startPt2D( StartPt.x, StartPt.z );
	const kbVec2 dir2D( Dir.x * TraceLen, Dir.z * TraceLen );
	const kbVec2 & LL = LowerLeftPos;
	const kbVec2 LM = LL + kbVec2( 0.0f, HalfNodeLength );
	const kbVec2 C = LL + kbVec2( HalfNodeLength, HalfNodeLength );
	const kbVec2 ML = LL + kbVec2( HalfNodeLength, 0.0f );
	const kbVec2 RU = C + kbVec2( HalfNodeLength, HalfNodeLength );
	
	if ( CurNodeLength <= m_ChunkWorldLength + 32.0f ) {

		// We're at a leaf node.  Check for collision against triangles
		const kbVec3 nodeCenterPos = kbVec3( LL.x + HalfNodeLength, 0.0f, LL.y + HalfNodeLength );
		const kbVec3 EndPt = StartPt + Dir * TraceLen;
		const int chunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( nodeCenterPos );
		
		const EtherTerrainChunk *const pCurChunk = m_VisibleTerrainMap.Get2DMap()[chunkIdx];
		if ( pCurChunk == nullptr || pCurChunk->m_ChunkState != EtherTerrainChunk::Visible ) {
			return false;
		}

		const int NumTerrainVerts = ( m_TrisPerChunkSide * m_TrisPerChunkSide * 6 );
		float closestT = FLT_MAX;

		const kbVec3 rayVec = Dir * TraceLen;
		for ( int iVert = 0; iVert < NumTerrainVerts; iVert += 3 ) {
			const kbVec3 & v0 = pCurChunk->m_StaticCollisionMesh[iVert + 0];
			const kbVec3 & v1 = pCurChunk->m_StaticCollisionMesh[iVert + 1];
			const kbVec3 & v2 = pCurChunk->m_StaticCollisionMesh[iVert + 2];

			float t;
			if ( kbRayTriIntersection( StartPt, rayVec, v0, v1, v2, t ) ) {
				if ( t < closestT && t <= 1.0f && t > 0.0f ) {
					OutHitInfo.m_bHitFound = true;
					OutHitInfo.m_HitLocation = StartPt + Dir * t;
					closestT = t;
				}
			}
		}

		if ( bTraceAgainstDynamicCollision ) {
			for ( int iVert = 0; iVert < pCurChunk->m_TerrainJob.m_NumDynamicIndices; iVert += 3 ) {
				const kbVec3 & v0 = pCurChunk->m_DynamicCollisionMesh[iVert + 0];
				const kbVec3 & v1 = pCurChunk->m_DynamicCollisionMesh[iVert + 1];
				const kbVec3 & v2 = pCurChunk->m_DynamicCollisionMesh[iVert + 2];

				float t;
				if ( kbRayTriIntersection( StartPt, rayVec, v0, v1, v2, t ) ) {
					if ( t < closestT && t <= 1.0f && t > 0.0f ) {
						OutHitInfo.m_bHitFound = true;
						OutHitInfo.m_HitLocation = StartPt + Dir * t;
						closestT = t;
					}
				}
			}
		}

		if ( closestT < OutHitInfo.m_DistFromOrigin ) {
			OutHitInfo.m_DistFromOrigin = closestT;
		}

		return false;
	}

	int NumEntries = 0;

	float hitT;
	const kbVec2 childNodeExtent( HalfNodeLength, HalfNodeLength );
	if ( IntersectsAABB( hitT, startPt2D, dir2D, LL, LL + childNodeExtent ) && hitT <= TraceLen ) {
		TraceAgainstTerrain_Recurse( OutHitInfo, StartPt, Dir, TraceLen, bTraceAgainstDynamicCollision, LL, HalfNodeLength );
	}

	if ( IntersectsAABB( hitT, startPt2D, dir2D, ML, ML + childNodeExtent ) && hitT <= TraceLen ) {
		TraceAgainstTerrain_Recurse( OutHitInfo, StartPt, Dir, TraceLen, bTraceAgainstDynamicCollision, ML, HalfNodeLength );
	}

	if ( IntersectsAABB( hitT, startPt2D, dir2D, LM, LM + childNodeExtent ) && hitT <= TraceLen ) {
		TraceAgainstTerrain_Recurse( OutHitInfo, StartPt, Dir, TraceLen, bTraceAgainstDynamicCollision, LM, HalfNodeLength );
	}

	if ( IntersectsAABB( hitT, startPt2D, dir2D, C, C + childNodeExtent ) && hitT <= TraceLen ) {
		TraceAgainstTerrain_Recurse( OutHitInfo, StartPt, Dir, TraceLen, bTraceAgainstDynamicCollision, C, HalfNodeLength );
	}

	return false;
}

/**
 *	EtherWorldGenComponent::CoverObjectsPointTest
 */
bool EtherWorldGenComponent::CoverObjectsPointTest( const EtherCoverObject *& OutCoverObject, const kbVec3 & startPt ) const {

	if ( m_VisibleTerrainMap.IsInitialized() == false ) {
		return false;
	}

	std::vector<const EtherTerrainChunk*> ChunkList;
	static float spread = 32.0f;

	const kbVec3 offsets[] = { kbVec3( spread, 0.0f, spread ), kbVec3( -spread, 0.0f, spread ), kbVec3( -spread, 0.0f, -spread ), kbVec3( spread, 0.0f, -spread ) };

	for ( int i = 0; i < 4; i++ ) {
		const int StartChunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( startPt + offsets[i] );
		if ( StartChunkIdx >= 0 && StartChunkIdx < m_VisibleTerrainMap.Get2DMap().size() ) {
			const EtherTerrainChunk *const pStartChunk = m_VisibleTerrainMap.Get2DMap()[StartChunkIdx];
			if ( pStartChunk != nullptr && pStartChunk->m_ChunkState == EtherTerrainChunk::Visible ) {
				if ( VectorFind( ChunkList, pStartChunk ) == false ) {
					ChunkList.push_back( pStartChunk );
				}
			}
		}
	}

	if ( ChunkList.size() == 0 ) {
		return 0;
	}

	for ( int iChunk = 0; iChunk < ChunkList.size(); iChunk++ ) {
		for ( int iCover = 0; iCover < ChunkList[iChunk]->m_CoverObjects.size(); iCover++ ) {
			const EtherCoverObject *const pCurCover = &ChunkList[iChunk]->m_CoverObjects[iCover];
			kbVec3 BoundsExtent2D = ( pCurCover->GetBounds().Max() - pCurCover->GetPosition() );
			BoundsExtent2D.y = 0.0f;
			const float radiusSq = BoundsExtent2D.LengthSqr();
			kbVec3 VecToTestPt2D = ( pCurCover->GetPosition() - startPt );
			VecToTestPt2D.y = 0.0f;
			const float distSq = VecToTestPt2D.LengthSqr();

			if ( distSq < radiusSq ) {
				OutCoverObject = pCurCover;
				return true;
			}
		}
	}

	return false;
}

/**
 *	EtherWorldGenComponent::GetCoverObjectsInRadius
 */
int	EtherWorldGenComponent::GetCoverObjectsInRadius(  std::vector<EtherCoverObject> & OutObjects, const kbVec3 & startPt, const float Radius ) {
	OutObjects.clear();


	return 0;
}

/**
 *	EtherWorldGenComponent::SetTerrainWarp
 */
void EtherWorldGenComponent::SetTerrainWarp( const kbVec3 & location, const float amplitude, const float radius, const float timeScale ) {

	kbVec4 param1( location.x, location.y, location.z, g_GlobalTimer.TimeElapsedSeconds() );
	kbVec4 param2( amplitude, radius, timeScale, 0.0f );

	std::vector<kbVec4> terrainShaderParams;
	terrainShaderParams.push_back( param1 );
	terrainShaderParams.push_back( param2 );
	m_pTerrainShader->SetGlobalShaderParams( terrainShaderParams );
}

/**
 *	EtherEnviroInfo::Constructor
 */
void EtherEnviroInfo::Constructor() {
}

/**
 *	EtherEnviroMaterial::Constructor
 */
void EtherEnviroMaterial::Constructor() {
	m_Texture = nullptr;
	m_Color.Set( 0.0f, 1.0f, 0.0f, 1.0f );
}

/**
 *	EtherEnviroObject::Constructor
 */
void EtherEnviroObject::Constructor() {
	m_pModel = nullptr;
	m_MinScale.Set( 1.0f, 1.0f, 1.0f );
	m_MaxScale.Set( 1.0f, 1.0f, 1.0f );
	m_MinHealth = 10.0f;
	m_MaxHealth = 10.0f;
}

/**
 *	EtherEnviroComponent::Constructor
 */
void EtherEnviroComponent::Constructor() {
}

/**
 *	EtherTimeOfDayModifier::Constructor
 */
void EtherTimeOfDayModifier::Constructor() {
	m_Hour = 0.0f;
	m_SunColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_FogColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_SkyColor.Set( 0.0f, 0.0f, 1.0f, 1.0f );
	m_LightShaftColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
}

/**
 *	EtherWorldGenComponent::VisibleTerrainMap_t::Initialize
 */
void EtherWorldGenComponent::VisibleTerrainMap_t::Initialize( const kbVec3 & centerPos, const float TerrainChunkWidth, const int TerrainDimensions ) {
	m_ChunkWorldLength = TerrainChunkWidth;
	m_2DMapDimensions = TerrainDimensions + 2;

	m_TerrainChunk2DMap.resize( m_2DMapDimensions * m_2DMapDimensions, nullptr );
	m_LowerLeftCornerIdx = 0;

	kbVec3 normalizedTerrainPos = centerPos / TerrainChunkWidth;
	normalizedTerrainPos.x = floor( normalizedTerrainPos.x );		// Clamp to int boundaries
	normalizedTerrainPos.z = floor( normalizedTerrainPos.z );		// 

	const int HalfTerrainDim = TerrainDimensions / 2;
	m_LowerLeftCornerPosition = ( kbVec3( normalizedTerrainPos.x, 0.0f, normalizedTerrainPos.z ) * TerrainChunkWidth ) - kbVec3( TerrainChunkWidth * HalfTerrainDim, 0.0f, TerrainChunkWidth * HalfTerrainDim );
}

/**
 *	EtherWorldGenComponent::VisibleTerrainMap_t::Update
 */
void EtherWorldGenComponent::VisibleTerrainMap_t::Update( const kbVec3 & newCenterPos ) {
	const float halfChunkWorldLength = m_ChunkWorldLength * 0.5f;

	const kbVec3 terrainAligned_LL_PlayerPos = newCenterPos - kbVec3( halfChunkWorldLength, 0.0f, halfChunkWorldLength );

	const kbVec3 ChunkCornerExtent = kbVec3( halfChunkWorldLength * m_2DMapDimensions, 0.0f, halfChunkWorldLength * m_2DMapDimensions );
	const kbVec3 LL_CornerPos = terrainAligned_LL_PlayerPos - ChunkCornerExtent;
	const kbVec3 UR_CornerPos = terrainAligned_LL_PlayerPos + ChunkCornerExtent;

	const int LL_Idx = GetChunkIndexFromPosition( LL_CornerPos );
	if ( LL_Idx != m_LowerLeftCornerIdx ) {
		m_LowerLeftCornerIdx  = LL_Idx;
		m_LowerLeftCornerPosition = LL_CornerPos;						
	}
}

/**
 *	EtherWorldGenComponent::VisibleTerrainMap_t::GetChunkIndexFromPosition
 */
int	EtherWorldGenComponent::VisibleTerrainMap_t::GetChunkIndexFromPosition( const kbVec3 & position ) const {
	kbErrorCheck( m_LowerLeftCornerIdx != -1, "VisibleTerrainMap_t accessed before being initialized" );
 
	const int curX = m_LowerLeftCornerIdx % m_2DMapDimensions;
	const int curZ = m_LowerLeftCornerIdx / m_2DMapDimensions;

	int xIndex = (int)( ( ( position.x - m_LowerLeftCornerPosition.x ) / m_ChunkWorldLength ) + curX ) % m_2DMapDimensions;
	int zIndex = (int)( ( ( position.z - m_LowerLeftCornerPosition.z ) / m_ChunkWorldLength ) + curZ ) % m_2DMapDimensions;

	if ( xIndex < 0 ) {
		xIndex += m_2DMapDimensions;
	}

	if ( zIndex < 0 ) {
		zIndex += m_2DMapDimensions;
	}

	const int returnIndex =  xIndex + ( zIndex * m_2DMapDimensions );
	return returnIndex;
}

/**
 *	EtherWorldGenComponent::VisibleTerrainMap_t::AddChunk
 */
void EtherWorldGenComponent::VisibleTerrainMap_t::AddChunk( EtherTerrainChunk *const pChunkToAdd ) {
	const kbVec3 ChunkPosition = pChunkToAdd->m_pTerrainEntity->GetPosition();
	const int idx = GetChunkIndexFromPosition( pChunkToAdd->m_pTerrainEntity->GetPosition() );
	kbErrorCheck( idx >= 0 && idx < m_TerrainChunk2DMap.size() && m_TerrainChunk2DMap[idx] == nullptr, "Failed to Add Chunk" );

	m_TerrainChunk2DMap[idx] = pChunkToAdd;
}

/**
 *	EtherWorldGenComponent::VisibleTerrainMap_t::RemoveChunk
 */
void EtherWorldGenComponent::VisibleTerrainMap_t::RemoveChunk( EtherTerrainChunk *const pChunkToRemove ) {
	const int idx = GetChunkIndexFromPosition( pChunkToRemove->m_pTerrainEntity->GetPosition() );
	if ( idx >= 0 && idx < m_TerrainChunk2DMap.size() ) {
		if ( m_TerrainChunk2DMap[idx] == pChunkToRemove ) {
			m_TerrainChunk2DMap[idx] = nullptr;
		}
	}
}
