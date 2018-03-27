//===================================================================================================
// DQuadGame.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbMath.h"
#include "kbNetworkingManager.h"
#include "kbTypeInfo.h"
#include "DQuadGame.h"
#include "DQuadSkelModel.h"
#include "DQuadPlayer.h"
#include "DQuadAI.h"

// oculus
#include "OVR_Math.h"
using namespace OVR;

kbConsoleVariable g_NoEnemies( "noenemies", false, kbConsoleVariable::Console_Bool );

/**
 *	DQuadGame::DQuadGame
 */
DQuadGame::DQuadGame() :
	m_pWorldGenComponent( NULL ),
	m_CameraMode( Cam_FirstPerson ),
	m_pCharacterPackage( NULL ),
	m_pWeaponsPackage( NULL ),
	m_pPlayerComponent( NULL ) {
	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );
}

/**
 *	DQuadGame::~DQuadGame
 */
DQuadGame::~DQuadGame() {
}

/**
 *	DQuadGame::PlayGame_Internal
 */
void DQuadGame::PlayGame_Internal() {

}

/**
 *	DQuadGame::InitGame_Internal
 */
void DQuadGame::InitGame_Internal() {

	m_pCharacterPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Characters.kbPkg" );
	if ( m_pCharacterPackage == NULL ) {
		kbError( "Unable to find character package." );
	}

	m_pWeaponsPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Weapons.kbPkg" );
	if ( m_pWeaponsPackage == NULL ) {
		kbError( "Unable to find weapons package." );
	}

	m_pFXPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/FX.kbPkg" );
	if ( m_pFXPackage == NULL ) {
		kbError( "Unable to find FX package." );
	}

	const kbPrefab *const pBaseMuzzleFlashPrefab = m_pFXPackage->GetPrefab( "BaseMuzzleFlash" );
	if ( pBaseMuzzleFlashPrefab != NULL && pBaseMuzzleFlashPrefab->GetGameEntity(0) != NULL ) {
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
}

/**
 *	DQuadGame::StopGame_Internal
 */
void DQuadGame::StopGame_Internal() {
	m_pPlayerComponent = NULL;
	m_pLocalPlayer = NULL;
}

/**
 *	DQuadGame::LevelLoaded_Internal
 */
void DQuadGame::LevelLoaded_Internal() {
	if ( kbNetworkingManager::GetHostType() == HOST_SINGLEPLAYER ) {
		CreatePlayer( 0, m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID(), kbVec3::zero );
	}

	m_pParticleManager->SetCustomParticleTextureAtlas( "./assets/FX/fx_atlas.jpg" );
}

/**
 *	DQuadGame::Update_Internal
 */
void DQuadGame::Update_Internal( float DT ) {

	if ( kbNetworkingManager::GetHostType() != HOST_SERVER ) {

		if ( IsConsoleActive() == false ) {
			ProcessInput( DT );
		}

		const kbMat4 gameCameraMatrix = m_Camera.m_Rotation.ToMat4();
		const kbVec3 rightVec = gameCameraMatrix[0].ToVec3();
		const kbVec3 forwardVec = kbVec3( gameCameraMatrix[2].ToVec3().x, 0.0f, gameCameraMatrix[2].ToVec3().z ).Normalized();

		float gameCameraYaw = acos( forwardVec.Dot( kbVec3::forward ) );
		if ( forwardVec.Dot( kbVec3::right ) >= 0 ) {
			gameCameraYaw = -gameCameraYaw;
		}
		
		if ( g_pRenderer != NULL && g_pRenderer->UsingHMD() ) {

			const ovrVector3f HmdToEyeViewOffset[2] = { -0.032000002f, 0.032000002f };	// = { m_EyeRenderDesc[0].HmdToEyeViewOffset, m_EyeRenderDesc[1].HmdToEyeViewOffset };
			const ovrFrameTiming ftiming = ovr_GetFrameTiming( g_pRenderer->GetHMD(), 0 );
			const ovrTrackingState hmdState = ovr_GetTrackingState( g_pRenderer->GetHMD(), ftiming.DisplayMidpointSeconds );

			ovrPosef eyeRenderPose[2];
			ovr_CalcEyePoses( hmdState.HeadPose.ThePose, HmdToEyeViewOffset, eyeRenderPose );

			kbVec3 eyePos[2];
			kbQuat eyeRot[2];

			for ( int eye = 0; eye < 2; eye++ ) {
				const Matrix4f rollPitchYaw = Matrix4f::RotationY( kbPI + gameCameraYaw );
				const Quatf orientation = eyeRenderPose[eye].Orientation;
				Matrix4f finalRollPitchYaw  = rollPitchYaw * Matrix4f(eyeRenderPose[eye].Orientation);
				finalRollPitchYaw.M[0][0] *= -1.0f;
				finalRollPitchYaw.M[0][1] *= -1.0f;
				finalRollPitchYaw.M[0][2] *= -1.0f;
		
				const Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0,1,0));
				const Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0,0,1));
				const Vector3f shiftedEyePos = rollPitchYaw.Transform(eyeRenderPose[eye].Position) + Vector3f( m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z );
	
				const Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos - finalForward, finalUp);
				memcpy( &m_Camera.m_EyeMats[eye], &view, sizeof( Matrix4f ) );
				m_Camera.m_EyeMats[eye].TransposeSelf();

				memcpy( &eyePos[eye], &eyeRenderPose[eye].Position, sizeof( kbVec3 ) );
				memcpy( &eyeRot[eye], &eyeRenderPose[eye].Orientation, sizeof( kbQuat ) );
			}

			g_pRenderer->SetRenderViewTransform( NULL, m_Camera.m_Position, m_Camera.m_Rotation, m_Camera.m_EyeMats, eyePos, eyeRot );

		} else {
			g_pRenderer->SetRenderViewTransform( NULL, m_Camera.m_Position, m_Camera.m_Rotation, NULL );
		}
