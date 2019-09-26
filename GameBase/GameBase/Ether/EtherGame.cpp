//===================================================================================================
// EtherGame.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbTypeInfo.h"
#include "EtherGame.h"
#include "kbIntersectionTests.h"
#include "EtherSkelModel.h"
#include "EtherPlayer.h"
#include "EtherAI.h"
#include "EtherWeapon.h"
#include "DX11/kbRenderer_DX11.h"
#include <directxpackedvector.h>

// oculus
#include "OVR_CAPI_D3D.h"
#include "OVR_Math.h"
using namespace OVR;

kbConsoleVariable g_NoEnemies( "noenemies", false, kbConsoleVariable::Console_Bool, "Remove enemies", "" );
kbConsoleVariable g_LockMouse( "lockmouse", true, kbConsoleVariable::Console_Int, "Locks mouse", "" );
kbConsoleVariable g_ShowPos( "showpos", false, kbConsoleVariable::Console_Bool, "Displays player position", "" );

EtherGame * g_pEtherGame = nullptr;


/**
 *	EtherGame::EtherGame
 */
EtherGame::EtherGame() :
	kbRenderHook( RP_FirstPerson ),
	m_CameraMode( Cam_FirstPerson ),
	m_pPlayerComponent( nullptr ),
	m_pWorldGenComponent( nullptr ),
	m_pCharacterPackage( nullptr ),
	m_pWeaponsPackage( nullptr ),
	m_pFXPackage( nullptr ),
	m_pTranslucentShader( nullptr ),
	m_HMDWorldOffset( kbVec3::zero ),
	m_pBulletHoleRenderTexture( nullptr ),
	m_pGrassCollisionTexture( nullptr ),
	m_pGrassCollisionReadBackTexture( nullptr ),
	m_pCollisionMapTimeGenShader( nullptr ),
	m_pCollisionMapDamageGenShader( nullptr ),
	m_pBulletHoleUpdateShader( nullptr ) {
	m_ShotFrame = 0;

	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );

	kbErrorCheck( g_pEtherGame == nullptr, "EtherGame::EtherGame() - g_pEtherGame is not nullptr" );
	g_pEtherGame = this;
}

/**
 *	EtherGame::~EtherGame
 */
EtherGame::~EtherGame() {

	kbErrorCheck( g_pEtherGame != nullptr, "EtherGame::~EtherGame() - g_pEtherGame is nullptr" );
	g_pEtherGame = nullptr;
}

kbRenderObject crossHair;

/**
 *	EtherGame::PlayGame_Internal
 */
void EtherGame::PlayGame_Internal() {
	g_pRenderer->RegisterRenderHook( this );

	m_FirePrefabs[0] = m_pFXPackage->GetPrefab( "Fire_0" );
	m_SmokePrefabs[0] = m_pFXPackage->GetPrefab( "Smoke_0" );
	m_EmberPrefabs[0] = m_pFXPackage->GetPrefab( "Embers_0" );
	m_FireLightPrefabs[0] = m_pFXPackage->GetPrefab( "FireLight_0" );
}

/**
 *	EtherGame::InitGame_Internal
 */
void EtherGame::InitGame_Internal() {

	m_AIManager.Initialize();

	m_pCharacterPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Characters.kbPkg" );
	kbErrorCheck( m_pCharacterPackage != nullptr, "Unable to find character package." );

	m_pWeaponsPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Weapons.kbPkg" );
	kbErrorCheck( m_pWeaponsPackage != nullptr, "Unable to find weapons package." );

	m_pFXPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/FX.kbPkg" );
	kbErrorCheck( m_pFXPackage != nullptr, "Unable to find FX package." );

	const kbPrefab *const pBaseMuzzleFlashPrefab = m_pFXPackage->GetPrefab( "BaseMuzzleFlash" );
	if ( pBaseMuzzleFlashPrefab != nullptr && pBaseMuzzleFlashPrefab->GetGameEntity(0) != nullptr ) {
		const kbGameEntity *const pCurParticleEntity = pBaseMuzzleFlashPrefab->GetGameEntity(0);
		for ( int i = 1; i < pCurParticleEntity->NumComponents(); i++ ) {
			if ( pCurParticleEntity->GetComponent( i )->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}

			const kbParticleComponent *const pParticleComponent = static_cast<kbParticleComponent*>( pCurParticleEntity->GetComponent( i ) );
			m_pParticleManager->PoolParticleComponent( pParticleComponent, 16 );
		}
	}
	m_GameStartTimer.Reset();

	m_pTranslucentShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbShader", true, true );
}

/**
 *	EtherGame::StopGame_Internal
 */
void EtherGame::StopGame_Internal() {
	m_pPlayerComponent = nullptr;
	m_pLocalPlayer = nullptr;

	for ( int i = 0; i < m_FireEntities.size(); i++ ) {
		m_FireEntities[i].Destroy();
	}
	m_FireEntities.clear();
	m_RenderThreadScorch.clear();

	g_pRenderer->UnregisterRenderHook( this );
}

/**
 *	EtherGame::LevelLoaded_Internal
 */
void EtherGame::LevelLoaded_Internal() {

	kbGameEntity *const pPlayerEntity = CreatePlayer( 0, m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID(), kbVec3::zero );

	if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
		kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
		pEntity->SetPosition( pEntity->GetPosition() + kbVec3( 0.0f, 20000.0f, 0.0f ) );
	}
}

/**
 *	EtherGame::PreUpdate_Internal
 */
