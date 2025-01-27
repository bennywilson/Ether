//===================================================================================================
// DQuadAI.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "kbNetworkingManager.h"
#include "DQuadGame.h"
#include "DQuadAI.h"
#include "DQuadPlayer.h"
#include "DQuadSkelModel.h"

DQuadAIManager g_AIManager;

/**
 *	kbDQuadAIComponent::Constructor
 */
void kbDQuadAIComponent::Constructor() {

	m_BaseMoveSpeed = 200.0f;
}

/**
 *	kbDQuadAIComponent::Update
 */
void kbDQuadAIComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

	// Place on the ground
	if ( g_pGame->IsPlaying() ) {
		PlaceOnGround();
	}

	//g_pRenderer->DrawSphere( m_pParent->GetPosition(), 32.0f, 12, kbColor::red );
	/*static float movementMag = 1.0f;

	m_TimeAlive += DeltaTime;
	const float angle = fmod( m_TimeAlive * movementMag, kbPI * 2.0f );
	kbMat4 mat = kbMat4::identity;
	mat[0][0] = cos(angle);
	mat[0][1] = sin(angle);
	mat[2][0] = -sin(angle);
	mat[2][2] = cos(angle);

	kbVec3 startingVec( 0.0f, 0.0f, 500.0f );
	startingVec = startingVec * mat;
	m_pParent->SetPosition( kbVec3( startingVec.x, 0.0f, startingVec.z ) );

	m_pParent->SetOrientation( kbQuatFromMatrix( mat ).Normalize() );*/

}

/**
 *	kbEnemyAIComponent::Constructor
 */
void kbEnemyAIComponent::Constructor() {
	m_AIState = Enemy_Idle;

	m_PursueMinDistance = 120.0f;

	m_TargetLocation.Set( 0.0f, 0.0f, 0.0f );
	m_TargetRotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
}

/**
 *	kbEnemySoldierAIComponent::NetUpdate
 */
void kbEnemySoldierAIComponent::NetUpdate( const kbNetMsg_t * NetMsg ) {
	PlaceOnGround();
}

/**
 *	kbEnemySoldierAIComponent::Update
 */