/*
		std::string PlayerPos;
		PlayerPos += "x:";
		PlayerPos += std::to_string( ( long double ) m_Camera.m_Position.x );
		PlayerPos += " y:";
		PlayerPos += std::to_string( ( long double ) m_Camera.m_Position.y );
		PlayerPos += " z:";
		PlayerPos += std::to_string( ( long double ) m_Camera.m_Position.z );

		g_pRenderer->DrawDebugText( PlayerPos, 0, 0, g_DebugTextSize, 0.1f );*/
	}

	UpdateWorld( DT );
}

/**
 *	DQuadGame::AddGameEntity_Internal
 */
void DQuadGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {
	if ( pEntity == NULL ) {
		kbLog( "DQuadGame::AddGameEntity_Internal() - NULL Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer && m_pWorldGenComponent != NULL ) {
		kbWorldGenCollision_t HitInfo;
		m_pWorldGenComponent->TraceAgainstWorld( pEntity->GetPosition() + kbVec3( 0.0f, 10000.0f, 0.0f ), pEntity->GetPosition() - kbVec3( 0.0f, 10000.0f, 0.0f ), HitInfo, false );
		if ( HitInfo.m_bHitFound ) {
			m_Camera.m_Position.y = HitInfo.m_HitLocation.y + 256.0f;
			m_pLocalPlayer->SetPosition( m_Camera.m_Position );
		}
	}
}

/**
 *	DQuadGame::ProcessInput
 */
void DQuadGame::ProcessInput( const float DT ) {

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}

	static bool bCameraChanged = false;
	if ( GetAsyncKeyState( 'C' ) && GetAsyncKeyState( VK_CONTROL ) ) {
		if ( bCameraChanged == false ) {
			bCameraChanged = true;
			m_CameraMode = eCameraMode_t( 1 + (int)m_CameraMode );
			if ( m_CameraMode >= Cam_Free ) {
				m_CameraMode = Cam_FirstPerson;
			}

			if ( m_pLocalPlayer != NULL ) {
				for ( int i = 0; i < m_pLocalPlayer->NumComponents(); i++ ) {

					if ( m_pLocalPlayer->GetComponent(i)->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
						kbDQuadSkelModelComponent *const SkelComp = static_cast<kbDQuadSkelModelComponent*>( m_pLocalPlayer->GetComponent(i) );
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

	if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != NULL ) {
		m_pPlayerComponent->HandleMovement( GetInput(), DT );
	}

	m_Camera.Update();

	if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != NULL ) {
		m_pPlayerComponent->HandleAction( GetInput() );
	}
}

/**
 *	DQuadGame::TraceAgainstWorld
 */
bool DQuadGame::TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbVec3 & collisionPt, const bool bTraceAgainstDynamicCollision ) {
	if ( m_pWorldGenComponent == NULL ) {
		return false;
	}

	kbWorldGenCollision_t HitInfo;
	m_pWorldGenComponent->TraceAgainstWorld( startPt, endPt, HitInfo, bTraceAgainstDynamicCollision );
	if ( HitInfo.m_bHitFound ) {
		collisionPt = HitInfo.m_HitLocation;
	}

	return HitInfo.m_bHitFound;
}

/**
 *	DQuadGame::UpdateWorld
 */
void DQuadGame::UpdateWorld( const float DT ) {
	const double GameTimeInSeconds = m_GameStartTimer.TimeElapsedSeconds();

	g_AIManager.Update( DT );
	int numEntities = 0;
	for ( int iEntity = 0; iEntity < GetGameEntities().size(); iEntity++ ) {
		for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
			if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( kbEnemySoldierAIComponent::GetType() ) ) {
				numEntities++;
			}
		}
	}

	if ( g_NoEnemies.GetBool() ) {
		for ( int iEntity = (int)GetGameEntities().size() - 1; iEntity >= 0; iEntity-- ) {
			for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
				if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( kbEnemySoldierAIComponent::GetType() ) ) {
					g_pGame->RemoveGameEntity( GetGameEntities()[iEntity] );
				}
			}
		}
	}

	if ( GameTimeInSeconds > 20.0f && GetPlayersList().size() > 0 && kbNetworkingManager::GetHostType() != HOST_CLIENT && numEntities < 3 && GameTimeInSeconds > 0.0f && g_NoEnemies.GetBool() == false ) {
		const float OffsetRange = 3000.0f;
		const float xOffset = ( kbfrand() * OffsetRange ) - ( OffsetRange * 0.5f );
		const float zOffset = ( kbfrand() * OffsetRange ) - ( OffsetRange * 0.5f );

		const kbGameEntity * pGameEntity = NULL;
		if ( rand() % 2 == 0 ) {
			pGameEntity = m_pCharacterPackage->GetPrefab( "Cartel_Stalker" )->GetGameEntity(0);
		} else {
			pGameEntity = m_pCharacterPackage->GetPrefab( "Cartel_Warrior" )->GetGameEntity(0);
		}

		kbGameEntity *const pEntity = CreateEntity( pGameEntity, NetOwner_Server );
		pEntity->SetPosition( kbVec3( xOffset, 0.0f, zOffset ) );
	}
}

