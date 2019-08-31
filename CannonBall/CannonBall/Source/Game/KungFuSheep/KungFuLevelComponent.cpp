//===================================================================================================
// KungFuLevelComponent.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbLevelDirector.h"
#include "CannonGame.h"
#include "KungFuLevelComponent.h"
#include "CannonPlayer.h"

namespace KungFuGame {

	/**
	 *	eKungFuGame_State
	 */
	enum eKungFuGame_State {
		MainMenu = 0,
		Intro,
		Gameplay,
		NumStates
	 };
};

/**
 *	KungFuGame_BaseState
 */
class KungFuGame_BaseState : public StateMachineNode<KungFuGame::eKungFuGame_State> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_BaseState( KungFuLevelComponent *const pLevelComponent ) : m_pLevelComponent( pLevelComponent ) { }

	virtual KungFuLevelComponent::AttackInfo_t PerformAttack( CannonActorComponent *const pAttacker ) { KungFuLevelComponent::AttackInfo_t ret; return ret; }

protected:
	KungFuLevelComponent * m_pLevelComponent;

};


/**
 *	KungFuGame_MainMenuState
 */
class KungFuGame_MainMenuState : public KungFuGame_BaseState {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_MainMenuState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) { }

private:

	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override { }

	virtual void UpdateState() override { }

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override { }
};

/**
 *	KungFuGame_IntroMenuState
 */
class KungFuGame_IntroMenuState : public KungFuGame_BaseState {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_IntroMenuState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) { }

private:
	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override { }

	virtual void UpdateState() override { }

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override { }

};

/**
 *	KungFuGame_GameplayState
 */
class KungFuGame_GameplayState : public KungFuGame_BaseState {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_GameplayState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) { }

	virtual KungFuLevelComponent::AttackInfo_t PerformAttack( CannonActorComponent *const pAttackerComp ) override { 

		kbErrorCheck( pAttackerComp != nullptr, "KungFuGame_GameplayState::PerformAttack() - null attacker" );
		KungFuLevelComponent::AttackInfo_t retVal;

		const kbVec3 attackerPos = pAttackerComp->GetOwnerPosition();
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent *const pTargetComp = pTargetEnt->GetComponent<CannonActorComponent>();
			if ( pTargetComp == nullptr || pTargetComp == pAttackerComp ) {
				continue;
			}

			const kbVec3 targetPos = pTargetComp->GetOwnerPosition();
			if ( ( attackerPos - targetPos ).Length() > 1.0f ) {
				continue;
			}

			const kbVec3 vecToTarget = ( targetPos - attackerPos ).Normalized();
			const kbVec3 attackerFacingDir = pAttackerComp->GetOwnerRotation().ToMat4()[2].ToVec3();
			if ( vecToTarget.Dot( attackerFacingDir ) > 0.0f ) {
				continue;
			}

			retVal.m_bHit = true;
		}
		return retVal;
	}

private:
	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override {

	}

	virtual void UpdateState() override {

	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override {

	}
};

/**
 *	KungFuSheep_Director
 */
class KungFuSheep_Director : public kbLevelDirector<KungFuGame_BaseState, KungFuGame::eKungFuGame_State>  {

//---------------------------------------------------------------------------------------------------
public:
	virtual	~KungFuSheep_Director() {

	}

	virtual void UpdateStateMachine() override {
		kbLevelDirector::UpdateStateMachine();
	}

	KungFuLevelComponent::AttackInfo_t PerformAttack( CannonActorComponent *const pAttacker ) {

		KungFuLevelComponent::AttackInfo_t attackInfo;
		if ( this->m_CurrentState < 0 || m_CurrentState >= KungFuGame::NumStates ) {
			return attackInfo;
		}

		return m_States[m_CurrentState]->PerformAttack( pAttacker );
	}
};

static KungFuSheep_Director * g_pKungFuDirector = nullptr;

/**
 *	KungFuLevelComponent::Constructor
 */
void KungFuLevelComponent::Constructor() {
	m_Dummy = -1;
}

/**
 *	KungFuLevelComponent::SetEnable_Internal
 */
void KungFuLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_pKungFuDirector = new KungFuSheep_Director();

		KungFuGame_BaseState * pGameStates[] = {
			new KungFuGame_MainMenuState( this ),
			new KungFuGame_IntroMenuState( this ),
			new KungFuGame_GameplayState( this ),
		};

		g_pKungFuDirector->InitializeStates( pGameStates );
		g_pKungFuDirector->RequestStateChange( KungFuGame::Gameplay );

	} else {
		delete g_pKungFuDirector;
	}
}

/**
 *	KungFuLevelComponent::Update_Internal
 */
void KungFuLevelComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	g_pKungFuDirector->UpdateStateMachine();
}

/**
 *	KungFuLevelComponent::Update_Internal
 */
KungFuLevelComponent::AttackInfo_t KungFuLevelComponent::PerformAttack( CannonActorComponent *const pAttacker ) {
	return g_pKungFuDirector->PerformAttack( pAttacker );
}