void  kbEnemyAIComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

	if ( kbNetworkingManager::GetHostType() != HOST_CLIENT ) {
		switch( m_AIState ) {
			case Enemy_Pursue : {
				State_Pursue( DeltaTime );
				break;
			}

			case Enemy_CircleStrafe : {
				State_CircleStrafe( DeltaTime );
				break;
			}
		}
	}


	if ( m_TargetList.size() > 0 ) {

		kbGameEntity *const myTarget = g_pGame->GetGameEntitybyId( m_TargetList[0] );
		if ( myTarget != NULL ) {
			kbVec3 vecToTarget = myTarget->GetPosition() - m_pParent->GetPosition();

			if ( vecToTarget.Compare( kbVec3::zero ) == false ) {
				vecToTarget.Normalize();
				if ( vecToTarget.Compare( kbVec3( 0.0f, 1.0f, 0.0f ) ) == false ) {
					const kbVec3 rightVec = kbVec3( 0.0f, 1.0f, 0.0f ).Cross( vecToTarget ).Normalized();
					const kbVec3 forwardVec = rightVec.Cross( kbVec3( 0.0f, 1.0f, 0.0f ) ).Normalized();
					const kbMat4 facingMatrix( rightVec, kbVec3( 0.0f, 1.0f, 0.0f ), forwardVec, kbVec3::zero );
					m_pParent->SetOrientation( kbQuatFromMatrix( facingMatrix ) );
				}
			}
		}
	}


	if ( kbNetworkingManager::GetHostType() == HOST_SERVER ) {
		if ( GetParent()->IsDirty() ) {
			kbActorPositionNetMsg_t posNetMsg;
			posNetMsg.m_ActorId = GetParent()->GetNetId();
			posNetMsg.m_bNotifyGame = true;
			posNetMsg.m_NewPos = GetParent()->GetPosition();
			posNetMsg.m_Rotation = GetParent()->GetOrientation();
			g_pNetworkingManager->QueueNetMessage( -1, &posNetMsg );
		}

		return;
	}

	// Search for "Additional Cloth Bones" and draw the hair locks for this AI using axial bill boards
	kbClothComponent * pClothComponent = NULL;

	for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
		if ( m_pParent->GetComponent(i)->IsA( kbClothComponent::GetType() ) ) {
			pClothComponent = static_cast<kbClothComponent*>( m_pParent->GetComponent(i) );
		}
	}

	if ( pClothComponent != NULL ) {
		
		const std::vector<class kbClothBone> &	ClothBoneInfo = pClothComponent->GetBoneInfo();
		const std::vector<kbClothMass_t> & ClothMasses = pClothComponent->GetMasses();

		for ( int iMass = (int)ClothBoneInfo.size(); iMass < ClothMasses.size() - 1; iMass++ ) {
			if ( ClothMasses[iMass + 1].m_bAnchored ) {
				continue;
			}

			const kbVec3 midPt = ( ClothMasses[iMass].GetPosition() + ClothMasses[iMass+1].GetPosition() ) * 0.5f;
			const float distance = ( ClothMasses[iMass].GetPosition() - ClothMasses[iMass+1].GetPosition() ).Length();
			const kbVec3 direction = ( ClothMasses[iMass+1].GetPosition() - ClothMasses[iMass].GetPosition() ) / distance; 

			kbParticleManager::CustomParticleInfo_t ParticleInfo;
			ParticleInfo.m_Position = midPt;
			ParticleInfo.m_Direction = direction;
			ParticleInfo.m_Color.Set( 0.25f, 0.25f, 0.25f );
			ParticleInfo.m_Width = distance;
			ParticleInfo.m_Height = 4.0f;
			ParticleInfo.m_UVs[0].Set( 0.25f, 0.0f );
			ParticleInfo.m_UVs[1].Set( 0.25f + 0.125f, 0.125f );
			ParticleInfo.m_Type = kbParticleManager::CPT_AxialBillboard;
 
			g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
		}
	}
}

/**
 *	kbEnemySoldierAIComponent::Constructor
 */
void kbEnemySoldierAIComponent::Constructor() {
	m_AIState = Enemy_Pursue;
	m_pEyeBall = NULL;
	m_bEyeballAdded = FALSE;
}

/**
 *	kbEnemySoldierAIComponent::SetEnable_Internal
 */
void kbEnemySoldierAIComponent::SetEnable_Internal( const bool isEnabled ) {
	if ( m_pEyeBall == NULL ) {
		return;
	}

	if ( kbNetworkingManager::GetHostType() != HOST_SERVER ) {
		if ( isEnabled && m_bEyeballAdded == false ) {
			g_pRenderer->AddRenderObject( this, m_pEyeBall, m_pParent->GetPosition(), m_pParent->GetOrientation(), m_pParent->GetScale(), RP_PostLighting, NULL );
			m_bEyeballAdded = true;
		} else {
			g_pRenderer->RemoveRenderObject( this );
			m_bEyeballAdded = false;
		}
	}
}

/**
 *	kbEnemySoldierAIComponent::Update
 */
