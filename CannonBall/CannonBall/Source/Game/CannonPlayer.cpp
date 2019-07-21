//===================================================================================================
// CannonPlayer.cpp
//
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	CannonPlayerComponent::Constructor
 */
 void CannonPlayerComponent::Constructor() {
	m_MaxRunSpeed = 3.0f;
	m_MaxRotateSpeed = 15.0f;
}

 /**
  *	CannonPlayerComponent::HandleInput
  */
 void CannonPlayerComponent::HandleInput( const kbInput_t & input, const float DT ) {

	static kbVec3 lastMove( 0.0f, 0.0f, -1.0f );
	static const kbString Run_Anim( "Run_Basic" );
	static const kbString IdleL_Anim( "IdleLeft_Basic" );
	static const kbString IdleR_Anim( "IdleRight_Basic" );
	static const kbString PunchL_Anim( "PunchLeft_Basic" );
	static const kbString KickL_Anim( "KickLeft_Basic" );
	static const kbString PunchR_Anim( "PunchRight_Basic" );
	static const kbString KickR_Anim( "KickRight_Basic" );
	static const kbString CannonBall_Anim( "CannonBall" );

	static bool bIsPunching = false;
	static bool bIsCannonBalling = false;


	if ( bIsCannonBalling ) {
		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -1.0f, 0.0f, 0.0f ), kbVec3::up );		

		const kbQuat curRot = GetOwnerRotation();
		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		if ( m_SkelModelsList[0]->IsPlaying( CannonBall_Anim ) == true ) {
			return;
		}

		bIsCannonBalling = false;
	}

	if ( input.WasKeyJustPressed( 'C' ) ) {
		bIsCannonBalling = true;
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			kbString returnIdle = IdleL_Anim;
			if ( lastMove.z > 0 ) {
				returnIdle = IdleR_Anim;
			}
			pSkelModel->PlayAnimation( CannonBall_Anim, 0.08f, false, returnIdle, 0.05f );
		}
		return;
	}

	if ( bIsPunching ) {
		if ( m_SkelModelsList[0]->IsPlaying( PunchL_Anim ) == true || 
			 m_SkelModelsList[0]->IsPlaying( KickL_Anim ) == true ||
			m_SkelModelsList[0]->IsPlaying( PunchR_Anim ) == true || 
			 m_SkelModelsList[0]->IsPlaying( KickR_Anim ) == true ) {
			return;
		}
		bIsPunching = false;
	}
		
	if ( input.WasKeyJustPressed( kbInput_t::KB_SPACE ) ) {
		bIsPunching = true;
		kbString directionToAttackMap[][3] = {  { PunchL_Anim, KickL_Anim, IdleL_Anim },
												{ PunchR_Anim, KickR_Anim, IdleR_Anim } };

		const int dirIndex = ( lastMove.z < 0 ) ? ( 0 ) : ( 1 );

		const kbString AttackAnim = directionToAttackMap[dirIndex][rand() %2];
		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( AttackAnim, 0.08f, false, directionToAttackMap[dirIndex][2], 1.05f );
		}
		return;
	}

	kbVec3 moveVec( kbVec3::zero );
	if ( input.IsKeyPressedOrDown( 'A' ) ) {
		moveVec = kbVec3( 0.0f, 0.0f, -1.0f ).Normalized();
	} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
		moveVec = kbVec3( 0.0f, 0.0f, 1.0f ).Normalized();
	}

	if ( moveVec.Compare( kbVec3::zero ) == false ) {

		const kbVec3 targetPos = GetOwnerPosition() - moveVec * DT * m_MaxRunSpeed;
		GetOwner()->SetPosition( targetPos );
		lastMove = moveVec;

		const kbVec3 facingVec = moveVec + kbVec3( -0.1f, 0.0f, 0.0f );	// Nudge a little so slerp below rotates player towards the camera
		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + facingVec, kbVec3::up );

		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( Run_Anim, 0.08f, false );
		}
	} else {

		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		kbString idleAnimToPlay;
		if ( lastMove.z > 0 ) {
			facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -0.1f, 0.0f, 1.0f ), kbVec3::up );
			idleAnimToPlay = IdleR_Anim;
		} else {
			facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -0.1f, 0.0f, -1.0f ), kbVec3::up );
			idleAnimToPlay =IdleL_Anim;
		}
		
		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * m_MaxRotateSpeed ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( idleAnimToPlay, 0.08f, false );
		}
	}
 }

