//===================================================================================================
// EtherPlayer.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "EtherGame.h"
#include "EtherPlayer.h"
#include "EtherSkelModel.h"
#include "EtherWeapon.h"
#include "DX11/kbRenderer_DX11.h"

kbConsoleVariable g_ShowPlayerCollision( "showcollision", false, kbConsoleVariable::Console_Bool, "Shows player collision", "" );
kbConsoleVariable g_GodMod( "god", false, kbConsoleVariable::Console_Bool, "God mode", "" );

const float RespawnZPenalty = 1200.0f;
const float g_GrenadeCoolDownSec = 3.0f;

/**
 *	EtherPlayerComponent::Constrcutor
 */
void EtherPlayerComponent::Constructor() {
	m_LastTimeHit = 0.0f;
	m_DeathStartTime = -1.0f;
	m_PlayerDummy = 0;
	m_bHasHitGround = false;

	m_NumStimPacks = 3;
	m_NumAirstrikes = 3;
	m_NumOLC = 3;

	m_pFPHands = nullptr;
	m_GrenadeCoolddownSec = 0.0f;
}

/**
 *	EtherPlayerComponent::TakeDamage
 */
void EtherPlayerComponent::TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {

	if ( g_GodMod.GetBool() ) {
		return;
	}

	if ( g_pEtherGame->IsInSlomoMode() ) {
		float desiredDamage = damageComponent->GetMaxDamage();
		desiredDamage *= 0.25f;
		m_CurrentHealth += damageComponent->GetMaxDamage() - desiredDamage;
	}

	Super::TakeDamage( damageComponent, attackerComponent );


	m_LastTimeHit = g_GlobalTimer.TimeElapsedSeconds();
}

/**
 *	EtherPlayerComponent::Action_Fire
 */
void EtherPlayerComponent::Action_Fire( const bool bActivatedThisFrame ) {
	kbGameEntity *const pWeapon = GetEquippedItem();
	if ( pWeapon == nullptr ) {
		return;
	}

	if ( g_pEtherGame->PressHighlightedButton() ) {
		return;
	}

	EtherWeaponComponent *const pWeaponComponent = ( EtherWeaponComponent* ) pWeapon->GetComponentByType( EtherWeaponComponent::GetType() );
	if ( pWeaponComponent != nullptr ) {
		if ( pWeaponComponent->Fire( bActivatedThisFrame ) ) {
			m_UpperBodyState = UBS_Firing;
		}
		return;
	}
}

/**
 *	EtherPlayerComponent::Action_ThrowGrenade
 */
void EtherPlayerComponent::Action_ThrowGrenade( const bool bActivatedThisFrame ) {

	if ( bActivatedThisFrame == false || m_pFPHands == nullptr || m_GrenadeCoolddownSec > 0.0f ) {
		return;
	}

	//m_pFPHands->Enable( true );
	m_pFPHands->PlayAnimation( kbString( "ThrowGrenade" ), -1, true );
	m_GrenadeCoolddownSec = g_GrenadeCoolDownSec;
}

/**
 *	EtherPlayerComponent::StartDeath
 */
void EtherPlayerComponent::StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) {
	m_DeathStartTime = g_GlobalTimer.TimeElapsedSeconds();

}

/**
 *	EtherPlayerComponent::Update_Internal
 */
