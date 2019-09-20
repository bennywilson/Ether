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
#include "KungFuSheep.h"
#include "KungFuSnolaf.h"


/**
 *	KungFuGame_BaseState
 */
class KungFuGame_BaseState : public StateMachineNode<KungFuGame::eKungFuGame_State> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_BaseState( KungFuLevelComponent *const pLevelComponent ) : m_pLevelComponent( pLevelComponent ) { }

	virtual AttackHitInfo_t DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo ) { AttackHitInfo_t ret; return ret; }

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

	virtual AttackHitInfo_t DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & dealAttackInfo ) override { 

		kbErrorCheck( dealAttackInfo.m_pAttacker != nullptr, "KungFuGame_GameplayState::DoAttack() - null attacker" );
		AttackHitInfo_t retVal;

		const auto pAttackerComp = dealAttackInfo.m_pAttacker;
		const kbVec3 attackerPos = pAttackerComp->GetOwnerPosition();
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent *const pTargetComp = pTargetEnt->GetComponent<CannonActorComponent>();
			if ( pTargetComp == nullptr || pTargetComp == pAttackerComp ) {
				continue;
			}

			if ( pTargetComp->IsDead() ) {
				continue;
			}

			const kbVec3 targetPos = pTargetComp->GetOwnerPosition();
			if ( dealAttackInfo.m_Radius == 0.0f ) {

				// Slap a %@3$&!!
				if ( ( attackerPos - targetPos ).Length() > 1.0f ) {
					continue;
				}

				const kbVec3 vecToTarget = ( targetPos - attackerPos ).Normalized();
				const kbVec3 attackerFacingDir = pAttackerComp->GetOwnerRotation().ToMat4()[2].ToVec3();
				if ( vecToTarget.Dot( attackerFacingDir ) > 0.0f ) {
					continue;
				}
			} else {
				// Slap that %@3$&!!
				if ( ( attackerPos - targetPos ).Length() > dealAttackInfo.m_Radius ) {
					continue;
				}
			}
			KungFuSheepComponent *const pAttackerSheep = pAttackerComp->GetAs<KungFuSheepComponent>();
			if ( pAttackerSheep != nullptr ) {
				KungFuSnolafComponent*const pTargetSnolaf = pTargetComp->GetAs<KungFuSnolafComponent>();
				if ( pTargetSnolaf != nullptr ) {
					pTargetSnolaf->TakeDamage( dealAttackInfo );
				}
			} else {
				KungFuSnolafComponent*const pAttackerSnolaf = pAttackerComp->GetAs<KungFuSnolafComponent>();
				if ( pAttackerSnolaf != nullptr ) {
					KungFuSheepComponent *const pTargetSheep = pTargetComp->GetAs<KungFuSheepComponent>();
					if ( pTargetSheep != nullptr ) {
						pTargetSheep->TakeDamage( dealAttackInfo );
					}
				}
			}
			retVal.m_bHit = true;

			if ( dealAttackInfo.m_Radius == 0.0f ) {
				break;
			}
		}
		return retVal;
	}

private:
	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override {
		m_GamePlayStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void UpdateState() override {

		if ( g_GlobalTimer.TimeElapsedSeconds() < m_GamePlayStartTime + 5.0f ) {
			return;
		}

		int numSnolafs = 0;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent *const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr ) {
				continue;
			}
			numSnolafs++;
		}

		if ( numSnolafs < 0 && g_GlobalTimer.TimeElapsedSeconds() > m_LastSpawnTime + 0.25f ) {

			KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
			pLevelComp->SpawnEnemy();

			m_LastSpawnTime = g_GlobalTimer.TimeElapsedSeconds();
		}
	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override {

	}

	float m_GamePlayStartTime;
	float m_LastSpawnTime = 0.0f;
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

	AttackHitInfo_t DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> attackInfo ) {

		AttackHitInfo_t attackHitInfo;
		if ( this->m_CurrentState < 0 || m_CurrentState >= KungFuGame::NumStates ) {
			return attackHitInfo;
		}

		return m_States[m_CurrentState]->DoAttack( attackInfo );
	}
};

static KungFuSheep_Director * g_pKungFuDirector = nullptr;

/**
 *	KungFuLevelComponent::Constructor
 */
void KungFuLevelComponent::Constructor() {
}

/**
 *	KungFuLevelComponent::SetEnable_Internal
 */
void KungFuLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		g_ResourceManager.GetPackage( "./assets/Packages/Snolaf.kbPkg" );
		g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );
		g_ResourceManager.GetPackage( "./assets/Packages/3000Ton.kbPkg" );

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

	if ( g_UseEditor ) {
		return;
	}

	g_pKungFuDirector->UpdateStateMachine();
}

/**
 *	KungFuLevelComponent::Update_Internal
 */
AttackHitInfo_t KungFuLevelComponent::DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & attackInfo ) {
	return g_pKungFuDirector->DoAttack( attackInfo );
}

/**
 *	KungFuLevelComponent::SpawnEnemy
 */
void KungFuLevelComponent::SpawnEnemy() {

	if ( m_SnolafPrefab.GetEntity() == nullptr ) {
		return;
	}

	kbGameEntity *const pSnolaf = g_pGame->CreateEntity( m_SnolafPrefab.GetEntity() );
	kbVec3 spawnOffset( 0.0f, 0.0f, 12.0f );
	if ( rand() % 2 == 0 ) {
		spawnOffset.z *=  -1.0f;
	}
	pSnolaf->SetPosition( g_pCannonGame->GetPlayer()->GetOwnerPosition() + spawnOffset );
	pSnolaf->SetOrientation( GetOwnerRotation() );
}