/**
 *	CannonPlayerComponent::SetEnable_Internal
 */
void CannonPlayerComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_SkelModelsList.clear();
	if ( bEnable ) {
		const int NumComponents = (int)GetOwner()->NumComponents();
		for ( int i = 0; i < NumComponents; i++ ) {
			kbComponent *const pComponent = GetOwner()->GetComponent(i);
			if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) == false ) {
				continue;
			}
			m_SkelModelsList.push_back( (kbSkeletalModelComponent*)pComponent );
		}
	}
}

/**
 *	CannonPlayerComponent::Update_Internal
 */
void CannonPlayerComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );
}

/**
 *	CannonCameraComponent::Constructor
 */
void CannonCameraComponent::Constructor() {
	m_MoveMode = MoveMode_Follow;
	m_Offset.Set( 0.0f, 0.0f, 0.0f );
	m_pTarget = nullptr;

	m_NearPlane = 1.0f;
	m_FarPlane = 20000.0f;
}

/**
 *	CannonCameraComponent::SetEnable_Internal
 */
void CannonCameraComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_pTarget = nullptr;
	g_pRenderer->SetNearFarPlane( nullptr, m_NearPlane, m_FarPlane );
	if ( bEnable ) {
		switch( m_MoveMode ) {
			case MoveMode_None : {
			}
			break;

			case MoveMode_Follow : {
			}
			break;
		}
	}
}

/**
 *	CannonCameraComponent::FindTarget
 */
void CannonCameraComponent::FindTarget() {
	
	if ( g_UseEditor ) {
		extern kbEditor * g_Editor;
		std::vector<kbEditorEntity *> &	gameEnts = g_Editor->GetGameEntities();
		for ( int i = 0; i < gameEnts.size(); i++ ) {
			const kbGameEntity *const pEnt = gameEnts[i]->GetGameEntity();
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonPlayerComponent::GetType() );
			if ( pComp != nullptr ) {
				m_pTarget = (kbGameEntity*)pComp->GetOwner();
				break;
			}
		}
	} else {
		const std::vector<kbGameEntity*> & GameEnts = g_pGame->GetGameEntities();
		for ( int i = 0; i < (int) GameEnts.size(); i++ ) {
			const kbGameEntity *const pEnt = GameEnts[i];
			const kbComponent *const pComp = pEnt->GetComponentByType( CannonPlayerComponent::GetType() );
			if ( pComp != nullptr ) {
				m_pTarget = (kbGameEntity*)pComp->GetOwner();
				break;
			}
		}
	}

}

/**
 *	CannonCameraComponent::Update_Internal
 */
void CannonCameraComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	switch( m_MoveMode ) {
		case MoveMode_None : {
				
		}
		break;

		case MoveMode_Follow : {
			if ( m_pTarget == nullptr ) {
				FindTarget();
				if ( m_pTarget == nullptr ) {
					break;
				}
			}

			const kbVec3 cameraDestPos = m_pTarget->GetPosition() + m_Offset;
			GetOwner()->SetPosition( cameraDestPos );

			kbMat4 cameraDestRot;
			cameraDestRot.LookAt( GetOwner()->GetPosition(), m_pTarget->GetPosition(), kbVec3::up );
			cameraDestRot.InvertFast();
			GetOwner()->SetOrientation( kbQuatFromMatrix( cameraDestRot ) );
		}
		break;
	}
}
 