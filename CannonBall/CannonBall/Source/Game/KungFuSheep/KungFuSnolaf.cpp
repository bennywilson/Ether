//===================================================================================================
// KungFuSnolaf.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuSnolaf.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	KungFuSnolafStateIdle
 */
template<typename T>
class KungFuSnolafStateIdle : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateIdle( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }


	virtual void BeginState( T ) override {

		static const kbString Idle_Anim( "Idle_1" );
		m_pActorComponent->PlayAnimation( Idle_Anim, 0.05f );
	}

	virtual void UpdateState() override {

		if ( GetTarget() != nullptr ) {
			RotateTowardTarget();

			if ( GetDistanceToTarget() > 2.0f ) {
				RequestStateChange( KungFuSnolafState::Run );
				return;
			}
		}
	}

	virtual void EndState( T ) override { }
};

/**
 *	KungFuSnolafStateRun
 */
template<typename T>
class KungFuSnolafStateRun : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateRun( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString Run_Anim( "Run" );
		m_pActorComponent->PlayAnimation( Run_Anim, 0.05f );

		GetSnolaf()->EnableSmallLoveHearts( true );
	}

	virtual void UpdateState() override {

		const float frameDT = g_pGame->GetFrameDT();

		if ( GetTarget() != nullptr ) {
			RotateTowardTarget();

			if ( GetDistanceToTarget() < 0.75f ) {
				RequestStateChange( KungFuSnolafState::Hug );
				return;
			} else {
				kbVec3 moveDir( 0.0f, 0.0f, 0.0f );
				if ( IsTargetOnLeft() ) {
					moveDir.z = 1.0f;
				} else {
					moveDir.z = -1.0f;
				}

				const kbVec3 targetPos = m_pActorComponent->GetOwnerPosition() + moveDir * frameDT * m_pActorComponent->GetMaxRunSpeed();
				m_pActorComponent->SetOwnerPosition( targetPos );
			}
		} else {
			RequestStateChange( KungFuSnolafState::Idle );
		}
	}

	virtual void EndState( T ) override {
		GetSnolaf()->EnableSmallLoveHearts( false );
	}
};

/**
 *	KungFuSnolafStateHug - Warm Hugs!
 */
template<typename T>
class KungFuSnolafStateHug : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateHug( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }

	virtual void BeginState( T ) override {

		static const kbString HugLeft_Anim( "Hug_Left" );
		static const kbString HugRight_Anim( "Hug_Right" );

		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;

		}

		if ( IsTargetOnLeft() ) {
			m_pActorComponent->PlayAnimation( HugLeft_Anim, 0.05f );
		} else {
			m_pActorComponent->PlayAnimation( HugRight_Anim, 0.05f );
		}

		GetSnolaf()->EnableLargeLoveHearts( true );
	}

	virtual void UpdateState() override {

		const float frameDT = g_pGame->GetFrameDT();
		
		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;
		}
			
		if ( GetDistanceToTarget() > 2.5f ) {
			RequestStateChange( KungFuSnolafState::Run );
			return;
		}

	}

	virtual void EndState( T ) override {
		GetSnolaf()->EnableLargeLoveHearts( false );
	}
};

/**
 *	KungFuSnolafStateDead
 */
template<typename T>
class KungFuSnolafStateDead : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateDead( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ), m_DeathStartTime( -1.0f ) { }

	virtual void BeginState( T ) override {
	//	kbLog( "Dead yo" );

		const int numDeaths = 2;
		const int deathSelection = rand() % numDeaths;

		if ( deathSelection == 0 ) {
			const kbString Death_FlyBackwards_1( "Death_FlyBackwards_1" );
			m_pActorComponent->PlayAnimation( Death_FlyBackwards_1, 0.05f );
		} else if ( deathSelection == 1 ) {
			KungFuSnolafComponent *const pSnolaf = m_pActorComponent->GetAs<KungFuSnolafComponent>();
			pSnolaf->DoPoofDeath();
		}

		m_DeathStartTime = g_GlobalTimer.TimeElapsedSeconds();
	/*	static const kbString HugLeft_Anim( "Hug_Left" );
		static const kbString HugRight_Anim( "Hug_Right" );

		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;

		}

		if ( IsTargetOnLeft() ) {
			m_pActorComponent->PlayAnimation( HugLeft_Anim, 0.05f );
		} else {
			m_pActorComponent->PlayAnimation( HugRight_Anim, 0.05f );
		}*

		GetSnolaf()->EnableLargeLoveHearts( true );*/
	}

	virtual void UpdateState() override {

		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		if ( curTime > m_DeathStartTime + 2.0f ) {
			g_pCannonGame->RemoveGameEntity( this->m_pActorComponent->GetOwner() );
			return;
		}
	/*	const float frameDT = g_pGame->GetFrameDT();
		
		if ( GetTarget() == nullptr ) {
			RequestStateChange( KungFuSnolafState::Idle );
			return;
		}
			
		if ( GetDistanceToTarget() > 2.5f ) {
			RequestStateChange( KungFuSnolafState::Run );
			return;
		}*/

	}

	virtual void EndState( T ) override {
	//	GetSnolaf()->EnableLargeLoveHearts( false );
	}