static float grenadeOffset = 2.0f;
static float rechargeRate = 20.0f;
void EtherPlayerComponent::Update_Internal( const float DeltaTimeSec ) {
	Super::Update_Internal( DeltaTimeSec );

	// todo
	if ( m_pFPHands == nullptr ) {
		for ( int i = 0; i < GetOwner()->GetChildEntities().size(); i++ ) {
			kbGameEntity *const pCurEnt = GetOwner()->GetChildEntities()[i];
			if ( strstr( pCurEnt->GetName().c_str(), "Hands" ) != nullptr ) {
				EtherSkelModelComponent *const pSkelModel = (EtherSkelModelComponent*) pCurEnt->GetComponentByType( EtherSkelModelComponent::GetType() );
				m_pFPHands = pSkelModel;
				break;
			}
		}
	}
	//g_pRenderer->DrawDebugText( std::to_string( m_CurrentHealth ) + " / " + std::to_string( m_MaxHealth ), 0.25f, 0.25f, g_DebugTextSize, g_DebugTextSize, kbColor::blue );
	if ( IsDead() == false && g_GlobalTimer.TimeElapsedSeconds() > m_LastTimeHit + 2.0f ) {

		if ( m_CurrentHealth < m_MaxHealth ) {
	
			m_CurrentHealth = kbClamp( m_CurrentHealth + DeltaTimeSec * rechargeRate, 0.0f, m_MaxHealth );
		}
	}

	if ( g_ShowPlayerCollision.GetBool() ) {
		kbCollisionComponent *const pCollisionComponent = (kbCollisionComponent*)GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );

		if ( pCollisionComponent != nullptr ) {
			g_pRenderer->DrawSphere( GetOwner()->GetPosition(), pCollisionComponent->GetRadius(), 12, kbColor::red );
		}
	}

	if ( IsDead() ) {
		UpdateDeath( DeltaTimeSec );
	}

	if ( m_pFPHands != nullptr && m_pFPHands->IsEnabled() && m_GrenadeCoolddownSec > 0.0f ) {
		float prevCoolDownSec = m_GrenadeCoolddownSec;
		m_GrenadeCoolddownSec -= DeltaTimeSec;

		if ( prevCoolDownSec > g_GrenadeCoolDownSec - 0.2f && m_GrenadeCoolddownSec <= g_GrenadeCoolDownSec - 0.2f ) {
			kbGameEntity *const pParent = GetOwner();
			for ( int i = 0; i < pParent->GetChildEntities().size(); i++ ) {
				kbGameEntity *const pChild = pParent->GetChildEntities()[i];
				if ( pChild->GetName() == "EL_Grenade" ) {
					
					kbGameEntity *const newProjectile = g_pGame->CreateEntity( pChild );
		
					kbMat4 WeaponMatrix = g_pEtherGame->GetCrossHairLocalSpaceMatrix();
					kbQuat WeaponOrientation;
		
					kbVec3 throwPos = pParent->GetPosition() + kbVec3( 0.0f, 50.0f, 10.0f );
					const kbVec3 aimAtPoint = WeaponMatrix[3].ToVec3();
					const kbVec3 zAxis = ( aimAtPoint - throwPos ).Normalized();
					const kbVec3 xAxis = kbVec3::up.Cross( zAxis ).Normalized();
					const kbVec3 yAxis = zAxis.Cross( xAxis ).Normalized();
		 
					WeaponMatrix[0].Set( xAxis.x, xAxis.y, xAxis.z, 0.0f );
					WeaponMatrix[1].Set( yAxis.x, yAxis.y, yAxis.z, 0.0f );
					WeaponMatrix[2].Set( zAxis.x, zAxis.y, zAxis.z, 0.0f );
					WeaponMatrix[3].Set( 0.0f, 0.0f, 0.0f, 1.0f );
		
					WeaponOrientation = kbQuatFromMatrix( WeaponMatrix );
		
					newProjectile->SetPosition( throwPos + pParent->GetOrientation().ToMat4()[0].ToVec3() * grenadeOffset );
					newProjectile->SetOrientation( WeaponOrientation );
		
					EtherProjectileComponent *const pProjectileComponent = static_cast<EtherProjectileComponent*>( newProjectile->GetComponentByType( EtherProjectileComponent::GetType() ) );
					pProjectileComponent->Enable( true );
					if ( pProjectileComponent == nullptr ) {
						kbError( "EtherWeaponComponent::Fire - No Projectile Component found on the projectile entity :o" );
					} else {
						kbGameEntity * pParent = GetOwner();
						while( pParent->GetOwner() != nullptr ) {
							pParent = pParent->GetOwner();
						}
						pProjectileComponent->Launch();
					}
		
					break;
				}
			}
		}
/*
      ARAmbientProjectileEndPoint * pTarget = Cast<ARAmbientProjectileEndPoint>(*it);
      if (pTarget == NULL)
         continue;

      for (INT i = 0; i < pTarget->Targets.Num(); i++)
      {
         pTarget->Targets(i).totalFlightTime = 0;
         pTarget->Targets(i).tanLaunchAngle = 0;
         pTarget->Targets(i).cosLaunchAngle = 0;

         if (pTarget->Targets(i).Target == NULL)
         {
            debugf(TEXT( "%s has a NULL target at index %d"), *pTarget->GetName(), i);
            continue;
         }
		  
         pTarget->Targets(i).PathPoints.Empty();

         // Calculate Paths
         FVector vecToTarget2D = pTarget->Targets(i).Target->Location - pTarget->Location;
         vecToTarget2D.Z = 0.0f;

         const FLOAT x = vecToTarget2D.Size();
         const FLOAT z = pTarget->Location.Z - pTarget->Targets(i).Target->Location.Z;

         vecToTarget2D.Normalize();

         const FLOAT v = pTarget->Targets(i).InitialSpeed;
         const FLOAT vSqr = v * v;
         const FLOAT g = -10.0f;

         const FLOAT denominator = g * x;
         FLOAT numerator = (vSqr*vSqr) - ( g * (( g * x*x ) + ( 2 * z * vSqr ) ) );

         if ( numerator < 0 )
           continue;

         numerator = appSqrt( numerator );
         
         FLOAT rotationAngle = 0.0f;
         if ( denominator != 0.0f )
         {
            if (pTarget->Targets(i).UseHighArc)
               numerator = vSqr + numerator;
            else
               numerator = vSqr - numerator;

            rotationAngle = -appAtan(numerator / denominator);
            const FLOAT tanAngle = -numerator / denominator;
            const FLOAT cosAngle = appCos(rotationAngle);
            const FLOAT oneOverTwoVCosSqr = 1.0f / ( 2 * ( v*cosAngle * v*cosAngle ) );

            for (FLOAT l = 0.05f; l <= 1.0f; l += 0.05f)
            {
               const FLOAT horizontalDistance = x * l;
               const FLOAT height = ( horizontalDistance * tanAngle ) - ( -g * horizontalDistance*horizontalDistance ) * oneOverTwoVCosSqr;
               const FVector finalPoint = (vecToTarget2D * horizontalDistance) + FVector( 0.0f, 0.0f, height );

               pTarget->Targets(i).PathPoints.AddItem( pTarget->Location + finalPoint );
            }

            const FLOAT t = ( (v * cosAngle ) > 0 ) ? ( x / ( v * cosAngle ) ) : ( 0 );

            pTarget->Targets(i).totalFlightTime = t;
            pTarget->Targets(i).tanLaunchAngle = tanAngle;
            pTarget->Targets(i).cosLaunchAngle = cosAngle;

            // debugf(TEXT("T = %f, tan(Theta) = %f, cos(Theta) = %f"), t, tanAngle, cosAngle);
         }
      }

      if (pTarget->RenderingComponent != NULL && pTarget->RenderingComponent->HiddenEditor == FALSE)
      {
         pTarget->RenderingComponent->UpdateRenderProxy();
      }

      pTarget->Modify();
   }*/
	}
}

