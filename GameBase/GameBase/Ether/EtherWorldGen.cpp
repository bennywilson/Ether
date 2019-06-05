//===================================================================================================
// EtherWorldGen.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbIntersectionTests.h"
#include "kbGameEntityHeader.h"
#include "kbJobManager.h"
#include "EtherWorldGen.h"
#include "EtherGame.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(EtherWorldGenComponent)

const int g_MaxDynamicVertices = 10000;

kbConsoleVariable g_ShowTerrainCollision( "showterraincollision", false, kbConsoleVariable::Console_Bool, "Shows terrain collision", "" );
kbConsoleVariable g_SetHourOfDay( "sethourofday", -1.0f, kbConsoleVariable::Console_Float, "Sets the hour of the day (0-23)", "" );
kbConsoleVariable g_ShowCoverObjects( "showcoverobjects", false, kbConsoleVariable::Console_Bool, "Draws debug shapes around the cover objects", "" );
kbConsoleVariable g_ProcGenInfo( "procgeninfo", false, kbConsoleVariable::Console_Bool, "Displays info about the procedurally generated world", "" );


/**
 *	EtherAntialiasingComponent::Constructor
 */
void EtherAntialiasingComponent::Constructor() {
	SetRenderPass( RP_PostProcess );
	m_pShader = nullptr;
}

/**
 *	EtherAntialiasingComponent::SetEnable_Internal
 */
void EtherAntialiasingComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->RegisterRenderHook( this );
	} else {
		g_pRenderer->UnregisterRenderHook( this );
	}
}

/**
 *	EtherAntialiasingComponent::RenderHookCallBack
 */
void EtherAntialiasingComponent::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {
	if ( m_pShader == nullptr ) {
		g_pRenderer->RT_CopyRenderTarget( pSrc, pDst );
		return;
	}

	g_pRenderer->RT_SetRenderTarget( pDst );
	kbShaderParamOverrides_t shaderParams;
	//shaderParams.SetVec4( "fog_Start_End_Clamp", kbVec4( m_FogStartDist, m_FogEndDist, m_FogClamp, 0.0f ) );
	//shaderParams.SetVec4( "fogColor", m_FogColor );

	kbVec3 position;
	kbQuat orientation;
	g_pRenderer->GetRenderViewTransform( nullptr, position, orientation );

	//shaderParams.SetVec4( "cameraPosition", position );
	g_pRenderer->RT_Render2DQuad( kbVec2( 0.5f, 0.5f ), kbVec2( 1.0f, 1.0f ), kbColor::white, m_pShader, &shaderParams );
}

/**
 *	EtherFogComponent::Constructor
 */
void EtherFogComponent::Constructor() {
	SetRenderPass( RP_Translucent );
	m_pShader = nullptr;
	m_FogStartDist = 300;
	m_FogEndDist = 3000;
	m_FogClamp = 1.0f;
	m_FogColor = kbColor::white;
}

/**
 *	EtherFogComponent::RenderHookCallBack
 */
void EtherFogComponent::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {
	//g_pRenderer->RT_ClearRenderTarget( pDst, kbColor::white );

	if ( m_pShader == nullptr ) {
		m_pShader = (kbShader *) g_ResourceManager.GetResource( "./assets/shaders/PostProcess/Fog.kbshader", true, true );
	}

	g_pRenderer->RT_SetRenderTarget( pDst );
	kbShaderParamOverrides_t shaderParams;
	shaderParams.SetVec4( "fog_Start_End_Clamp", kbVec4( m_FogStartDist, m_FogEndDist, m_FogClamp, 0.0f ) );
	shaderParams.SetVec4( "fogColor", m_FogColor );

	kbVec3 position;
	kbQuat orientation;
	g_pRenderer->GetRenderViewTransform( nullptr, position, orientation );

	shaderParams.SetVec4( "cameraPosition", position );
	g_pRenderer->RT_Render2DQuad( kbVec2( 0.5f, 0.5f ), kbVec2( 1.0f, 1.0f ), kbColor::white, m_pShader, &shaderParams );
}

/**
 *	EtherFogComponent::RenderHookCallBack
 */
void EtherFogComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pRenderer->RegisterRenderHook( this );
	} else {
		g_pRenderer->UnregisterRenderHook( this );
	}
}

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
EtherTerrainChunk::EtherTerrainChunk( const int numTrisPerSide ) {

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
}

/**
 *	EtherWorldGenComponent::TearDownWorld
 */
void EtherWorldGenComponent::TearDownWorld() {

	
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

}

/**
 *	EtherWorldGenComponent::UpdateTimeOfDayFX
 */
void EtherWorldGenComponent::UpdateTimeOfDayFX() {

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
				if ( VectorContains( ChunkList, pStartChunk ) == false ) {
					ChunkList.push_back( pStartChunk );
				}
			}
		}

		const int EndChunkIdx = m_VisibleTerrainMap.GetChunkIndexFromPosition( EndPt + offsets[i] );
		if ( EndChunkIdx >= 0 && EndChunkIdx < m_VisibleTerrainMap.Get2DMap().size() ) {
			const EtherTerrainChunk *const pEndChunk = m_VisibleTerrainMap.Get2DMap()[StartChunkIdx];
			if ( pEndChunk != nullptr && pEndChunk->m_ChunkState == EtherTerrainChunk::Visible ) {
				if ( VectorContains( ChunkList, pEndChunk ) == false ) {
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
		pActor->GetOwner()->SetPosition( finalPlayerPos );
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
			if ( kbRayTriIntersection( t, StartPt, rayVec, v0, v1, v2 ) ) {
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
				if ( kbRayTriIntersection( t, StartPt, rayVec, v0, v1, v2 ) ) {
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
				if ( VectorContains( ChunkList, pStartChunk ) == false ) {
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
	m_MinRainSheetTileAndSpeed.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_MaxRainSheetTileAndSpeed.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	m_RainColor.Set( 1.0f, 1.0f, 1.0f, 1.0f );
}

/**
 *	EtherEnviroComponent::SetEnable_Internal
 */
void EtherEnviroComponent::SetEnable_Internal( const bool bEnable ) {
	if ( bEnable ) {
		SetGlobalShaderParams();
	}
}

/**
 *	EtherEnviroComponent::EditorChange
 */
void EtherEnviroComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );
	if ( IsEnabled() ) {
		SetGlobalShaderParams();
	}
}

/**
 *	EtherEnviroComponent::SetGlobalShaderParams
 */
void EtherEnviroComponent::SetGlobalShaderParams() {

	kbShaderParamOverrides_t shaderParam;
	shaderParam.SetVec4( "rainParams", kbVec4Rand( m_MinRainSheetTileAndSpeed, m_MaxRainSheetTileAndSpeed ) );
	shaderParam.SetVec4( "rainColor", m_RainColor );
	for ( int i = 0; i < shaderParam.m_ParamOverrides.size(); i++ ) {
		g_pRenderer->SetGlobalShaderParam( shaderParam.m_ParamOverrides[i] );
	}
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