private:
	float m_DeathStartTime;
};


/**
 *	KungFuSnolafComponent::Constructor
 */
void KungFuSnolafComponent::Constructor() {
	m_pSmallLoveHearts = nullptr;
	m_pLargeLoveHearts = nullptr;
}

/**
 *	KungFuSnolafComponent::SetEnable_Internal
 */
void KungFuSnolafComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	// Make sure sheep package is loaded
	g_ResourceManager.GetPackage( "./assets/Packages/Snolaf.kbPkg" );

	m_pSmallLoveHearts = nullptr;
	if ( bEnable ) {
		if ( m_SkelModelsList.size() > 1 ) {
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		}

		static const kbString SmallLoveHearts( "Small_LoveHearts" );
		static const kbString LargeLoveHearts( "Large_LoveHearts" );

		for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
			if ( GetOwner()->GetComponent(i)->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}

			kbParticleComponent *const pParticle = (kbParticleComponent*) GetOwner()->GetComponent(i);

			if ( pParticle->GetName() == SmallLoveHearts.stl_str() ) {
				m_pSmallLoveHearts = pParticle;
				m_pSmallLoveHearts->EnableNewSpawns( false );
			} else if ( pParticle->GetName() == LargeLoveHearts.stl_str() ) {
				m_pLargeLoveHearts = pParticle;
				m_pLargeLoveHearts->EnableNewSpawns( false );
			}
		}

		KungFuSnolafStateBase<KungFuSnolafState::SnolafState_t> * snolafStates[] = {
			new KungFuSnolafStateIdle<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateRun<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateHug<KungFuSnolafState::SnolafState_t>( this ),
			new KungFuSnolafStateDead<KungFuSnolafState::SnolafState_t>( this )
		};

		InitializeStates( snolafStates );
		RequestStateChange( KungFuSnolafState::Idle );
	}
}

/**
*	KungFuSnolafComponent::OnAnimEvent
*/
void KungFuSnolafComponent::OnAnimEvent( const kbAnimEventInfo_t & animEventInfo ) {

	static const kbString LeftFootStep( "Step_LeftFoot" );
	static const kbString RightFootStep( "Step_RightFoot" );
	static const kbString LeftFootBone( "L_Foot" );
	static const kbString RightFootBone( "R_Foot" );

	const kbAnimEvent & animEvent = animEventInfo.m_AnimEvent;
	if ( animEvent.GetEventName() == LeftFootStep || animEvent.GetEventName() == RightFootStep ) {
		if ( m_FootStepImpactFX.GetEntity() != nullptr ) {
			kbGameEntity *const pFootStepFX = g_pGame->CreateEntity( m_FootStepImpactFX.GetEntity() );

			const kbString footBone = ( animEvent.GetEventName() == LeftFootStep ) ? ( LeftFootBone ) : ( RightFootBone );
			kbVec3 decalPosition = kbVec3::zero;
			kbLog( "FOot bone is %s", footBone.c_str() );
			//const kbString boneName
			m_SkelModelsList[0]->GetBoneWorldPosition( footBone, decalPosition  ) ;

			pFootStepFX->SetPosition( decalPosition );
		//	pFootStepFX->SetOrientation( GetOwnerRotation() );
		//	pFootStepFX->DeleteWhenComponentsAreInactive( true );
		}
	}
}

 /**
  *	KungFuSnolafComponent::Update_Internal
  */
void KungFuSnolafComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();
}

/**
 *	KungFuSnolafComponent::EnableSmallLoveHearts
 */
void KungFuSnolafComponent::EnableSmallLoveHearts( const bool bEnable ) {
	if ( m_pSmallLoveHearts == nullptr ) {
		return;
	}

	m_pSmallLoveHearts->EnableNewSpawns( bEnable );
}

/**
 *	KungFuSnolafComponent::EnableLargeLoveHearts
 */
void KungFuSnolafComponent::EnableLargeLoveHearts( const bool bEnable ) {
	if ( m_pLargeLoveHearts == nullptr ) {
		return;
	}

	m_pLargeLoveHearts->EnableNewSpawns( bEnable );
}

/**
 *	KungFuSnolafComponent::TakeDamage
 */
void KungFuSnolafComponent::TakeDamage( const float amount, CannonActorComponent *const pAttacker ) {

	m_Health = -1.0f;
	RequestStateChange( KungFuSnolafState::Dead );
}

/**
 *	KungFuSnolafComponent::DoPoofDeath
 */
void KungFuSnolafComponent::DoPoofDeath() {

	m_SkelModelsList[0]->Enable( false );
	m_SkelModelsList[1]->Enable( false );

	if ( m_PoofDeathFX.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pCannonBallImpact = g_pGame->CreateEntity( m_PoofDeathFX.GetEntity() );
	pCannonBallImpact->SetPosition( GetOwnerPosition() );
	pCannonBallImpact->SetOrientation( GetOwnerRotation() );
	pCannonBallImpact->DeleteWhenComponentsAreInactive( true );

	g_pCannonGame->RemoveGameEntity( this->GetOwner() );
}