void EtherGame::PreUpdate_Internal() {

	const float DT = GetFrameDT();
	if ( IsConsoleActive() == false ) {
		ProcessInput( DT );
	}
	
	if ( GetAsyncKeyState( VK_LSHIFT ) && GetAsyncKeyState( 'P' ) ) {
	    kbCamera & playerCamera = GetCamera();

		for ( int i = 0; i < GetGameEntities().size();  i++ ) {

			const kbGameEntity *const pCurEntity = GetGameEntities()[i];
			kbPlayerStartComponent *const pStart = (kbPlayerStartComponent*)pCurEntity->GetComponentByType( kbPlayerStartComponent::GetType() );
			if ( pStart == nullptr ) {
				continue;
			}

			const kbVec3 newPos = pCurEntity->GetPosition();
			const kbQuat newRotation = pCurEntity->GetOrientation();

			playerCamera.m_Position = newPos;
			playerCamera.m_Rotation = newRotation;
			playerCamera.m_RotationTarget = newRotation;

			m_pLocalPlayer->SetPosition( newPos );
			m_pLocalPlayer->SetOrientation( newRotation );
			break;
		}
	}

	if ( ( g_pD3D11Renderer->IsRenderingToHMD() || g_pD3D11Renderer->IsUsingHMDTrackingOnly() ) && g_pD3D11Renderer->GetFrameNum() > 0 ) {
	
		kbVec3 eyePos[2];
	
		const ovrPosef * eyeRenderPose = g_pD3D11Renderer->GetOvrEyePose();
		float eyeX = 200.0f * ( ( eyeRenderPose[0].Position.x + eyeRenderPose[1].Position.x ) * 0.5f );
	
		kbMat4 camMatrix = m_Camera.m_Rotation.ToMat4();
		m_HMDWorldOffset = camMatrix[0].ToVec3() * eyeX;
	
		for ( int eye = 0; eye < 2; eye++ ) {
			Vector3f curEyePos = eyeRenderPose[eye].Position;
			curEyePos.x += eyeX;
	
			if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
				kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
				EtherWeaponComponent *const pPlayerWeapon = static_cast<EtherWeaponComponent*>( pEntity->GetComponentByType( EtherWeaponComponent::GetType() ) );
				if ( pPlayerWeapon != nullptr ) {
	
					kbVec3 curPos( 5.5f, -10.0f, 3.0f );	// Weapon offset from camera
					curPos += m_HMDWorldOffset; 
					kbTransformComponent *const pTrans = static_cast<kbTransformComponent*>( pPlayerWeapon->GetOwner()->GetComponent(0) );
					pTrans->SetPosition( kbVec3( curPos.x, curPos.y, curPos.z ) );
				}
			}
	
			eyePos[eye] = ovrVecTokbVec3( curEyePos );
		}
	
		// Update renderer cam
		g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_Camera.m_Position + m_HMDWorldOffset, m_Camera.m_Rotation );
	
	} else {
		if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
			kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
			EtherWeaponComponent *const pPlayerWeapon = static_cast<EtherWeaponComponent*>( pEntity->GetComponentByType( EtherWeaponComponent::GetType() ) );
			if ( pPlayerWeapon != nullptr ) {
		
				static kbVec3 curPos( 7.75f, -6.5f, 8.0f );	// Weapon offset from camera
				static float updateAmt = 1.0f;
				kbTransformComponent *const pTrans = static_cast<kbTransformComponent*>( pPlayerWeapon->GetOwner()->GetComponent(0) );
				pTrans->SetPosition( kbVec3( curPos.x, curPos.y, curPos.z ) );
			}
		}

		// Update renderer cam
		g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );
	}

	if ( g_ShowPos.GetBool() ) {
		std::string PlayerPos;
		PlayerPos += "x:";
		PlayerPos += std::to_string( m_Camera.m_Position.x );
		PlayerPos += " y:";
		PlayerPos += std::to_string( m_Camera.m_Position.y );
		PlayerPos += " z:";
		PlayerPos += std::to_string( m_Camera.m_Position.z );
	
		g_pD3D11Renderer->DrawDebugText( PlayerPos, 0, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );
	}

	UpdateWorld( DT );
}

/**
 *	EtherGame::AddGameEntity_Internal
 */
void EtherGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {
	if ( pEntity == nullptr ) {
		kbLog( "EtherGame::AddGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer && m_pWorldGenComponent != nullptr ) {
		kbVec3 groundPt;
		m_Camera.m_Position.y = 256.0f;
		m_pLocalPlayer->SetPosition( m_Camera.m_Position );

		const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
		m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
	}
}

/**
 *	EtherGame::RemoveEntity_Internal
 */
void EtherGame::RemoveGameEntity_Internal( kbGameEntity *const pEntity ) {

//	kbGame::RemoveGameEntity_Internal( pEntity );

}

/**
 *	EtherGame::ProcessInput
 */
void EtherGame::ProcessInput( const float DT ) {

	if ( m_pLocalPlayer == nullptr || m_pLocalPlayer->GetActorComponent()->IsDead() ) {
		return;
	}

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}

	if ( GetAsyncKeyState( VK_SPACE ) ) {
		const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
		m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
		m_Camera.m_Rotation = kbQuatFromMatrix( orientation );
		m_Camera.m_RotationTarget = kbQuatFromMatrix( orientation );

		POINT CursorPos;
		GetCursorPos( &CursorPos );
		RECT rc;
 		GetClientRect( m_Hwnd, &rc );
		SetCursorPos( (rc.right - rc.left ) / 2, (rc.bottom - rc.top) / 2 );
	}

	if ( g_LockMouse.GetInt() <= 1 ) {
		if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != nullptr ) {
			m_pPlayerComponent->HandleMovement( GetInput(), DT );
		}
	}

	m_Camera.Update();

	if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != nullptr ) {
		m_pPlayerComponent->HandleAction( GetInput() );
	}
}

/**
 *	EtherGame::TraceAgainstWorld
 */
bool EtherGame::TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbVec3 & collisionPt, const bool bTraceAgainstDynamicCollision ) {

	if ( m_pWorldGenComponent == nullptr ) {
		return false;
	}

	EtherWorldGenCollision_t HitResult;
	m_pWorldGenComponent->TraceAgainstWorld( HitResult, startPt, endPt, bTraceAgainstDynamicCollision );
	if ( HitResult.m_bHitFound ) {
		collisionPt = HitResult.m_HitLocation;
	}

	return HitResult.m_bHitFound;
}

/**
 *	EtherGame::CoverObjectsPointTest
 */
bool EtherGame::CoverObjectsPointTest( const EtherCoverObject *& OutCoverObject, const kbVec3 & startPt ) const {
	if ( m_pWorldGenComponent == nullptr ) {
		return false;
	}

	return m_pWorldGenComponent->CoverObjectsPointTest( OutCoverObject, startPt );
}

/**
 *	EtherGame::MoveActorAlongGround
 */
void EtherGame::MoveActorAlongGround( EtherActorComponent *const pActor, const kbVec3 & startPt, const kbVec3 & endPt ) {

	kbErrorCheck( pActor != nullptr, "EtherGame::MoveActorAlongGround() - nullptr actor passed in" );

	if ( m_pWorldGenComponent == nullptr ) {
		return;
	}

	m_pWorldGenComponent->MoveActorAlongGround( pActor, startPt, endPt );
}

