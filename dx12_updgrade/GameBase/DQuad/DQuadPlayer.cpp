//===================================================================================================
// DQuadPlayer.cpp
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbNetworkingManager.h"
#include "DQuadGame.h"
#include "DQuadPlayer.h"
#include "DQuadSkelModel.h"
#include "DQuadWeapon.h"

/**
 *	kbDQuadPlayerComponent::Constrcutor
 */
void kbDQuadPlayerComponent::Constructor() {
	m_LastMovementTime = -1.0f;
}

/**
 *	kbDQuadPlayerComponent::Action_Fire
 */
void kbDQuadPlayerComponent::Action_Fire() {
	kbGameEntity *const pWeapon = GetEquippedItem();
	if ( pWeapon == NULL ) {
		return;
	}

	for ( int i = 0; i < pWeapon->NumComponents(); i++ ) {
		if ( pWeapon->GetComponent(i)->IsA( kbDQuadWeaponComponent::GetType() ) ) {
			kbDQuadWeaponComponent *const weaponComponent = static_cast<kbDQuadWeaponComponent*>( pWeapon->GetComponent(i) );
			if ( weaponComponent->Fire() ) {
				m_UpperBodyState = UBS_Firing;
			}
			return;
		}
	}

}

/**
 *	kbDQuadPlayerComponent::HandleInput
 */
void kbDQuadPlayerComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

	if ( g_pNetworkingManager->GetLocalPlayerId() != GetParent()->GetNetId() && m_LastMovementTime > 0.0f ) {

		kbDQuadSkelModelComponent * pSkelModelComponent = NULL;
		for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
			if ( m_pParent->GetComponent(i)->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
				pSkelModelComponent = static_cast<kbDQuadSkelModelComponent*>( m_pParent->GetComponent(i) );
			}
		}

		if ( m_LastMovementTime + 0.35f >= g_GlobalTimer.TimeElapsedSeconds() ) {
			if ( pSkelModelComponent != NULL ) {
				const static kbString RunAnimName( "Run" );
				if ( pSkelModelComponent->IsPlaying( RunAnimName ) == false ) {
					pSkelModelComponent->PlayAnimation( RunAnimName, true );
				}
			}
		} else {
			const static kbString AimAnimName( "Aim" );
			if ( pSkelModelComponent->IsPlaying( AimAnimName ) == false ) {
				pSkelModelComponent->PlayAnimation( AimAnimName, true );
			}
		}
	}
}

/**
 *	kbDQuadPlayerComponent::HandleMovement
 */
void kbDQuadPlayerComponent::HandleMovement( const kbInput_t & Input, const float DT ) {
	if ( kbNetworkingManager::GetHostType() == HOST_SERVER ) {
		return;
	}

	kbGameEntity *const localPlayer = g_pGame->GetLocalPlayer();
	if ( localPlayer != m_pParent ) {
		return;
	}

	kbDQuadSkelModelComponent * pSkelModelComponent = NULL;
	
	for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
		if ( m_pParent->GetComponent(i)->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
			pSkelModelComponent = static_cast<kbDQuadSkelModelComponent*>( m_pParent->GetComponent(i) );
		}
	}


	DQuadGame *const pGame = static_cast<DQuadGame*>( g_pGame );
	kbCamera & playerCamera = pGame->GetCamera();
	const kbMat4 cameraMatrix = playerCamera.m_RotationTarget.ToMat4();
	const kbVec3 rightVec = cameraMatrix[0].ToVec3();
	const kbVec3 upVec = cameraMatrix[1].ToVec3();
	const kbVec3 forwardVec = cameraMatrix[2].ToVec3();
	kbQuat xRotation( 0.0f, 0.0f, 0.0f, 1.0f ), yRotation( 0.0f, 0.0f, 0.0f, 1.0f );
	bool bMoved = false;

	kbVec3 movementVec( kbVec3::zero );

	if ( Input.IsKeyPressedOrDown( 'W' ) || Input.IsKeyPressedOrDown( 'w' ) ) {
		movementVec += forwardVec;
	} else if ( Input.IsKeyPressedOrDown( 'S' ) || Input.IsKeyPressedOrDown( 's' ) ) {
		movementVec -= forwardVec;
	}

	if ( Input.IsKeyPressedOrDown( 'A' ) || Input.IsKeyPressedOrDown( 'a' ) ) {
		movementVec -= rightVec;
	} else if ( Input.IsKeyPressedOrDown( 'D' ) || Input.IsKeyPressedOrDown( 'd' ) ) {
		movementVec += rightVec;
	}

	movementVec += forwardVec * Input.LeftStickY;
	movementVec += rightVec * Input.LeftStickX;
	
