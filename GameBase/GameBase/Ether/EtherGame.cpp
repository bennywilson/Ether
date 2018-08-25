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

// oculus
#include "OVR_CAPI_D3D.h"
#include "OVR_Math.h"
using namespace OVR;

kbConsoleVariable g_NoEnemies( "noenemies", false, kbConsoleVariable::Console_Bool, "Remove enemies", "" );
kbConsoleVariable g_LockMouse( "lockmouse", true, kbConsoleVariable::Console_Int, "Locks mouse", "" );

EtherGame * g_pEtherGame = nullptr;

// Stimpack
const float g_SlomoLength = 7.0f;

// Airstrike
const float g_AirstrikeDurationSec = 7.0f;
const float g_TimeBetweenBombers = 1.5f;
const float g_TimeBetweenBombs = 0.075f;

const kbVec3 g_CountUIScale( 0.5f, 0.5f, 0.5f );
static kbVec3 g_CountUIOffset( 14.96f, -12.0f, 10.0f );

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
	m_CurrentGameState( GamePlay ),
	m_VerseIdx( 0 ),
	m_SlomoStartTime( -1.0f ),
	m_pSlomoSound( nullptr ),
	m_AirstrikeTimeLeft( -120.0f ),
	m_BombersLeft( 0 ),
	m_NextBomberSpawnTime( 0.0f ),
	m_pELBomberModel( nullptr ),
	m_pAirstrikeFlybyWave( nullptr ),
	m_OLCTimer( -1.0f ),
	m_OLCPostProcess( -1.0f ),
	m_pOLCWindupWave( nullptr ),
	m_pOLCExplosion( nullptr ),
	m_OLCTint( kbVec3::zero ),
	m_pBulletHoleTarget( nullptr ) {

	m_ELBomberEntity[0] = m_ELBomberEntity[1] = m_ELBomberEntity[2] = nullptr;
	m_BombTimer[0] = m_BombTimer[1] = m_BombTimer[2] = 0.0f;

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

/*
	m_pAirstrikeFlybyWave = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/airstrike.wav", true );
	kbErrorCheck( m_pAirstrikeFlybyWave != nullptr, "EtherGame::PlayGame_Internal() - Failed to find airstrike.wav" );

	m_pOLCWindupWave = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/olc_windup.wav", true );
	m_pOLCExplosion = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/olc_explosion.wav", true );*/
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

	m_pTranslucentShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbShader", true );
	m_pSlomoSound = (kbWaveFile*)g_ResourceManager.GetResource( "./assets/Sounds/slomo.wav", true );
}

/**
 *	EtherGame::StopGame_Internal
 */
void EtherGame::StopGame_Internal() {
	m_pPlayerComponent = nullptr;
	m_pLocalPlayer = nullptr;

	g_pRenderer->UnregisterRenderHook( this );

	for ( int i = 0; i < 3; i++ ) {
		RemoveGameEntity( m_ELBomberEntity[i] );
	}
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

	m_pParticleManager->SetCustomParticleTextureAtlas( 0, "./assets/FX/fx_atlas.jpg" );

	m_pParticleManager->SetCustomParticleTextureAtlas( 1, "./assets/FX/SmokeTrailAtlas.dds" );
	m_pParticleManager->SetCustomParticleShader( 1, "./assets/shaders/shellTrailParticle.kbShader" );
}

/**
 *	EtherGame::Update_Internal
 */