/**
 *	DQuadGame::CreatePlayer
 */
kbGameEntity * DQuadGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	kbGameEntity * pNewEntity = NULL;

	if ( prefabGUID == m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID() ) {
		// Create players

		kbLog( "Creating player with id %d", netId );

		pNewEntity = g_pGame->CreateEntity( m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0), NetOwner_LocalClient, true, netId );

		if ( pNewEntity->GetNetId() == g_pNetworkingManager->GetLocalPlayerId() ) {
			AddPrefabToEntity( m_pWeaponsPackage, "EL_Rifle", pNewEntity, false );
		}

		for ( int i = 0; i < pNewEntity->NumComponents(); i++ ) {
			kbComponent *const pCurComponent = pNewEntity->GetComponent(i);
			if ( pCurComponent->IsA( kbDQuadPlayerComponent::GetType() ) && pNewEntity->GetNetId() == g_pNetworkingManager->GetLocalPlayerId() ) {
				m_pPlayerComponent = static_cast<kbDQuadPlayerComponent*>( pCurComponent );
				kbLog( "		Setting m_pPlayerComponent" );

			} else if ( pCurComponent->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
				kbDQuadSkelModelComponent *const pSkelModel = static_cast<kbDQuadSkelModelComponent*>( pCurComponent );
				if ( pSkelModel->IsFirstPersonModel() != ( pNewEntity->GetNetId() == g_pNetworkingManager->GetLocalPlayerId() ) ) {
					pSkelModel->Enable( false );
				} else {
					pSkelModel->Enable( true );
				}
			}
		}

		// Place on ground
		if ( m_pWorldGenComponent != NULL ) {
			const kbVec3 desiredStartLocation = kbVec3::zero;
			kbWorldGenCollision_t HitInfo;
			if ( m_pWorldGenComponent->TraceAgainstWorld( desiredStartLocation + kbVec3( 0.0f, 10000.0f, 0.0f ), desiredStartLocation - kbVec3( 0.0f, 10000.0f, 0.0f ), HitInfo, false ) ) {
				pNewEntity->SetPosition( HitInfo.m_HitLocation );
			} else {
				pNewEntity->SetPosition( DesiredLocation );
			}
		}

		bool bIsLocalPlayer = false;
		if ( kbNetworkingManager::GetHostType() == HOST_SINGLEPLAYER || netId == g_pNetworkingManager->GetLocalPlayerId() ) {
			kbLog( "Setting local player to entity with id %d", netId );
			m_pLocalPlayer = pNewEntity;
			bIsLocalPlayer = true;
		}

	} else {

		pNewEntity = g_pGame->CreateEntity( g_ResourceManager.GetGameEntityFromGUID( prefabGUID ), NetOwner_Server, false, netId );
		pNewEntity->SetPosition( DesiredLocation );

		WriteToFile( "Creating %s with id %d", pNewEntity->GetName().c_str(), pNewEntity->GetNetId() );
		if ( kbNetworkingManager::GetHostType() == HOST_SERVER ) {
			g_pNetworkingManager->ServerCreateNetEntity( 1, pNewEntity, prefabGUID );
		}
	}

	return pNewEntity;
}

