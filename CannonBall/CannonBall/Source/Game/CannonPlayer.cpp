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
	 m_Dummy = -1;
}

 /**
 *	CannonPlayerComponent::HandleInput
 */
 static float rotRate = 15.0f;
 void CannonPlayerComponent::HandleInput( const kbInput_t & input, const float DT ) {

	kbVec3 moveVec( kbVec3::zero );
	if ( input.IsKeyPressedOrDown( 'A' ) ) {
		moveVec = kbVec3( -0.1f, 0.0f, -1.0f ).Normalized();
	} else if ( input.IsKeyPressedOrDown( 'D' ) ) {
		moveVec = kbVec3( -0.1f, 0.0f, 1.0f ).Normalized();
	}

	static kbString Run_Anim( "Run_Basic" );
	static kbString Idle_Anim( "Idle_Basic" );

	if ( moveVec.Compare( kbVec3::zero ) == false ) {
		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + moveVec, kbVec3::up );

		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * rotRate ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( Run_Anim, 0.08f, false );
		}
	} else {

		const kbQuat curRot = GetOwnerRotation();

		kbMat4 facingMat;
		facingMat.LookAt( GetOwnerPosition(), GetOwnerPosition() + kbVec3( -1.0f, 0.0f, 0.0f ), kbVec3::up );

		const kbQuat targetRot = kbQuatFromMatrix( facingMat );
		GetOwner()->SetOrientation( curRot.Slerp( curRot, targetRot, DT * rotRate ) );

		for ( int i = 0; i < m_SkelModelsList.size(); i++ ) {
			kbSkeletalModelComponent *const pSkelModel = m_SkelModelsList[i];
			pSkelModel->PlayAnimation( Idle_Anim, 0.08f, false );
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
 