void EtherGame::Update_Internal( float DT ) {

	if ( IsConsoleActive() == false ) {
		ProcessInput( DT );
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
/*
if (GetAsyncKeyState('O')) curPos.y += updateAmt;
if (GetAsyncKeyState('P')) curPos.y -= updateAmt;

if (GetAsyncKeyState('K')) curPos.x += updateAmt;
if (GetAsyncKeyState('L')) curPos.x -= updateAmt;

if (GetAsyncKeyState('N')) curPos.z += updateAmt;
if (GetAsyncKeyState('M')) curPos.z -= updateAmt;
*/
				kbTransformComponent *const pTrans = static_cast<kbTransformComponent*>( pPlayerWeapon->GetOwner()->GetComponent(0) );
				pTrans->SetPosition( kbVec3( curPos.x, curPos.y, curPos.z ) );
			}
		}

		// Update renderer cam
		g_pD3D11Renderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );
	}
	
	std::string PlayerPos;
	PlayerPos += "x:";
	PlayerPos += std::to_string( m_Camera.m_Position.x );
	PlayerPos += " y:";
	PlayerPos += std::to_string( m_Camera.m_Position.y );
	PlayerPos += " z:";
	PlayerPos += std::to_string( m_Camera.m_Position.z );
	
	//g_pD3D11Renderer->DrawDebugText( PlayerPos, 0, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );


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

	static bool bCameraChanged = false;
	if ( GetAsyncKeyState( 'C' ) && GetAsyncKeyState( VK_CONTROL ) ) {
		if ( bCameraChanged == false ) {
			bCameraChanged = true;
			m_CameraMode = eCameraMode_t( 1 + (int)m_CameraMode );
			if ( m_CameraMode >= Cam_Free ) {
				m_CameraMode = Cam_FirstPerson;
			}

			if ( m_pLocalPlayer != nullptr ) {
				for ( int i = 0; i < m_pLocalPlayer->NumComponents(); i++ ) {

					if ( m_pLocalPlayer->GetComponent(i)->IsA( EtherSkelModelComponent::GetType() ) ) {
						EtherSkelModelComponent *const SkelComp = static_cast<EtherSkelModelComponent*>( m_pLocalPlayer->GetComponent(i) );
						if ( m_CameraMode == Cam_FirstPerson ) {
							SkelComp->Enable( SkelComp->IsFirstPersonModel() );
						} else {
							SkelComp->Enable( !SkelComp->IsFirstPersonModel() );
						}
					}
				}
			}
		}
	} else {
		bCameraChanged = false;
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
			} else if ( pCurComponent->IsA( EtherSkelModelComponent::GetType() ) ) {
				EtherSkelModelComponent *const pSkelModel = static_cast<EtherSkelModelComponent*>( pCurComponent );
				pSkelModel->Enable( pSkelModel->IsFirstPersonModel() );
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
		if ( pPrefabComponent->IsA( EtherSkelModelComponent::GetType() ) ) {
			const EtherSkelModelComponent *const skelModel = static_cast<const EtherSkelModelComponent*>( pPrefabComponent );
			if ( skelModel->IsFirstPersonModel() && g_pGame->GetLocalPlayer() != pEntity ) {
				continue;
			}

			if ( skelModel->IsFirstPersonModel() == false && g_pGame->GetLocalPlayer() == pEntity ) {
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
 *	EtherGame::ActivateStimPack
 */
void EtherGame::ActivateStimPack() {

	if ( m_SlomoStartTime > 0.f || m_pPlayerComponent->GetNumStimPacks() == 0 ) {
	   return;
	}

	SetDeltaTimeScale( 0.2f );
	m_SlomoStartTime = g_GlobalTimer.TimeElapsedSeconds();

	m_pPlayerComponent->UseStimPack();

	if ( m_pSlomoSound != nullptr ) {
		GetSoundManager().PlayWave( m_pSlomoSound, 0.5f );
	}
}

/**
 *	EtherGame::UpdateStimPack
 */
void EtherGame::UpdateStimPack( const float deltaTimeSec ) {

	if ( g_GlobalTimer.TimeElapsedSeconds() > m_SlomoStartTime + g_SlomoLength ) {
		m_SlomoStartTime = -1.0f;
		SetDeltaTimeScale(1.0f);
		GetSoundManager().SetFrequencyRatio(1.0f);
	}
}

/**
 *	EtherGame::ActivateAirstrike
 */
void EtherGame::ActivateAirstrike() {

	if ( m_AirstrikeTimeLeft > 0.0f || m_pPlayerComponent->GetNumAirstrikes() == 0 ) {
		return;
	}
	
	m_AirstrikeTimeLeft = g_AirstrikeDurationSec;

	m_pPlayerComponent->UseAirstrike();

	m_BombersLeft = 3;
	m_NextBomberSpawnTime = 0.0f;
	m_BombTimer[0] = m_BombTimer[1] = m_BombTimer[2] = g_TimeBetweenBombs;

	for ( int i = 0; i < 3; i++ ) {
		for ( int iComp = 0; iComp < m_ELBomberEntity[i]->NumComponents(); iComp++ ) {
			m_ELBomberEntity[i]->GetComponent(iComp)->Enable( true );
		}
	}
}

/**
 *	EtherGame::UpdateAirstrike
 */
void EtherGame::UpdateAirstrike( const float DeltaTimeSec ) {

	if ( m_AirstrikeTimeLeft < -10.0f ) {
		return;
	}

	m_AirstrikeTimeLeft -= DeltaTimeSec;
	if ( m_AirstrikeTimeLeft < 0 ) {
		for ( int i = 0; i < 3; i++ ) {
			for ( int iComp = 0; iComp < m_ELBomberEntity[i]->NumComponents(); iComp++ ) {
				m_ELBomberEntity[i]->SetPosition( kbVec3::zero );
			}
		}
		return;
	}

	// Spawn new bombers
	if ( m_BombersLeft > 0 ) {
		m_NextBomberSpawnTime -= DeltaTimeSec;
		if ( m_NextBomberSpawnTime <= 0 ) {
			m_NextBomberSpawnTime = g_TimeBetweenBombers;
			m_BombersLeft--;

			float xOffset;
			switch( m_BombersLeft ) {
				case 2 : xOffset = 0.0f; break;
				case 1 : xOffset = 600.0f; break;
				case 0 : xOffset = -700.0f; break;
			}

			m_ELBomberEntity[m_BombersLeft]->SetPosition( m_pLocalPlayer->GetPosition() + kbVec3( 0.0f, 1000.0f, -300.0f ) + kbVec3( xOffset, 0.0f, 0.0f ) );

			m_SoundManager.PlayWave( m_pAirstrikeFlybyWave, 1.0f );
		}
	}

	// Update bombers
	for ( int i = 2; i >= m_BombersLeft; i-- ) {

		static float moveRate = 3000.f;
		const kbVec3 moveDelta = kbVec3( 0.0f, 0.0f, 1.0f ) * DeltaTimeSec * moveRate;
		m_ELBomberEntity[i]->SetPosition( m_ELBomberEntity[i]->GetPosition() + moveDelta );
		if ( m_ELBomberEntity[i]->GetPosition().z < m_pLocalPlayer->GetPosition().z || m_ELBomberEntity[i]->GetPosition().z > m_pLocalPlayer->GetPosition().z + 4096.0f ) {
			continue;
		}

		m_BombTimer[i] -= DeltaTimeSec;
		if ( m_BombTimer[i] > 0 ) {
			continue;
		}
		m_BombTimer[i] = g_TimeBetweenBombs;
		
		const kbPrefab *const pBombPrefab = m_pWeaponsPackage->GetPrefab( "Airstrike_Bomb" );
		if ( pBombPrefab != nullptr && pBombPrefab->GetGameEntity(0) != nullptr ) {
			const kbGameEntity *const pBombPrefabEnt = pBombPrefab->GetGameEntity(0);
			kbGameEntity *const newProjectile = g_pGame->CreateEntity( pBombPrefabEnt );

			newProjectile->SetPosition( m_ELBomberEntity[i]->GetPosition() );

			static float xOffset = 2.0f;
			const float halfXOffset = xOffset * 0.5f;

			const kbVec3 zVec = kbVec3( ( kbfrand() * xOffset ) - halfXOffset, -1.0f, 0.0f ).Normalized();
			const kbVec3 yVec = ( kbVec3( 1.0f, 0.0f, 0.0f).Cross( zVec ) ).Normalized();
			const kbVec3 xVec = yVec.Cross( zVec ).Normalized();
			const kbMat4 downMat( kbVec4( xVec.x, xVec.y, xVec.z, 0.0f ),   kbVec4( yVec.x, yVec.y, yVec.z, 0.0f ), kbVec4( zVec.x, zVec.y, zVec.z, 0.0f ), kbVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			newProjectile->SetOrientation( kbQuatFromMatrix( downMat)  );

			// TODO: is it necessary to enable the projectile?
			EtherProjectileComponent *const pProj = (EtherProjectileComponent *)newProjectile->GetComponentByType( EtherProjectileComponent::GetType() );
			if ( pProj != nullptr ) {
				pProj->Enable( true );
			}
		}
	}
}

/**
 *	EtherGame::ActivateOLC
 */
const float g_OLCLen = 13.6f;
void EtherGame::ActivateOLC() {
	if ( m_OLCTimer > 0 || m_pPlayerComponent->GetNumOLC() == 0 ) {
		return;
	}

	m_OLCTimer = g_OLCLen;
	m_pPlayerComponent->UseOLC();
}

/**
 *	EtherGame::UpdateOLC
 */
void EtherGame::UpdateOLC( const float DeltaTimeSec ) {

if ( m_pWorldGenComponent == nullptr ) {
		return;
	}
	if ( m_OLCTimer < 0 ) {
		m_OLCPostProcess = -1.0f;
	 	m_pWorldGenComponent->SetTerrainWarp( kbVec3::zero, 0, 0, 0 );
		m_OLCTint = kbVec3::zero;

		return;
	}

	m_OLCTimer -= DeltaTimeSec;
	const float fxSecElapsed = g_OLCLen - m_OLCTimer;

	kbParticleManager::CustomParticleAtlasInfo_t ParticleInfo;
	ParticleInfo.m_Type = BT_FaceCamera;
	ParticleInfo.m_Position = m_pLocalPlayer->GetPosition() + kbVec3( 0.0f, 0.0f, 900.0f );
	ParticleInfo.m_Direction = kbVec3( 0.0f, 1.0f, 0.0f );
	ParticleInfo.m_Width = 1024;
	ParticleInfo.m_Height = 4096;

	const float randV = 0.01f + kbfrand() * 0.23f;
	ParticleInfo.m_UVs[0].Set( 0.625f, randV );
	ParticleInfo.m_UVs[1].Set( 0.875, randV );

	kbVec3 finalColor( 1.0f, 1.0f, 1.0f );

	// Wind up
	const float STAGE_1_LEN = 5.0f;

	// Accelerate
	const float STAGE_2_START = STAGE_1_LEN;
	const float STAGE_2_LEN = 0.1f;
	const float STAGE_2_END = STAGE_1_LEN + STAGE_2_LEN;

	// Flash
	const float STAGE_3_START = STAGE_2_END;
	const float STAGE_3_LEN = 0.5f;
	const float STAGE_3_END = STAGE_2_END + STAGE_3_LEN;

	// Big Blast
	const float STAGE_4_START = STAGE_3_END;
	const float STAGE_4_LEN = 3.0f;
	const float STAGE_4_END = STAGE_3_END + STAGE_4_LEN;

	// Cool down
	const float STAGE_5_START = STAGE_4_END;
	const float STAGE_5_LEN = 5.0f;
	const float STAGE_5_END = STAGE_4_END + STAGE_5_LEN;

	if ( fxSecElapsed < STAGE_1_LEN ) {

		// Wind up
		const float MaxWidth = 4096.0f;
		const float MinWidth = 512.0f; 
		const float intensity = fxSecElapsed / STAGE_1_LEN;
		finalColor.Set( intensity, intensity, intensity );
		ParticleInfo.m_Width = MinWidth + ( MaxWidth - MinWidth ) * ( 1.0f - intensity );;

		const float playSoundTime = STAGE_1_LEN * 0.05f;
		if ( fxSecElapsed - DeltaTimeSec < playSoundTime && fxSecElapsed >= playSoundTime ) {
			m_SoundManager.PlayWave( m_pOLCWindupWave, 1.0f );
		}

	} else if ( fxSecElapsed < STAGE_2_END ) {

		// Accelerate
		const float MaxWidth = 512.0f;
		const float MinWidth = 0.0f;

		const float intensity = ( fxSecElapsed - STAGE_1_LEN ) / STAGE_2_LEN;
		float width = ( MaxWidth - MinWidth ) * ( 1.0f - intensity );
		ParticleInfo.m_Width = width;
	} else if ( fxSecElapsed < STAGE_3_END ) {

		// Flash
		ParticleInfo.m_Width = 800.0f;
		const float midPt = (  STAGE_2_END + STAGE_3_END ) * 0.5f;
		if ( fxSecElapsed < midPt ) {
			m_OLCPostProcess = ( fxSecElapsed - STAGE_2_END ) / ( STAGE_3_LEN * 0.5f );
		} else {
			m_OLCPostProcess = 1.0f - ( fxSecElapsed - midPt ) / ( STAGE_3_LEN * 0.5f );
		}

		if ( fxSecElapsed - DeltaTimeSec <= midPt && fxSecElapsed > midPt ) {
			m_SoundManager.PlayWave( m_pOLCExplosion, 1.0f );
		}

	} else {

		ParticleInfo.m_Width = 1024.0f + kbfrand() * 128.0f;
		m_OLCPostProcess = -1.0f;

		if ( fxSecElapsed < STAGE_4_END ) {
			for ( int i = 0; i < g_pEtherGame->GetGameEntities().size(); i++ ) {
				kbGameEntity *const pCurEnt = g_pEtherGame->GetGameEntities()[i];
				EtherAIComponent *const pAI = (EtherAIComponent*) pCurEnt->GetComponentByType( EtherAIComponent::GetType() );
				if ( pAI == nullptr ) {
					continue;
				}

				RemoveGameEntity( pCurEnt );
			}
		}
	}

	EtherWorldGenCollision_t hitInfo;
	if ( m_pWorldGenComponent->TraceAgainstWorld( hitInfo, ParticleInfo.m_Position + kbVec3( 0.0f, 10000.0f, 0.0f ), ParticleInfo.m_Position - kbVec3( 0.0f, 10000.0f, 0.0f ), false ) ) {

		float warpIntensity = 0.0f;
		float timeScale = 2.0f;

		kbVec3 redTint( 1.0f, 10.0f / 256.0f, 20.0f/ 256.0f );
		redTint *= 2.0f;
		if ( fxSecElapsed < STAGE_3_END ) {
			// Increase up to the flash
			warpIntensity = 2 * ( fxSecElapsed / STAGE_3_END );
			timeScale *= 3;
		} else if ( fxSecElapsed < STAGE_4_END ) {
			// Damage
			warpIntensity = 2.0f;
			timeScale = 8.0f * warpIntensity;
			m_OLCTint = redTint * warpIntensity * 0.05f;
		} else {
			warpIntensity = 2.0f * ( 1.0f - ( fxSecElapsed - STAGE_5_START ) / STAGE_5_LEN );
			timeScale = 16.0f;// * warpIntensity;
			finalColor.Set( warpIntensity, warpIntensity, warpIntensity );
			finalColor *= 0.5f;
			const float MaxWidth = 4096.0f;
			const float MinWidth = 1024.0f;
			ParticleInfo.m_Width  = ( MaxWidth - MinWidth ) * ( 2.0f - warpIntensity ) + MinWidth;

			float tintEndTime = 1.0f;
			float tintAdjust = 1.0f - kbClamp( ( fxSecElapsed - STAGE_5_START ) / tintEndTime, 0.0f, 1.0f );
			m_OLCTint = redTint * warpIntensity * tintAdjust * 0.05f;
		}

warpIntensity = kbClamp( warpIntensity, 0.0f, 10.0f );

//	kbLog( "Particle Width = %f", ParticleInfo.m_Width );
		//warpIntensity = kbClamp( warpIntensity, 0.0f, 1.0f );
		const float warpAmp = 16.0f * warpIntensity;
		const float radius = 1024.0f * warpIntensity;

	 	m_pWorldGenComponent->SetTerrainWarp( hitInfo.m_HitLocation, warpAmp, radius, timeScale );
	}

	ParticleInfo.m_Color = finalColor;
	this->m_pParticleManager->AddQuad( 0, ParticleInfo );
}

/**
 *	EtherGameRenderHook::RegisterHit
 */
void EtherGame::RegisterHit( kbComponent *const pHitComponent, const kbVec3 & hitLocation, const kbVec3 & hitDirection ) {
	hits newHit;
	newHit.hitDirection = hitDirection;
	newHit.hitLocation = hitLocation;
	newHit.pHitComponent = pHitComponent;
	m_Hits.push_back(newHit);
}

/**
 *	EtherGameRenderHook::RenderThreadCallBack
 */
void EtherGame::RenderThreadCallBack() {

}

/**
 *	EtherGameRenderHook::RenderSync
 */
void EtherGame::RenderSync() {
	if ( m_pBulletHoleTarget == nullptr ) {
		m_pBulletHoleTarget = g_pRenderer->RT_GetRenderTexture( 1024, 1024, eTextureFormat::KBTEXTURE_R8G8B8A8 );
		g_pRenderer->RT_ClearRenderTarget( m_pBulletHoleTarget, kbColor( 0.f, 0.f, 0.f, 0.f ) );
		
		for ( int i = 0; i < GetGameEntities().size(); i++ ) {
			kbGameEntity *const pEnt = GetGameEntities()[i];
			if ( pEnt->GetName().find( "Holey_Wall" ) != std::string::npos ) {
				kbStaticModelComponent *const pSM = (kbStaticModelComponent*)pEnt->GetComponentByType( kbStaticModelComponent::GetType() );
				if ( pSM != nullptr ) {

					kbShaderParamOverrides_t shaderParams;
					shaderParams.SetTexture( "holeTex", m_pBulletHoleTarget );
					pSM->SetShaderParams( shaderParams );
				}
				break;
			}
		}
	}

	kbShader *const pUnwrapShader = (kbShader*)g_ResourceManager.GetResource( "./assets/shaders/pokeyholeunwrap.kbshader", true );
	for ( int i = 0; i < m_Hits.size(); i++ ) {
		kbGameEntity *const pEnt = (kbGameEntity*)m_Hits[i].pHitComponent->GetOwner();
		kbStaticModelComponent *const pSM = (kbStaticModelComponent*)pEnt->GetComponentByType( kbStaticModelComponent::GetType() );
		if ( pSM == nullptr ) {
			continue;
		}
		g_pRenderer->RT_SetRenderTarget( m_pBulletHoleTarget );
		kbShaderParamOverrides_t shaderParams;

		kbMat4 invWorldMatrix;
		invWorldMatrix.MakeScale( pEnt->GetScale() );
		invWorldMatrix *= pEnt->GetOrientation().ToMat4();
		invWorldMatrix[3] = pEnt->GetPosition();
		invWorldMatrix.InvertFast();

		kbVec3 hitLocation = invWorldMatrix.TransformPoint( m_Hits[i].hitLocation );
		kbVec3 hitDir = m_Hits[i].hitDirection * invWorldMatrix;

		shaderParams.SetTexture( "baseTexture", pSM->GetModel()->GetMaterials()[0].GetTextureList()[0] );
		shaderParams.SetVec4( "hitLocation", kbVec4( hitLocation.x, hitLocation.y, hitLocation.z, 1.0f ) );
		shaderParams.SetVec4( "hitDirection", kbVec4( hitDir.x, hitDir.y, hitDir.z, 1.0f ) );
		shaderParams.SetTexture( "holeTex", m_pBulletHoleTarget );
		pSM->SetShaderParams( shaderParams );


		g_pRenderer->RT_RenderMesh( pSM->GetModel(), pUnwrapShader, &shaderParams );
	}
}