/**
 *	EtherGame::UpdateWorld
 */
void EtherGame::UpdateWorld( const float DT ) {
	const double GameTimeInSeconds = m_GameStartTimer.TimeElapsedSeconds();

	UpdateFires_GameThread( DT );


/*
	m_AIManager.Update( DT );
	int numEntities = 0;
	for ( int iEntity = 0; iEntity < GetGameEntities().size(); iEntity++ ) {
		for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
			if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( EtherEnemySoldierAIComponent::GetType() ) ) {
				numEntities++;
			}
		}
	}

	if ( g_NoEnemies.GetBool() ) {
		for ( int iEntity = (int)GetGameEntities().size() - 1; iEntity >= 0; iEntity-- ) {
			for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
				if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( EtherEnemySoldierAIComponent::GetType() ) ) {
					g_pGame->RemoveGameEntity( GetGameEntities()[iEntity] );
				}
			}
		}
	}

	if ( GameTimeInSeconds > 7.0f && GetPlayersList().size() > 0 && numEntities < 1 && g_NoEnemies.GetBool() == false ) {
		static float XOffsetRange = 500.0f;
		static float ZOffsetRange = 1750.0;
		const float xOffset = ( XOffsetRange * 0.33f + kbfrand() * ( XOffsetRange * 0.667f ) ) * ( ( kbfrand() < 0.5f ) ? ( 1.0f ) : ( -1.0f ) );
		const float zOffset = ( ZOffsetRange * 0.33f + kbfrand() * ( ZOffsetRange * 0.667f ) );
		const kbVec3 spawnLocation = GetPlayersList()[0]->GetPosition() + kbVec3( xOffset, 0.0f, zOffset );

		const kbGameEntity *const pGameEntity = m_pCharacterPackage->GetPrefab( "Cartel_Warrior" )->GetGameEntity( 0 );
		kbGameEntity *const pEntity = CreateEntity( pGameEntity );

		pEntity->SetPosition( spawnLocation );
	}

	UpdateStimPack( DT );
	UpdateAirstrike( DT );
	UpdateOLC( DT );*/
}

/**
 *	EtherGame::CreatePlayer
 */
kbGameEntity * EtherGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	kbGameEntity * pNewEntity = nullptr;

	if ( prefabGUID == m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID() ) {
		// Create players

		kbLog( "Creating player with id %d", netId );

		pNewEntity = g_pGame->CreateEntity( m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0), true );

		AddPrefabToEntity( m_pWeaponsPackage, "EL_Rifle", pNewEntity, false );
		AddPrefabToEntity( m_pWeaponsPackage, "EL_Grenade", pNewEntity, false );

		for ( int i = 0; i < pNewEntity->NumComponents(); i++ ) {
			kbComponent *const pCurComponent = pNewEntity->GetComponent(i);
			if ( pCurComponent->IsA( EtherPlayerComponent::GetType() ) ) {
				m_pPlayerComponent = static_cast<EtherPlayerComponent*>( pCurComponent );
				kbErrorCheck( m_pPlayerComponent != nullptr, "EtherGame::CreatePlayer() - Failed to find player component" );
			} else if ( pCurComponent->IsA( kbSkeletalModelComponent::GetType() ) ) {
				kbSkeletalModelComponent *const pSkelModel = static_cast<kbSkeletalModelComponent*>( pCurComponent );
				pSkelModel->Enable( true );
			}
		}

		// Place on ground
	/*	if ( m_pWorldGenComponent != nullptr ) {
			const kbVec3 desiredStartLocation = kbVec3::zero;
			kbVec3 groundPt;
			if ( TraceAgainstWorld( desiredStartLocation + kbVec3( 0.0f, 10000.0f, 0.0f ), desiredStartLocation - kbVec3( 0.0f, 10000.0f, 0.0f ), groundPt, false ) ) {
				pNewEntity->SetPosition( groundPt );
			} else {
				pNewEntity->SetPosition( DesiredLocation );
			}
		}*/

		for ( int i = 0; i < GetGameEntities().size(); i++ ) {

			const kbGameEntity *const pCurEntity = GetGameEntities()[i];
			kbPlayerStartComponent *const pStart = (kbPlayerStartComponent*)pCurEntity->GetComponentByType( kbPlayerStartComponent::GetType() );
			if ( pStart != nullptr ) {
				pNewEntity->SetPosition( pCurEntity->GetPosition() );
				break;
			}
		}

		m_pLocalPlayer = pNewEntity;

	} else {

		pNewEntity = g_pGame->CreateEntity( g_ResourceManager.GetGameEntityFromGUID( prefabGUID ), false );
		pNewEntity->SetPosition( DesiredLocation );
	}

	return pNewEntity;
}

/**
 *	EtherGame::AddPrefabToEntity
 */
void EtherGame::AddPrefabToEntity( const kbPackage *const pPrefabPackage, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly ) {
	if ( pPrefabPackage == nullptr ) {
		kbError( "EtherGame::AddPrefabToEntity() - nullptr prefab package found while searching for %s", prefabName.c_str() );
		return;
	}

	const kbPrefab *const pPrefab = pPrefabPackage->GetPrefab( prefabName );
	if ( pPrefab == nullptr || pPrefab->GetGameEntity(0) == nullptr ) {
		kbError( "EtherGame::AddPrefabToEntity() - Null prefab or none Entities found with name %s", prefabName.c_str() );
		return;
	}

	const kbGameEntity *const pPrefabEntity = pPrefab->GetGameEntity(0);
	if ( bComponentsOnly == false ) {
		kbGameEntity *const newItem = new kbGameEntity( pPrefabEntity, false );
		pEntity->AddEntity( newItem );
		for ( int i = 0; i < pEntity->NumComponents(); i++ ) {
			if ( pEntity->GetComponent(i)->IsA( EtherActorComponent::GetType() ) ) {
				if ( static_cast<EtherActorComponent*>( pEntity->GetComponent(i) )->GetEquippedItem() == nullptr ) {
					static_cast<EtherActorComponent*>( pEntity->GetComponent(i) )->SetEquippedItem( newItem );
				}
			}
		}
		return;
	}

	for ( int iComp = 1; iComp < pPrefabEntity->NumComponents(); iComp++ ) {

		bool bShowComponent = true;
		const kbComponent *const pPrefabComponent = pPrefabEntity->GetComponent(iComp);
kbLog(" Prefab is %s", pPrefabComponent->GetType()->GetClassNameW() );
		if ( pPrefabComponent->IsA( kbSkeletalModelComponent::GetType() ) ) {
			const kbSkeletalModelComponent *const skelModel = static_cast<const kbSkeletalModelComponent*>( pPrefabComponent );
			bool isRifle = skelModel->GetOwnerName() == kbString( "EL_Rifle" );
			kbLog( "%s -- %d", skelModel->GetOwnerName().c_str(), isRifle );
			if ( isRifle && g_pGame->GetLocalPlayer() != pEntity ) {
				continue;
			}

			if ( isRifle == false && g_pGame->GetLocalPlayer() == pEntity ) {
				bShowComponent = false;
			}
		} else if ( pPrefabComponent->IsA( EtherPlayerComponent::GetType() ) && pEntity != g_pGame->GetLocalPlayer() ) {
			continue;
		}

		kbComponent *const pNewComponent = pPrefabComponent->Duplicate();
		pEntity->AddComponent( pNewComponent );
		pNewComponent->Enable( false );
		if ( bShowComponent ) {
			pNewComponent->Enable( true );
		}
	}
}