/**
 *	DQuadGame::AddPrefabToEntity
 */
void DQuadGame::AddPrefabToEntity( const kbPackage *const pPrefabPackage, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly ) {
	if ( pPrefabPackage == NULL ) {
		kbError( "DQuadGame::AddPrefabToEntity() - NULL prefab package found while searching for %s", prefabName.c_str() );
		return;
	}

	const kbPrefab *const pPrefab = pPrefabPackage->GetPrefab( prefabName );
	if ( pPrefab == NULL || pPrefab->GetGameEntity(0) == NULL ) {
		kbError( "DQuadGame::AddPrefabToEntity() - Null prefab or none Entities found with name %s", prefabName.c_str() );
		return;
	}

	const kbGameEntity *const pPrefabEntity = pPrefab->GetGameEntity(0);
	if ( bComponentsOnly == false ) {
		kbGameEntity *const newItem = new kbGameEntity( pPrefabEntity, false );
		pEntity->AddEntity( newItem );
		for ( int i = 0; i < pEntity->NumComponents(); i++ ) {
			if ( pEntity->GetComponent(i)->IsA( kbDQuadActorComponent::GetType() ) ) {
				static_cast<kbDQuadActorComponent*>( pEntity->GetComponent(i) )->SetEquippedItem( newItem );
			}
		}
		return;
	}

	for ( int iComp = 1; iComp < pPrefabEntity->NumComponents(); iComp++ ) {

		bool bShowComponent = true;
		const kbComponent *const pPrefabComponent = pPrefabEntity->GetComponent(iComp);
		if ( pPrefabComponent->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
			const kbDQuadSkelModelComponent *const skelModel = static_cast<const kbDQuadSkelModelComponent*>( pPrefabComponent );
			if ( skelModel->IsFirstPersonModel() && g_pGame->GetLocalPlayer() != pEntity ) {
				continue;
			}

			if ( skelModel->IsFirstPersonModel() == false && g_pGame->GetLocalPlayer() == pEntity ) {
				bShowComponent = false;
			}
		} else if ( pPrefabComponent->IsA( kbDQuadPlayerComponent::GetType() ) && pEntity != g_pGame->GetLocalPlayer() ) {
			continue;
		}

		kbComponent *const pNewComponent = pPrefabComponent->Duplicate();
		pEntity->AddComponent( pNewComponent );
		pNewComponent->Enable( false );
		if ( bShowComponent ) {
			pNewComponent->Enable( true );
		}

		if ( pEntity->GetNetId() == g_pNetworkingManager->GetLocalPlayerId() && pPrefabComponent->IsA( kbDQuadPlayerComponent::GetType() ) ) {
			m_pPlayerComponent = static_cast<kbDQuadPlayerComponent*>( pNewComponent );
		}
	}
}

/**
 *	kbGame::CustomNetPackageReceived_Internal
 */
void DQuadGame::CustomNetPackageReceived_Internal( const kbCustomNetMsg_t & netMsg ) {
	CustomNetPackageReceived_Internal( netMsg );
}

/**
 *	kbGame::ServerNewPlayerJoined
 */
void DQuadGame::ServerNewPlayerJoined( kbCreateActorNetMsg_t & NetMsg ) {
	NetMsg.m_ActorGUID = m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID();
}

/**
 *	DQuadGame::ClientNetMsgNotify
 */