/**
 *	EtherPlayerComponent::UpdateDeath
 */
void EtherPlayerComponent::UpdateDeath( const float DeltaTimeSec ) {
	EtherGame *const pGame = (EtherGame*) g_pGame;
	const kbVec3 playerPos = GetOwner()->GetPosition();

	// Drop player to ground
	kbVec3 collisionPt;
	if ( pGame->TraceAgainstWorld( playerPos + kbVec3( 0.0f, 20000.0f, 0.0f ), playerPos - kbVec3( 0.0f, 20000.0f, 0.0f ), collisionPt, false ) ) {
		float startY = 20.0f;
		float endY = 5.0f;
		float normalizedTime = 1.0f - kbClamp( ( g_GlobalTimer.TimeElapsedSeconds() - m_DeathStartTime ) / 1.0f, 0.0f, 1.0f );
		collisionPt.y += ( startY - endY ) * normalizedTime + endY;
		GetOwner()->SetPosition( collisionPt );
	}

	// Respawn Player
	if ( g_GlobalTimer.TimeElapsedSeconds() > m_DeathStartTime + 5.0f ) {
		m_CurrentHealth = m_MaxHealth;
		kbVec3 respawnPos = GetOwner()->GetPosition();
		respawnPos.z -= RespawnZPenalty;

		if ( pGame->TraceAgainstWorld( respawnPos + kbVec3( 0.0f, 20000.0f, 0.0f ), respawnPos - kbVec3( 0.0f, 20000.0f, 0.0f ), respawnPos, false ) ) {
			GetOwner()->SetPosition( respawnPos + kbVec3( 0.0f, 20.0f, 0.0f ) );
		}
		pGame->GetAIManager().RegisterCombatant( this );
	}

	kbCamera & playerCamera = pGame->GetCamera();
	switch( pGame->GetCameraMode() ) {
		case Cam_FirstPerson : {
			playerCamera.m_Position = GetOwner()->GetPosition();
			break;
		}
	}
}

/**
 *	EtherPlayerComponent::HandleMovement
 */