/**
 *	EtherGame::RegiRegisterBulletShotsterHit
 */
void EtherGame::RegisterBulletShot( kbComponent *const pHitComponent, const kbVec3 & shotStart, const kbVec3 & shotEnd ) {
	frameBulletShots newShot;
	newShot.shotStart = shotStart;
	newShot.shotEnd = shotEnd;
	newShot.pHitComponent = pHitComponent;
	m_ShotsThisFrame[m_ShotFrame].push_back(newShot);
}

/**
 *	EtherGame::RenderSync
 */
void EtherGame::RenderSync() {

	if ( HasFirstSyncCompleted() == false ) {

		m_pParticleManager->SetCustomAtlasTexture( 0, "./assets/FX/fx_atlas.jpg" );

		m_pParticleManager->SetCustomAtlasShader( 1, "./assets/shaders/FX/shellTrailParticle.kbShader" );
		m_pParticleManager->SetCustomAtlasTexture( 1, "./assets/FX/SmokeTrailAtlas.dds" );

		g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicParticle.kbShader", true, true );
		m_pParticleManager->SetCustomAtlasShader( 2, "../../kbEngine/assets/Shaders/basicParticle.kbShader" );
		m_pParticleManager->SetCustomAtlasTexture( 2, "./assets/FX/MuzzleFlashes/BasicOrange_MuzzleFlash.jpg" );

		// Bullet Hole FX
		m_pBulletHoleRenderTexture = g_pRenderer->RT_GetRenderTexture( 4096, 4096, eTextureFormat::KBTEXTURE_R8G8B8A8, false );
		g_pRenderer->RT_ClearRenderTarget( m_pBulletHoleRenderTexture, kbColor::white );
		
		g_ResourceManager.GetResource( "./assets/FX/noise.jpg", true, true );
		g_ResourceManager.GetResource( "./assets/FX/scorch.jpg", true, true );

		for ( int i = 0; i < GetGameEntities().size(); i++ ) {
			kbGameEntity *const pEnt = GetGameEntities()[i];
			if ( pEnt->GetName().find( "Holey_Wall" ) != std::string::npos ) {
				kbStaticModelComponent *const pSM = (kbStaticModelComponent*)pEnt->GetComponentByType( kbStaticModelComponent::GetType() );
				if ( pSM != nullptr ) {
	
					kbShader *const pHoleShader = (kbShader*)g_ResourceManager.GetResource( "./assets/shaders/environment/environmenthole.kbshader", true, true );
					kbTexture *const diff = (kbTexture*)g_ResourceManager.GetResource( "./assets/models/architecture/bricks.png", true, true );
					kbTexture *const normal = (kbTexture*)g_ResourceManager.GetResource( "./assets/models/architecture/bricks_nm.png", true, true );

					pSM->SetMaterialParamTexture( 0, "holeTex", m_pBulletHoleRenderTexture );

				}
				break;
			}
		}

		// Grass Damage
		m_pGrassCollisionTexture = g_pRenderer->RT_GetRenderTexture( 1024, 1024, eTextureFormat::KBTEXTURE_R16G16B16A16, false );
		g_pRenderer->RT_ClearRenderTarget( m_pGrassCollisionTexture, kbColor( 64000.0f, 64000.0f, 64000.0f, 64000.0f ) );

		m_pGrassCollisionReadBackTexture = g_pRenderer->RT_GetRenderTexture( 1024, 1024, eTextureFormat::KBTEXTURE_R16G16B16A16, true );

		for ( int i = 0; i < GetGameEntities().size(); i++ ) {
			kbGameEntity *const pEnt = GetGameEntities()[i];
			if ( pEnt->GetName().find( "Terrain" ) != std::string::npos ) {
				kbTerrainComponent *const pTerrain = (kbTerrainComponent*)pEnt->GetComponentByType( kbTerrainComponent::GetType() );
				if ( pTerrain != nullptr ) {
					pTerrain->SetCollisionMap( m_pGrassCollisionTexture );
					break;
				}
			}
		}

		m_pCollisionMapDamageGenShader = (kbShader *) g_ResourceManager.GetResource( "./assets/shaders/DamageGen/collisionMapDamageGen.kbshader", true, true );
		m_pCollisionMapTimeGenShader = (kbShader *) g_ResourceManager.GetResource( "./assets/shaders/DamageGen/collisionMapTimeGen.kbshader", true, true );
		m_pBulletHoleUpdateShader = (kbShader *) g_ResourceManager.GetResource( "./assets/shaders/DamageGen/pokeyholeunwrap.kbshader", true, true );
		m_pCollisionMapScorchGenShader = (kbShader*) g_ResourceManager.GetResource( "./assets/shaders/DamageGen/collisionMapScorchGen.kbShader", true, true );
	}

	m_RenderThreadShotsThisFrame = m_ShotsThisFrame[m_ShotFrame];
	m_ShotFrame ^= 1;

	m_ShotsThisFrame[m_ShotFrame].clear();

	UpdateFires_RenderSync();
}

/**
 *	EtherGame::RenderHookCallBack
 */