void  kbEnemySoldierAIComponent::Update( const float DeltaTime ) {
	if ( m_pEyeBall != NULL ) {

		kbDQuadSkelModelComponent * pSkelModelComponent = NULL;

		for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
			if ( m_pParent->GetComponent(i)->IsA( kbDQuadSkelModelComponent::GetType() ) ) {
				pSkelModelComponent = static_cast<kbDQuadSkelModelComponent*>( m_pParent->GetComponent(i) );
			}
		}

		if ( pSkelModelComponent != NULL ) {
			static kbString EyeBone( "Eye" );
			kbBoneMatrix_t EyeMatrix;
			if ( pSkelModelComponent->GetBoneWorldMatrix( EyeBone, EyeMatrix ) ) {
				kbQuat rot;
				kbMat4 rotMat( kbMat4::identity );
				rotMat[0] = EyeMatrix.GetAxis(0);
				rotMat[1] = EyeMatrix.GetAxis(1);
				rotMat[2] = EyeMatrix.GetAxis(2);

				std::vector<kbShader *> ShaderOverrideList;
				ShaderOverrideList.push_back( (kbShader*)g_ResourceManager.GetResource(  "../../kbEngine/assets/Shaders/SimpleAdditive.kbShader", true ) );

				if ( m_bEyeballAdded == false ) {
					g_pRenderer->AddRenderObject( this, m_pEyeBall, m_pParent->GetPosition(), m_pParent->GetOrientation(), m_pParent->GetScale(), RP_PostLighting, &ShaderOverrideList );
				} else {
					g_pRenderer->UpdateRenderObject( this, EyeMatrix.GetOrigin(), m_pParent->GetOrientation(), m_pParent->GetScale(), &ShaderOverrideList );
				}
				m_bEyeballAdded = true;
			}
		}
	}

	Super::Update( DeltaTime );
}

/**
 *	kbEnemySoldierAIComponent::State_Pursue
 */
void kbEnemySoldierAIComponent::State_Pursue( const float DeltaTime ) {

	if ( g_pGame->GetPlayersList().size() == 0 ) {
		return;
	}

	kbVec3 goalOffset;
	const int targetId = g_AIManager.GetTarget( this, &goalOffset );
	const kbGameEntity *const MyTarget = g_pGame->GetGameEntitybyId( targetId );
	if ( MyTarget == NULL ) {
	//	m_AIState = Enemy_Idle;
		return;
	}

	m_TargetList.push_back( targetId );
	m_TargetLocation = MyTarget->GetPosition() + ( goalOffset * m_PursueMinDistance );

	kbVec3 TargetDirection = ( m_TargetLocation - m_pParent->GetPosition() ).Normalized();
	kbVec3 ToPlayer = m_pParent->GetPosition() - MyTarget->GetPosition();
	const float DistToPlayer = ToPlayer.Normalize();

	if ( DistToPlayer <= m_PursueMinDistance / 2.0f ) {
		kbVec3 FinalPos = m_pParent->GetPosition() + ( ToPlayer * DeltaTime * m_BaseMoveSpeed );
		m_pParent->SetPosition( FinalPos );
		//m_AIState = Enemy_CircleStrafe;
	} 
	else if ( DistToPlayer < m_PursueMinDistance ) {

	} else {
		kbVec3 FinalPos = m_pParent->GetPosition() + ( TargetDirection * DeltaTime * m_BaseMoveSpeed );
		m_pParent->SetPosition( FinalPos );
	}
}

/**
 *	kbEnemySoldierAIComponent::State_CircleStrafe
 */
void kbEnemySoldierAIComponent::State_CircleStrafe( const float DeltaTime ) {
	if ( m_TargetList.size() == 0 ) {
		return;
	}

	const kbGameEntity *const MyTarget = g_pGame->GetGameEntitybyId( m_TargetList[0] );
	if ( MyTarget == NULL ) {
		m_AIState = Enemy_Idle;
		return;
	}
	m_TargetLocation = MyTarget->GetPosition();

	kbVec3 TargetDirection = ( m_TargetLocation - m_pParent->GetPosition() );
	float DistToPlayer = TargetDirection.Normalize();
	if ( DistToPlayer > m_PursueMinDistance + 30.0f ) {
		m_AIState = Enemy_Pursue;
	}
}

/**
 *	DQuadAIManager::DQuadAIManager
 */
DQuadAIManager::DQuadAIManager() {
}

/**
 *	DQuadAIManager::RegisterActor
 */