void DQuadGame::ClientNetMsgNotify( const kbNetMsg_t *const NetMsg ) {
	if ( NetMsg == NULL ) {
		kbError( "NULL net msg" );
		return;
	}

	bool bHandled = false;
	if ( NetMsg->m_MessageType == NETMSG_ACTORPOSITION ) {
		const kbActorPositionNetMsg_t *const posNetMsg = static_cast<const kbActorPositionNetMsg_t*>( NetMsg );
		for ( int i = 0; i < g_pGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pEntity = g_pGame->GetGameEntities()[i];
			if ( pEntity->GetNetId() == posNetMsg->m_ActorId ) {
				if ( pEntity->GetActorComponent() != NULL && pEntity->GetActorComponent()->IsA( kbDQuadActorComponent::GetType() ) ) {
					kbDQuadActorComponent *const pActorComponent = static_cast<kbDQuadActorComponent*>( pEntity->GetActorComponent() );
					pActorComponent->NetUpdate( NetMsg );
					bHandled = true;
				}
				break;
			}
		}
	} else if ( NetMsg->m_MessageType == NETMSG_CUSTOM ) {
		const kbCustomNetMsg_t * customNetMsg = static_cast<const kbCustomNetMsg_t*>( NetMsg );
		const DQuadCustomNetMessageType_t DQuadMsgType = *((DQuadCustomNetMessageType_t*)customNetMsg->m_CustomBuffer);

		switch( DQuadMsgType ) {
			case DQuadMsg_SpawnActor : {
				const DQuadSpawnActorNetMsg_t * spawnMsg = (DQuadSpawnActorNetMsg_t*)customNetMsg->m_CustomBuffer;

				const kbGameEntity * pEntityPrefab = g_ResourceManager.GetGameEntityFromGUID( spawnMsg->m_PrefabGUID );
				kbGameEntity *const pEntity = CreateEntity( pEntityPrefab, NetOwner_OtherClient );
				pEntity->SetPosition( spawnMsg->m_Position );
				pEntity->SetOrientation( spawnMsg->m_Orientation );
				break;
			}

			case DQuadMsg_DamageActor : {
				const DQuadDamageActorNetMsg_t * damageMsg = (DQuadDamageActorNetMsg_t*)customNetMsg->m_CustomBuffer;
				kbGameEntity *const pEntity = this->GetGameEntitybyId( damageMsg->m_TargetNetId );
				if ( pEntity != NULL ) {
					// Entity has died, remove it
					kbCustomNetMsg_t customMsg;
					customMsg.m_bNotifyGame = true;
					DQuadDamageActorNetMsg_t damageMsg;
					damageMsg.m_bKilled = true;
					customMsg.SetCustomData( &damageMsg, sizeof( damageMsg ) );
					g_pNetworkingManager->QueueNetMessage( -1, &customMsg );
					RemoveGameEntity( pEntity );
				}

				break;
			}
		};
	}

	if ( bHandled == false ) {
		static int breakhere = 0;
		breakhere++;
	}
}

/**
 *	DQuadGame::ServerNetMsgNotify
 */
void DQuadGame::ServerNetMsgNotify( const kbNetMsg_t *const NetMsg ) {
	if ( NetMsg == NULL ) {
		kbError( "NULL net msg" );
		return;
	}

	if ( NetMsg->m_MessageType == NETMSG_CUSTOM ) {
		const kbCustomNetMsg_t * customNetMsg = static_cast<const kbCustomNetMsg_t*>( NetMsg );
		const DQuadCustomNetMessageType_t DQuadMsgType = *((DQuadCustomNetMessageType_t*)customNetMsg->m_CustomBuffer);

		switch( DQuadMsgType ) {
			case DQuadMsg_DamageActor : {
				const DQuadDamageActorNetMsg_t * damageMsg = (DQuadDamageActorNetMsg_t*)customNetMsg->m_CustomBuffer;
				kbGameEntity *const pEntity = GetGameEntitybyId( damageMsg->m_TargetNetId );
				if ( pEntity != NULL ) {
					kbCustomNetMsg_t customKillMsg;
					customKillMsg.m_bNotifyGame = true;
					DQuadDamageActorNetMsg_t killMsg;
					killMsg.m_bKilled = true;
					killMsg.m_TargetNetId = damageMsg->m_TargetNetId;
					customKillMsg.SetCustomData( &killMsg, sizeof( killMsg ) );
					g_pNetworkingManager->QueueNetMessage( -1, &customKillMsg );
					RemoveGameEntity( pEntity );

					kbDQuadActorComponent *const pActorComponent = (kbDQuadActorComponent*)pEntity->GetComponentByType( kbDQuadActorComponent::GetType() );
					if ( pActorComponent != NULL ) {
						g_AIManager.UnregisterActor( pActorComponent );
					}
				}
				break;
			}
		}
	}
}