static float g_TimeMultiplier = 0.95f / 0.016f;
void EtherGame::RenderHookCallBack( kbRenderTexture *const pSrc, kbRenderTexture *const pDst ) {
	static kbVec3 terrainPos;
	static float terrainWidth;
	static float halfTerrainWidth;

	// Initialize
	static kbTerrainComponent * pTerrain = nullptr;
	if ( pTerrain == nullptr ) {

		for ( int i = 0; i < GetGameEntities().size(); i++ ) {
			kbGameEntity *const pEnt = GetGameEntities()[i];
			pTerrain = (kbTerrainComponent*)pEnt->GetComponentByType( kbTerrainComponent::GetType() );
			if ( pTerrain == nullptr ) {
				continue;
			}

			terrainPos = pEnt->GetPosition();
			terrainWidth = pTerrain->GetTerrainWidth();
			halfTerrainWidth = terrainWidth * 0.5f;
			break;
		}
	}

    if ( pTerrain == nullptr ) {
        return;
    }

	if ( GetAsyncKeyState( 'C' ) ) {
		g_pRenderer->RT_ClearRenderTarget( m_pBulletHoleRenderTexture, kbColor::white );
		g_pRenderer->RT_ClearRenderTarget( m_pGrassCollisionTexture, kbColor( 64000.0f, 64000.0f, 64000.0f, 64000.0f ) );
	}

	// Update time in collision Map
	g_pRenderer->RT_SetRenderTarget( m_pGrassCollisionTexture );
//	const float timeMultiplier = ( 1.0f - kbClamp( g_TimeMultiplier * GetCurrentFrameDeltaTime(), 0.0f, 1.0f ) ) * 0.15f + 0.85f;
	//g_pRenderer->RT_Render2DLine( kbVec3( 0.5f, 0.0f, 0.0f ), kbVec3( 0.5f, 1.0f, 0.0f ), kbColor( timeMultiplier, timeMultiplier, timeMultiplier, timeMultiplier ), 15.0f, m_pCollisionMapUpdateTimeShader );

	const float curTime = g_GlobalTimer.TimeElapsedSeconds();
	const kbVec3 terrainCenter( terrainPos.x, terrainPos.z, 0.0f );

	for ( int i = 0; i < m_RenderThreadShotsThisFrame.size(); i++ ) {

		const frameBulletShots & curShot = m_RenderThreadShotsThisFrame[i];
		if ( curShot.pHitComponent != nullptr ) {
			kbGameEntity *const pEnt = (kbGameEntity*)curShot.pHitComponent->GetOwner();
			kbStaticModelComponent *const pSM = (kbStaticModelComponent*)pEnt->GetComponentByType( kbStaticModelComponent::GetType() );
			if ( pSM == nullptr ) {
				continue;
			}

			// Generate bullet holes
			g_pRenderer->RT_SetRenderTarget( m_pBulletHoleRenderTexture );
			kbShaderParamOverrides_t shaderParams;

			kbMat4 invWorldMatrix;
			invWorldMatrix.MakeScale( pEnt->GetScale() );
			invWorldMatrix *= pEnt->GetOrientation().ToMat4();
			invWorldMatrix[3] = pEnt->GetPosition();
			invWorldMatrix.InvertFast();

			const kbVec3 hitLocation = invWorldMatrix.TransformPoint( curShot.shotEnd );
			const kbVec3 hitDir = ( curShot.shotEnd - curShot.shotStart ).Normalized() * invWorldMatrix;
			const float holeSize = 0.75f + ( kbfrand() * 0.5f );
			const float scorchSize = 3.0f + ( kbfrand() * 1.5f );

			//shaderParams.SetTexture( "baseTexture", pSM->SetSh()->GetMaterials()[0].GetTextureList()[0] );
			shaderParams.SetVec4( "hitLocation", kbVec4( hitLocation.x, hitLocation.y, hitLocation.z, holeSize ) );
			shaderParams.SetVec4( "hitDirection", kbVec4( hitDir.x, hitDir.y, hitDir.z, scorchSize ) );

			kbTexture *const pNoiseTex = (kbTexture*)g_ResourceManager.GetResource( "./assets/FX/Noise/noise.jpg", true, true );
			shaderParams.SetTexture( "noiseTex", pNoiseTex );

			kbTexture *const pScorchTex = (kbTexture*)g_ResourceManager.GetResource( "./assets/FX/scorch.jpg", true, true );
			shaderParams.SetTexture( "scorchTex", pScorchTex );

			g_pRenderer->RT_RenderMesh( pSM->GetModel(), m_pBulletHoleUpdateShader, &shaderParams );
			
		}

		// Update collision map
		g_pRenderer->RT_SetRenderTarget( m_pGrassCollisionTexture );
		kbVec3 startPos = ( ( kbVec3( curShot.shotStart.x, curShot.shotStart.z, 0.0f ) - terrainCenter ) / halfTerrainWidth ) * 0.5f + 0.5f;
		startPos.z = curShot.shotStart.y;

		kbVec3 endPos = ( ( kbVec3( curShot.shotEnd.x, curShot.shotEnd.z, 0.0f ) - terrainCenter ) / halfTerrainWidth ) * 0.5f + 0.5f;
		endPos.z = curShot.shotEnd.y;

		kbVec3 finalStartPt = kbVec3( startPos.x * 2.0f - 1.0f, ( ( startPos.y * 2.0f) - 1.0f ), 0.0f );
		kbVec3 finalEndPt = kbVec3( endPos.x * 2.0f - 1.0f, ( ( endPos.y * 2.0f) - 1.0f ), 0.0f );
		kbVec3 perpLine( finalEndPt.x - finalStartPt.x, finalEndPt.y - finalStartPt.y, 0.0f );
		perpLine.Normalize();

		const float pushLineWidth = 32.0f / 4096.0f;
		float swap = perpLine.x;
		perpLine.x = perpLine.y;
		perpLine.y = -swap;
		perpLine.Normalize();

		perpLine *= pushLineWidth * 0.5f;
	
		// Render push data
		m_ShaderParamOverrides.SetVec4( "perpendicularDirection", kbVec4( perpLine.x, perpLine.y, g_GlobalTimer.TimeElapsedSeconds(), 0.0f ) );
		m_ShaderParamOverrides.SetVec4( "heightScale", kbVec4( pTerrain->GetHeightScale(), 0.0f, 0.0f, 0.0f ) );
	
		// Render time data
		g_pRenderer->RT_Render2DLine( startPos, endPos, kbColor( 0.0f, 0.0f, 1.0f, 0.0f ), pushLineWidth, m_pCollisionMapTimeGenShader, &m_ShaderParamOverrides );

		// Render damage
		g_pRenderer->RT_Render2DLine( startPos, endPos, kbColor( 0.0f, 0.0f, curTime, 0.0f ), 16.0f / 4096.0f, m_pCollisionMapDamageGenShader, &m_ShaderParamOverrides );
	}
	m_RenderThreadShotsThisFrame.clear();

	UpdateFires_RenderHook( pTerrain );
}