void DQuadAIManager::RegisterActor( kbDQuadActorComponent *const actorComponent ) {

	if ( actorComponent == NULL || actorComponent->GetParent() == NULL ) {
		kbError( "DQuadAIManager::RegisterActor() - NULL actorComponent passed in" );
		return;
	}

	const int entityId = actorComponent->GetParent()->GetNetId();
	if ( m_TargetToEnemiesMap.find( entityId ) != m_TargetToEnemiesMap.end() ) {
		kbError( "DQuadAIManager::RegisterActor() - %s is already registered", actorComponent->GetParent()->GetName().c_str() );
		return;
	}

	m_TargetToEnemiesMap[entityId] = AITarget_t();
	m_TargetList.push_back( entityId );
}

/**
 *	DQuadAIManager::UnregisterActor
 */
void DQuadAIManager::UnregisterActor( kbDQuadActorComponent *const actorComponent ) {

	if ( actorComponent == NULL || actorComponent->GetParent() == NULL ) {
		kbError( "DQuadAIManager::UnregisterActor() - NULL actorComponent passed in" );
		return;
	}

	const int actorId = actorComponent->GetParent()->GetNetId();
	m_TargetToEnemiesMap.erase( actorId );
	RemoveItemFromVector( m_TargetList, actorId );

	if ( actorComponent->IsA( kbDQuadAIComponent::GetType() ) ) {

		const std::vector<int> & targetList = static_cast<kbDQuadAIComponent*>( actorComponent )->GetTargetList();
		for ( int i = 0; i < targetList.size(); i++ ) {
			std::map<int, AITarget_t>::iterator it = m_TargetToEnemiesMap.find( targetList[i] );
			if ( it != m_TargetToEnemiesMap.end() ) {
				for ( int strafeSpots = 0; strafeSpots < NUM_STRAFE_SPOTS; strafeSpots++ ) {
					if ( it->second.m_StrafeSpots[strafeSpots] == actorId ) {
						it->second.m_StrafeSpots[strafeSpots] = -1;
					}
				}
			}
		}
	}
}

/**
 *	DQuadAIManager::GetTarget
 */
int DQuadAIManager::GetTarget( kbDQuadActorComponent *const requester, kbVec3 * targetOffset ) {
	if ( requester == NULL ) {
		kbError( "DQuadAIManager::GetTarget() - NULL requestor passed in");
		return -1;
	}

	const int requesterId = requester->GetParent()->GetNetId();
	for ( int i = 0; i < m_TargetList.size(); i++ ) {
		const kbGameEntity *const pEntity = g_pGame->GetGameEntitybyId( m_TargetList[i] );
		if ( pEntity != NULL && pEntity->GetActorComponent() != NULL && pEntity->GetActorComponent()->IsA( kbDQuadPlayerComponent::GetType() ) ) {
			const int entityId = pEntity->GetNetId();
			std::map<int, AITarget_t>::iterator it = m_TargetToEnemiesMap.find( entityId );
			if ( it != m_TargetToEnemiesMap.end() ) {
				AITarget_t & targetInfo = it->second;
				int strafeSpot;

				for ( strafeSpot = 0; strafeSpot < NUM_STRAFE_SPOTS; strafeSpot++ ) {
					if ( targetInfo.m_StrafeSpots[strafeSpot] == -1 || targetInfo.m_StrafeSpots[strafeSpot] == requesterId ) {
						targetInfo.m_StrafeSpots[strafeSpot] = requesterId;
						
						if ( targetOffset != NULL ) {
							if ( strafeSpot == 0 ) {
								targetOffset->Set( 1.0f, 0.0f, 0.0f );
							} else if ( strafeSpot == 1 ) {
								targetOffset->Set( -1.0f, 0.0f, 0.0f );
							} else if ( strafeSpot == 2 ) {
								targetOffset->Set( 0.0f, 0.0f, 1.0f );
							} else {
								targetOffset->Set( 0.0f, 0.0f, -1.0f );
							}
						}
						return pEntity->GetNetId();
					}
				}
			}
		}
	}

	return -1;
}

/**
 *	DQuadAIManager::Update 
 */
void DQuadAIManager::Update( const float DeltaTime ) {

}