void EtherPlayerComponent::HandleMovement( const kbInput_t & Input, const float DT ) {

	if ( IsDead() ) {
		return;
	}

	kbGameEntity *const localPlayer = g_pGame->GetLocalPlayer();
	if ( localPlayer != GetOwner() ) {
		return;
	}

	EtherSkelModelComponent * pSkelModelComponent = nullptr;
	
	for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
		if ( GetOwner()->GetComponent(i)->IsA( EtherSkelModelComponent::GetType() ) ) {
			pSkelModelComponent = static_cast<EtherSkelModelComponent*>( GetOwner()->GetComponent(i) );
		}
	}

	EtherGame *const pGame = static_cast<EtherGame*>( g_pGame );
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
	

	
	if ( movementVec.Compare( kbVec3::zero ) == false ) {
		bMoved = true;
		movementVec.y = 0.0f;
		movementVec.Normalize();
	
		const float PlayerSpeed = 190.0f;
		const float movementMag = DT * PlayerSpeed * 0.5f;//( Input.m_KeyStruct.m_Shift ? 20.5f : 0.5f );
		
		localPlayer->SetPosition( localPlayer->GetPosition() + movementVec * movementMag );

		if ( pSkelModelComponent != nullptr ) {
			const static kbString RunAnimName( "Run" );
			if ( pSkelModelComponent->IsPlaying( RunAnimName ) == false ) {
				pSkelModelComponent->PlayAnimation( RunAnimName, -1.0f, false );
			}
		}
	} else {
		if ( pSkelModelComponent != nullptr ) {
			const static kbString AimAnimName( "Aim" );
			if ( pSkelModelComponent->IsPlaying( AimAnimName ) == false ) {
				pSkelModelComponent->PlayAnimation( AimAnimName, -1.0f, false );
			}
		}
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

		if ( g_pD3D11Renderer->IsRenderingToHMD() ) {
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
		
		const kbQuat newPlayerOrientation = ( GetOwner()->GetOrientation() * xRotation ).Normalized();
		GetOwner()->SetOrientation( newPlayerOrientation );
	}	

	// Place the player on the ground asap
	bool bFirstFrameGroundHit = false;
	if ( m_bHasHitGround == false ) {
		const kbVec3 desiredStartLocation = kbVec3::zero;
		kbVec3 groundPt;
		if ( pGame->TraceAgainstWorld( desiredStartLocation + kbVec3( 0.0f, 10000.0f, 0.0f ), desiredStartLocation - kbVec3( 0.0f, 10000.0f, 0.0f ), groundPt, false ) ) {
			groundPt.y += 20.0f;
			GetOwner()->SetPosition( groundPt );
			m_bHasHitGround = true;
			bFirstFrameGroundHit = true;
		} 
	}
	

	if ( bMoved || bFirstFrameGroundHit ) {
		switch( pGame->GetCameraMode() ) {
			case Cam_FirstPerson : {
				playerCamera.m_Position = GetOwner()->GetPosition();
				playerCamera.m_RotationTarget = ( playerCamera.m_RotationTarget * yRotation * xRotation ).Normalized();
				playerCamera.m_Rotation = playerCamera.m_RotationTarget;
				GetOwner()->SetOrientation( playerCamera.m_RotationTarget );
				break;
			}

			case Cam_ThirdPerson : {

				kbMat4 mat =  GetOwner()->GetOrientation().ToMat4();
				kbVec3 toAxis = -mat[2].ToVec3();
				playerCamera.m_Position = GetOwner()->GetPosition() + toAxis * 50.0f;

				kbMat4 lookAtMat;	lookAtMat.LookAt( playerCamera.m_Position, GetOwner()->GetPosition(), kbVec3( 0.0f, 1.0f, 0.0f ) );
				lookAtMat.InvertFast();
			
				playerCamera.m_RotationTarget = kbQuatFromMatrix( lookAtMat );

				break;
			}
		}
	}
}

/**
 *	EtherPlayerComponent::HandleAction
 */
void EtherPlayerComponent::HandleAction( const kbInput_t & Input ) {
	if ( Input.LeftMouseButtonDown || Input.LeftMouseButtonPressed || Input.RightTrigger > 0.0f || Input.RightTriggerPressed ) {
		Action_Fire( Input.LeftMouseButtonPressed || Input.RightTriggerPressed );
	} else if ( Input.RightMouseButtonDown || Input.RightMouseButtonPressed || Input.RightTrigger > 0.0f || Input.RightTriggerPressed ) {
		Action_ThrowGrenade( Input.RightMouseButtonPressed || Input.RightTriggerPressed );
	}
}