/**
 *	EtherCollisionMapReadBackJob
 */
const float StartingDamageMapHeight = 64000.0f;
class EtherCollisionMapReadBackJob : public kbJob {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherCollisionMapReadBackJob() :
													m_bSynced( false ) { }

	virtual void								Run() override {

		m_FireInfo.clear();

		DirectX::PackedVector::HALF * pCurVal = (DirectX::PackedVector::HALF*)m_RTMap.m_pData;
		const size_t halfsPerWidth = m_RTMap.m_Width * 4;
		const size_t halfsPerPitch = m_RTMap.m_rowPitch / sizeof(DirectX::PackedVector::HALF);
		const float curTimer = g_GlobalTimer.TimeElapsedSeconds();
		int numFires = 0;

		const float thresh = 64000.0f - 1000.0f;
		for ( uint y = 0; y < m_RTMap.m_Height; y++ ) {
			for ( uint x = 0; x < m_RTMap.m_Width; x++ ) {

				byte * pCurByte = (byte*)pCurVal;
				const float r = DirectX::PackedVector::XMConvertHalfToFloat( *pCurVal );
				pCurVal++;
				const float g = DirectX::PackedVector::XMConvertHalfToFloat( *pCurVal );
				pCurVal++;
				const float b = DirectX::PackedVector::XMConvertHalfToFloat( *pCurVal );
				pCurVal++;
				const float a = DirectX::PackedVector::XMConvertHalfToFloat( *pCurVal );
				pCurVal++;

				const bool rTest = ( r < thresh && curTimer < r + 2.0f );
				const bool bTest = ( a < thresh && curTimer < b + 0.5f );
				if ( rTest || bTest ) {

					if ( kbfrand() > 0.9f ) {
						kbVec3 normalizedPos( (float) x / m_RTMap.m_Width, 0.0f, (float) y /  m_RTMap.m_Height );
						normalizedPos = ( normalizedPos * 2.0f ) - 1.0f;
						normalizedPos = m_WorldCenter + normalizedPos * ( m_WorldSize * 0.5f );

						FireInfo newFireInfo;
						if ( bTest ) {
							newFireInfo.m_Position.Set( normalizedPos.x, a, normalizedPos.z );
						} else {
							newFireInfo.m_Position.Set( normalizedPos.x, -59999.0f, normalizedPos.z );
						}
						m_FireInfo.push_back( newFireInfo );
						numFires++;
					}
				}
			}

			pCurVal += halfsPerPitch - halfsPerWidth;
		}
	}

	// Input
	kbRenderTargetMap							m_RTMap;

	kbVec3										m_WorldCenter;
	float										m_WorldSize;

	// Output
	struct FireInfo {
		kbVec3									m_Position;
	};

	std::vector<FireInfo>						m_FireInfo;

	bool										m_bSynced;

} g_CollisionMapReadJob;

/**
 *	EtherGame::UpdateFires_GameThread
 */
void EtherGame::UpdateFires_GameThread( const float DeltaTime ) {

	if ( g_CollisionMapReadJob.IsJobFinished() ) {

		if ( m_FireEntities.size() < 40.0f ) {
			for ( int i = 0; i < g_CollisionMapReadJob.m_FireInfo.size(); i++ ) {

				EtherCollisionMapReadBackJob::FireInfo & curFire = g_CollisionMapReadJob.m_FireInfo[i];
				bool bCancelTheContract = false;

				kbVec3 firePos = curFire.m_Position;
				firePos.y = 0.0f;

				for ( int l = 0; l < m_FireEntities.size(); l++ ) {
					kbVec3 compareFirePos = m_FireEntities[l].GetPosition();
					compareFirePos.y = 0.0f;

					float distBetween = ( firePos - compareFirePos ).Length();
					if ( distBetween < 10.0f ) {
						bCancelTheContract = true;
						break;
					}
				}

				if ( bCancelTheContract ) {
					continue;
				}

				kbVec3 startPos = curFire.m_Position;
				startPos.y = 0.0f;
				const kbCollisionInfo_t collisionInfo = g_CollisionManager.PerformLineCheck( startPos + kbVec3( 0.0f, 99999999.0f, 0.0f ), startPos + kbVec3( 0.0f, -99999999.0f, 0.0f ) );
				if ( collisionInfo.m_bHit && ( curFire.m_Position.y < -50000.0f || abs( curFire.m_Position.y - collisionInfo.m_HitLocation.y ) < 15.0f ) ) {
					if ( collisionInfo.m_pHitComponent->GetOwner()->GetComponentByType( kbTerrainComponent::GetType() ) != nullptr ) {
						EtherFireEntity newFireEntity( collisionInfo.m_HitLocation, m_FirePrefabs[0], m_SmokePrefabs[0], m_EmberPrefabs[0], m_FireLightPrefabs[0] );
						m_FireEntities.push_back( newFireEntity );
					}
				}
			}
		}
		g_CollisionMapReadJob.m_FireInfo.clear();

		static float lastTime = -1.0f;
		if ( g_GlobalTimer.TimeElapsedSeconds() > lastTime + 0.5f ) {
			g_CollisionMapReadJob.m_bSynced = true;
			lastTime = g_GlobalTimer.TimeElapsedSeconds();
		}
	}

	for ( int i = (int)m_FireEntities.size() - 1; i >= 0; i-- ) {
		if ( m_FireEntities[i].IsFinished() ) {
			m_FireEntities[i].Destroy();
			VectorRemoveFastIndex( m_FireEntities, i );
			continue;
		}
		m_FireEntities[i].Update( DeltaTime );
	}
}

/**
 *	EtherGame::UpdateFires_RenderSync
 */
void EtherGame::UpdateFires_RenderSync() {
	m_RenderThreadScorch.clear();
	for ( int i = 0; i < m_FireEntities.size(); i++ ) {
		if ( m_FireEntities[i].GetScorchRadius() > 0.00001f ) {
			RenderThreadScorch newScorch;
			newScorch.m_Position = m_FireEntities[i].GetScorchOffset();
			newScorch.m_Size.Set( m_FireEntities[i].GetScorchRadius(), m_FireEntities[i].GetScorchRadius(), m_FireEntities[i].GetScorchRadius() );
			m_RenderThreadScorch.push_back( newScorch );
		}
	}
}