/*
	if ( Input.m_KeyStruct.m_Up ) {
		movementVec += upVec;
	} else if ( Input.m_KeyStruct.m_Down ) {
		movementVec -= upVec;
	}*/
	
	if ( movementVec.Compare( kbVec3::zero ) == false ) {
		bMoved = true;
		movementVec.y = 0.0f;
		movementVec.Normalize();
	
		const float PlayerSpeed = 190.0f;
		const float movementMag = DT * PlayerSpeed * 0.5f;//( Input.m_KeyStruct.m_Shift ? 20.5f : 0.5f );
		localPlayer->SetPosition( localPlayer->GetPosition() + movementVec * movementMag );

		if ( pSkelModelComponent != NULL ) {
			const static kbString RunAnimName( "Run" );
			if ( pSkelModelComponent->IsPlaying( RunAnimName ) == false ) {
				pSkelModelComponent->PlayAnimation( RunAnimName, true );
			}
		}
	}

	// Place local player on the ground.
	kbVec3 CollisionPt;
	kbVec3 playerPos = m_pParent->GetPosition();
	if ( pGame->TraceAgainstWorld( playerPos + kbVec3( 0.0f, 1000000.0f, 0.0f ), playerPos - kbVec3( 0.0f, 100000.0f, 0.0f ), CollisionPt, false ) ) {

		if ( pGame->GetCameraMode() == Cam_FirstPerson ) {
			playerPos.y = CollisionPt.y + 20.0f;
		} else {
			playerPos.y = CollisionPt.y;
		}

		m_pParent->SetPosition( playerPos );
	}

	if ( abs( Input.MouseDeltaX ) > 0 || abs( Input.MouseDeltaY ) > 0 || Input.RightStickY != 0 || Input.RightStickX != 0 ) {

		bMoved = true;

		const float StickSensitivity = 350.0f;
		float DeltaX = Input.MouseDeltaX + ( Input.RightStickX * StickSensitivity * DT );
		float DeltaY = Input.MouseDeltaY + ( -Input.RightStickY * StickSensitivity * DT );
		const float rotationMagnitude = 0.01f;
		const float minPitch = 0.24f;	// where 0 is looking straight up and PI is looking straight down
		const float maxPitch = 2.9f;
		const float maxRotationAmount = kbPI / 6.0f;
		const float epsilon = 0.001f;

		float pitchDelta = kbClamp( DeltaY * -rotationMagnitude, -maxRotationAmount, maxRotationAmount );

		if ( g_pRenderer->UsingHMD() ) {
			pitchDelta = 0.0f;
		}

		float curRotationAboutX = acos(forwardVec.Dot(kbVec3::up));

		if ( pitchDelta < 0 ) {
			if ( curRotationAboutX > maxPitch ) {
				pitchDelta = curRotationAboutX - maxPitch - epsilon;
			}
		} else if ( pitchDelta > 0 ) {
			if ( curRotationAboutX < minPitch ) {
				pitchDelta = curRotationAboutX - minPitch + epsilon;
			}
		}


		xRotation.FromAxisAngle( kbVec3::up, DeltaX * -rotationMagnitude );
		yRotation.FromAxisAngle( rightVec, pitchDelta );
		
		const kbQuat newPlayerOrientation = ( m_pParent->GetOrientation() * xRotation ).Normalized();
		m_pParent->SetOrientation( newPlayerOrientation );
	}	

	if ( bMoved ) {
		switch( pGame->GetCameraMode() ) {
			case Cam_FirstPerson : {
				playerCamera.m_Position = m_pParent->GetPosition();
				playerCamera.m_RotationTarget = ( playerCamera.m_RotationTarget * yRotation * xRotation ).Normalized();
				playerCamera.m_Rotation = playerCamera.m_RotationTarget;
				m_pParent->SetOrientation( playerCamera.m_RotationTarget );
				break;
			}

			case Cam_ThirdPerson : {

				kbMat4 mat =  m_pParent->GetOrientation().ToMat4();
				kbVec3 toAxis = -mat[2].ToVec3();
				playerCamera.m_Position = m_pParent->GetPosition() + toAxis * 50.0f;

				kbMat4 lookAtMat;	lookAtMat.LookAt( playerCamera.m_Position, m_pParent->GetPosition(), kbVec3( 0.0f, 1.0f, 0.0f ) );
				lookAtMat.InvertFast();
			
				playerCamera.m_RotationTarget = kbQuatFromMatrix( lookAtMat );

				break;
			}
		}

		if ( bMoved && g_pNetworkingManager != NULL ) {

			kbActorPositionNetMsg_t posNetMsg;
			posNetMsg.m_ActorId = GetParent()->GetNetId();
			posNetMsg.m_bNotifyGame = true;
			posNetMsg.m_NewPos = GetParent()->GetPosition();
			posNetMsg.m_Rotation = GetParent()->GetOrientation();
			g_pNetworkingManager->QueueNetMessage( -1, &posNetMsg );
		}
	}
}

/**
 *	kbDQuadPlayerComponent::HandleAction
 */
void kbDQuadPlayerComponent::HandleAction( const kbInput_t & Input ) {
	if ( Input.LeftMouseButton || Input.RightTrigger > 0.0f ) {
		Action_Fire();
	}
}

/**
 *	kbDQuadPlayerComponent::NetUpdate
 */
void kbDQuadPlayerComponent::NetUpdate( const kbNetMsg_t * NetMsg ) {

	if ( NetMsg->m_MessageType == NETMSG_ACTORPOSITION ) {
		if ( g_pNetworkingManager->GetLocalPlayerId() != GetParent()->GetNetId() ) {
			kbMat4 playerOrientation = GetParent()->GetOrientation().ToMat4();
			kbVec3 facingDir = playerOrientation[2].ToVec3();
			facingDir.y = 0.0f;
			facingDir.Normalize();
			kbVec3 rightDir = kbVec3(0.0f, 1.0f, 0.0f ).Cross( facingDir ).Normalized();
			playerOrientation[0] = rightDir;
			playerOrientation[1] = kbVec3( 0.0f, 1.0f, 0.0f );
			playerOrientation[2] = facingDir;
			GetParent()->SetOrientation( kbQuatFromMatrix( playerOrientation ) );
			PlaceOnGround();

			m_LastMovementTime = g_GlobalTimer.TimeElapsedSeconds();
		}
	}
}
