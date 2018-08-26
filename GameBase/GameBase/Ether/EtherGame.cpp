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
	m_pBulletHoleTarget( nullptr ) {

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
}

/**
 *	EtherGame::StopGame_Internal
 */
void EtherGame::StopGame_Internal() {
	m_pPlayerComponent = nullptr;
	m_pLocalPlayer = nullptr;

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

	m_pParticleManager->SetCustomAtlasTexture( 0, "./assets/FX/fx_atlas.jpg" );

	m_pParticleManager->SetCustomAtlasTexture( 1, "./assets/FX/SmokeTrailAtlas.dds" );
	m_pParticleManager->SetCustomAtlasShader( 1, "./assets/shaders/shellTrailParticle.kbShader" );
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
		m_pBulletHoleTarget = g_pRenderer->RT_GetRenderTexture( 4096, 4096, eTextureFormat::KBTEXTURE_R8G8B8A8 );
		g_pRenderer->RT_ClearRenderTarget( m_pBulletHoleTarget, kbColor::white );
		
		g_ResourceManager.GetResource( "./assets/FX/noise.jpg", true );
		g_ResourceManager.GetResource( "./assets/FX/scorch.jpg", true );

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

	if ( GetAsyncKeyState( 'C' ) ) {
		g_pRenderer->RT_ClearRenderTarget( m_pBulletHoleTarget, kbColor::white );
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

		const kbVec3 hitLocation = invWorldMatrix.TransformPoint( m_Hits[i].hitLocation );
		const kbVec3 hitDir = m_Hits[i].hitDirection * invWorldMatrix;
		const float holeSize = 0.75f + ( kbfrand() * 0.5f );
		const float scorchSize = 3.0f + ( kbfrand() * 1.5f );

		shaderParams.SetTexture( "baseTexture", pSM->GetModel()->GetMaterials()[0].GetTextureList()[0] );
		shaderParams.SetVec4( "hitLocation", kbVec4( hitLocation.x, hitLocation.y, hitLocation.z, holeSize ) );
		shaderParams.SetVec4( "hitDirection", kbVec4( hitDir.x, hitDir.y, hitDir.z, scorchSize ) );

		kbTexture *const pNoiseTex = (kbTexture*)g_ResourceManager.GetResource( "./assets/FX/noise.jpg", true );
		shaderParams.SetTexture( "noiseTex", pNoiseTex );

		kbTexture *const pScorchTex = (kbTexture*)g_ResourceManager.GetResource( "./assets/FX/scorch.jpg", true );
		shaderParams.SetTexture( "scorchTex", pScorchTex );

		g_pRenderer->RT_RenderMesh( pSM->GetModel(), pUnwrapShader, &shaderParams );
	}
	m_Hits.clear();
}