/**
 *	EtherGame::UpdateFires_RenderHook
 */
void EtherGame::UpdateFires_RenderHook( const kbTerrainComponent *const pTerrain ) {

    if ( pTerrain == nullptr ) {
        return;
    }

	const kbVec3 terrainPos = pTerrain->GetOwner()->GetPosition();
	const kbVec3 terrainCenter( pTerrain->GetOwner()->GetPosition().x, pTerrain->GetOwner()->GetPosition().z, 0.0f );
	const float terrainWidth = pTerrain->GetTerrainWidth();
	const float halfTerrainWidth = pTerrain->GetTerrainWidth() * 0.5f;

	static int status = -1;

	if ( g_CollisionMapReadJob.m_bSynced ) {
		if ( status == 0 ) {
			g_pRenderer->RT_UnmapRenderTarget( m_pGrassCollisionReadBackTexture );
		} else if ( status == 1 || status == -1 ) {
			g_pRenderer->RT_CopyRenderTarget( m_pGrassCollisionTexture, m_pGrassCollisionReadBackTexture );
			status = 1;
		} else if ( status == 2 ) {
			g_CollisionMapReadJob.m_RTMap = g_pRenderer->RT_MapRenderTarget( m_pGrassCollisionReadBackTexture );
			g_CollisionMapReadJob.m_WorldCenter = terrainPos;
			g_CollisionMapReadJob.m_WorldSize = terrainWidth;
			g_CollisionMapReadJob.m_bSynced = false;
			g_pJobManager->RegisterJob( &g_CollisionMapReadJob );
		}
		status++;
		if ( status > 2 ) {
			status = 0;
		}
	}

	if ( m_RenderThreadScorch.size() == 0 ) {
		return;
	}

	// EtherGame Scorches
	g_pRenderer->RT_SetRenderTarget( m_pGrassCollisionTexture );

	for ( int i = 0; i < m_RenderThreadScorch.size(); i++ ) {

		kbVec2 startPos( m_RenderThreadScorch[i].m_Position.x, m_RenderThreadScorch[0].m_Position.z );
		kbVec2 width( m_RenderThreadScorch[i].m_Size.x, m_RenderThreadScorch[i].m_Size.z );

		startPos.x = ( ( startPos.x - terrainCenter.x - (width.x * 0.5f) ) / halfTerrainWidth ) * 0.5f + 0.5f;
		startPos.y = ( ( startPos.y - terrainCenter.z - (width.y * 0.5f) ) / halfTerrainWidth ) * 0.5f + 0.5f;

		kbShaderParamOverrides_t shaderParams;
		shaderParams.SetVec4( "centerPt", kbVec4( ( startPos.x * 2.0f ) - 1.0f,  -((startPos.y*2.0f) - 1.0f), width.x, 0.0f ) );
		g_pRenderer->RT_Render2DQuad( startPos, width, kbColor::red, m_pCollisionMapScorchGenShader, &shaderParams ); 
	}
}

/**
 *	EtherFireEntity::EtherFireEntity
 */
static kbVec3 fireOffset = kbVec3( 0.0f, 20.0f, 0.0f );
static kbVec3 smokeOffset = kbVec3( 0.0f, 12.0f, 0.0f );
static kbVec3 emberOffset = kbVec3( 0.0f, 15.0f, 0.0f );
static kbVec3 fireLightOffset = kbVec3( 0.0f, 15.25f, 0.0f );
static float lightIntensity = 0.45f;

EtherFireEntity::EtherFireEntity( const kbVec3 & position, const kbPrefab *const pFirePrefab, const kbPrefab *const pSmokePrefab, const kbPrefab *const pParticlePrefab, const kbPrefab * const pFireLightPrefab ) {

	m_ScorchRadius = 0.0f;
	m_ScorchState = 0;

	m_pFireEntity = g_pGame->CreateEntity( pFirePrefab->GetGameEntity(0) );
	m_pSmokeEntity = g_pGame->CreateEntity( pSmokePrefab->GetGameEntity(0) );
	m_pEmberEntity = g_pGame->CreateEntity( pParticlePrefab->GetGameEntity(0) );

	static int numero = 1;

	if ( numero & 1 ) {
		m_pFireLight = g_pGame->CreateEntity( pFireLightPrefab->GetGameEntity(0) );
	} else {
		m_pFireLight = nullptr;
	}

	numero++;

	m_Position = position - kbVec3( 0.0f, 60.0f, 0.0f );

	const float scaleAmt = 0.67f + kbfrand() * 0.70f;

	m_pFireEntity->SetScale( m_pFireEntity->GetScale() * scaleAmt );
	m_pSmokeEntity->SetScale( m_pFireEntity->GetScale() * scaleAmt );
	m_pEmberEntity->SetScale( m_pEmberEntity->GetScale() * scaleAmt );

	m_pFireEntity->SetPosition( position );
	m_pSmokeEntity->SetPosition( position + kbVec3( 0.0f, -kbfrand() * 12.0f, 0.0f ) + smokeOffset );
	m_pEmberEntity->SetPosition( position );

	if ( m_pFireLight != nullptr ) {
		m_pFireLight->SetPosition( position + fireLightOffset );
		kbPointLightComponent *const pLight = (kbPointLightComponent*)m_pFireLight->GetComponentByType( kbPointLightComponent::GetType() );
		pLight->Enable( false );
		pLight->Enable( true );
		m_StartFireColor = pLight->GetColor() * lightIntensity;
		pLight->SetColor( kbColor::black );
		m_StartFireLightPos = m_pFireLight->GetPosition();
	}

	m_FireStartPos = m_pFireEntity->GetPosition();
	m_SmokeStartPos = m_pSmokeEntity->GetPosition();
	m_EmberStartPos = m_pEmberEntity->GetPosition();

	m_StartingTimeSeconds = g_GlobalTimer.TimeElapsedSeconds();
	m_RandomScroller = 1.0f + kbfrand() * 0.1f;
	m_ScrollRate = kbVec3Rand( kbVec3( 1.0f, 1.0f, 1.0f ), kbVec3( 1.15f, 1.15f, 1.15f ) );

	// MATERIALHACK

	kbStaticModelComponent * pSM = (kbStaticModelComponent*)m_pSmokeEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->SetMaterialParamVector( 0, "additionalData", kbVec4( 0.0f, m_RandomScroller, m_ScrollRate.x, m_ScrollRate.y ) );

	pSM = (kbStaticModelComponent*)m_pFireEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->SetMaterialParamVector( 0, "additionalData", kbVec4( 0.0f, m_RandomScroller, m_ScrollRate.z, m_ScrollRate.y ) );

	m_bIsFinished = false;
}

/**
 *	EtherFireEntity::Destroy
 */
void EtherFireEntity::Destroy() {

	kbStaticModelComponent * pSM = (kbStaticModelComponent*)m_pSmokeEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->Enable( false );

	pSM = (kbStaticModelComponent*)m_pFireEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->Enable( false );

	kbParticleComponent *const pParticle = (kbParticleComponent*)m_pEmberEntity->GetComponentByType( kbParticleComponent::GetType() );
	pParticle->Enable( false );

	if ( m_pFireLight != nullptr ) {
		kbPointLightComponent *const pLight = (kbPointLightComponent*)m_pFireLight->GetComponentByType( kbPointLightComponent::GetType() );
		pLight->Enable( false );
		g_pGame->RemoveGameEntity( m_pFireLight );
	}
	g_pGame->RemoveGameEntity( m_pFireEntity );
	g_pGame->RemoveGameEntity( m_pSmokeEntity );
	g_pGame->RemoveGameEntity( m_pEmberEntity );
}

/**
 *	EtherFireEntity::Update
 */
void EtherFireEntity::Update( const float DeltaTime ) {

	if ( GetAsyncKeyState( 'C' ) ) {
		m_bIsFinished = true;
		return;
	}

	const float currentTimeSeconds = g_GlobalTimer.TimeElapsedSeconds();
	const float scaleTime = 1.0f;
	const float unclampedNormalizedTime = ( currentTimeSeconds - m_StartingTimeSeconds ) / scaleTime;
	const float clampedNormalizedTime = kbClamp( unclampedNormalizedTime, 0.0f, 1.0f );

	float firePos = clampedNormalizedTime;
	float fireFade = 1;
	float smokeFade = clampedNormalizedTime;
	float scrollTime = currentTimeSeconds;

	if ( unclampedNormalizedTime > 0.33f && m_ScorchState == 0 ) {
		m_ScorchState = 1;
		m_ScorchRadius = 4.0f / 1024.0f;

		m_ScorchOffset = m_Position;
	} else if ( m_ScorchState == 1 ) {
		m_ScorchRadius = 0.0f;
		m_ScorchState = 2;
		m_NextStateChangeTime = 2.0f + kbfrand() * 2.0f + currentTimeSeconds;
	} else if ( m_ScorchState == 2 && currentTimeSeconds > m_NextStateChangeTime ) {
		m_ScorchState = 3;

		m_ScorchRadius = 4.0f / 1024.0f;
		m_ScorchOffset = m_Position + kbVec3( ( kbfrand() * 10.0f ) - 5.0f, 0.0f, ( kbfrand() * 10.0f ) - 5.0f );
	} else if ( m_ScorchState == 3 ) {
		m_ScorchRadius = 0.0f;
		m_FadeOutStartTime = currentTimeSeconds;
		m_NextStateChangeTime = 5.1f + kbfrand() * 5.3f;
		m_ScorchState = 4;

		kbParticleComponent *const pParticle = (kbParticleComponent*)m_pEmberEntity->GetComponentByType( kbParticleComponent::GetType() );
		pParticle->EnableNewSpawns( false );

	} else if ( m_ScorchState == 4 ) {
		fireFade = firePos = 1.0f - kbClamp( ( currentTimeSeconds - m_FadeOutStartTime ) / m_NextStateChangeTime, 0.0f, 1.0f );
		smokeFade = 1.0f - kbClamp( ( currentTimeSeconds - m_FadeOutStartTime ) / ( m_NextStateChangeTime * 1.3f ), 0.0f, 1.0f );

		if ( currentTimeSeconds > m_FadeOutStartTime + m_NextStateChangeTime * 1.3f ) {
			m_bIsFinished = true;
		}
		scrollTime = m_FadeOutStartTime + ( currentTimeSeconds - m_FadeOutStartTime ) * ( 0.1f + fireFade * 0.9f );
	}

	if ( m_pFireLight != nullptr ) {
		kbPointLightComponent *const pLight = (kbPointLightComponent*)m_pFireLight->GetComponentByType( kbPointLightComponent::GetType() );
		pLight->Enable( false );
		const float targetIntensity = m_StartFireColor.ToVec3().Dot( kbVec3( 0.3f, 0.59f, 0.11f ) ) * ( fireFade * ( lightIntensity + kbfrand() * ( lightIntensity * 1.0f ) ) );
		const float curIntensity = pLight->GetColor().ToVec3().Dot( kbVec3( 0.3f, 0.59f, 0.11f ) );
		float newIntensity = kbClamp( kbLerp( curIntensity, targetIntensity, 0.25f ), 0.0f, 1.0f );
		pLight->SetColor( m_StartFireColor * newIntensity );

		kbVec3 desiredPos = kbLerp( m_pFireLight->GetPosition(), m_StartFireLightPos + kbVec3( kbfrand() * 3.0f, kbfrand() * 3.0f, kbfrand() * 4.5f ), 0.25f );
		m_pFireLight->SetPosition( desiredPos );
		pLight->Enable( true );
	}

	m_pFireEntity->SetPosition( m_FireStartPos + fireOffset * firePos );
	m_pSmokeEntity->SetPosition( m_SmokeStartPos + smokeOffset * smokeFade );
	m_pEmberEntity->SetPosition( m_EmberStartPos + fireOffset * fireFade );

	m_RandomScroller += DeltaTime * fireFade;

	kbStaticModelComponent * pSM = (kbStaticModelComponent*)m_pSmokeEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->SetMaterialParamVector( 0, "additionalData", kbVec4( smokeFade * 0.24f, m_RandomScroller, m_ScrollRate.x, m_ScrollRate.y ) );

	pSM = (kbStaticModelComponent*)m_pFireEntity->GetComponentByType( kbStaticModelComponent::GetType() );
	pSM->SetMaterialParamVector( 0, "additionalData", kbVec4( fireFade, m_RandomScroller, m_ScrollRate.z, m_ScrollRate.y ) );	
}

