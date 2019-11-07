//===================================================================================================
// KungFuLevelComponent.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbLevelDirector.h"
#include "CannonGame.h"
#include "UI/CannonUI.h"
#include "KungFuLevelComponent.h"
#include "CannonPlayer.h"
#include "KungFuSheep.h"
#include "KungFuSnolaf.h"

static const kbVec3 g_SheepStartPos( 77.10445f, -52.6362f, -391.559f ) ;
static const kbQuat g_SheepStartRot( 0.0f, 1.0f, 0.0f, 0.0f );

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

	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override {
		m_bSpawnedSheep = false;
	}

	virtual void UpdateState() override {
		if ( m_bSpawnedSheep == false ) {
			m_bSpawnedSheep = true;
			m_pLevelComponent->SpawnSheep();
		}

		RequestStateChange( KungFuGame::Gameplay );
	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override { }

	bool m_bSpawnedSheep = false;
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

			if ( pTargetComp->IsDead() || pTargetComp->IsEnabled() == false ) {
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
					if ( pTargetSnolaf->IsDead() ) {
						numSnolafsKilled++;

						if ( numSnolafsKilled == 1 ) {
							KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
							pLevelComp->SetPlayLevelMusic( true );
						}
					}
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

		if ( previousState != KungFuGame::Paused ) {
			m_GamePlayStartTime = g_GlobalTimer.TimeElapsedSeconds();
			m_bFirstUpdate = true;
		}

		KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		pLevelComp->SetPlayLevelMusic( false );

		numSnolafsKilled = 0;
	}

	virtual void UpdateState() override {

		if ( m_bFirstUpdate ) {
			m_bFirstUpdate = false;
			g_pCannonGame->GetMainCamera()->SetTarget( g_pCannonGame->GetPlayer()->GetOwner() );
		}

		const kbInput_t & input = g_pInputManager->GetInput();
		if ( input.WasNonCharKeyJustPressed( kbInput_t::Escape ) ) {
			RequestStateChange( KungFuGame::Paused );
			return;
		}

		if ( g_GlobalTimer.TimeElapsedSeconds() < m_GamePlayStartTime + 5.0f ) {
			return;
		}

		int numSnolafs = 0;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent *const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr || pSnolaf->IsEnabled() == false ) {
				continue;
			}

			numSnolafs++;
		}
		//kbLog( "Num SNo = %d", numSnolafs );
		if ( numSnolafsKilled == 0 ) {

			if ( numSnolafs == 0 ) {
				KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
				pLevelComp->SpawnEnemy( true, 1 );
			}
		} else if ( numSnolafs == 0 && g_GlobalTimer.TimeElapsedSeconds() > m_LastSpawnTime + 0.25f ) {

			KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
			pLevelComp->SpawnEnemy( false, 15 );

			m_LastSpawnTime = g_GlobalTimer.TimeElapsedSeconds();
		}

		if ( g_pCannonGame->GetPlayer()->IsDead() ) {
			RequestStateChange( KungFuGame::PlayerDead );
		}

		//kbLog( "Travel dist = %f", g_pCannonGame->GetLevelComponent<KungFuLevelComponent>()->GetDistancePlayerHasTraveled() );
	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override {

	}

	float m_GamePlayStartTime;
	float m_LastSpawnTime = 0.0f;
	bool m_bFirstUpdate = false;
	int numSnolafsKilled = 0;
};

/**
 *	KungFuGame_PausedState
 */
class KungFuGame_PausedState : public KungFuGame_BaseState {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_PausedState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) { 
		m_pPauseMenu = nullptr;
	}

private:
	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override {

		g_pGame->SetDeltaTimeScale( 0.0f );

		m_pPauseMenu = nullptr;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pEnt = g_pCannonGame->GetGameEntities()[i];
			m_pPauseMenu = pEnt->GetComponent<CannonBallPauseMenuUIComponent>();
			if ( m_pPauseMenu != nullptr ) {
				m_pPauseMenu->Enable( true );
				break;
			}
		}

		KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		pLevelComp->SetPlayLevelMusic( false );
	}

	virtual void UpdateState() override {
		const kbInput_t & input = g_pInputManager->GetInput();
		
		if ( input.WasNonCharKeyJustPressed( kbInput_t::Escape ) || m_pPauseMenu->CloseRequested() ) {
			RequestStateChange( KungFuGame::Gameplay );
			return;
		}
	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override {
		g_pGame->SetDeltaTimeScale( 1.0f );

		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pEnt = g_pCannonGame->GetGameEntities()[i];
			CannonBallPauseMenuUIComponent *const pPauseMenu = pEnt->GetComponent<CannonBallPauseMenuUIComponent>();
			if ( pPauseMenu == nullptr ) {
				continue;
			}

			pPauseMenu->Enable( false );
		}

		KungFuLevelComponent *const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		pLevelComp->SetPlayLevelMusic( true );

	}

private: 
	CannonBallPauseMenuUIComponent * m_pPauseMenu;
};

/**
 *	KungFuGame_PlayerDeadState
 */
class KungFuGame_PlayerDeadState : public KungFuGame_BaseState {

//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_PlayerDeadState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) { }

private:

	virtual void BeginState( KungFuGame::eKungFuGame_State previousState ) override {

		g_pCannonGame->GetMainCamera()->SetTarget( nullptr );

		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent *const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr ) {
				continue;
			}

			pSnolaf->RequestStateChange( KungFuSnolafState::RunAway );
		}

		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void UpdateState() override {

		if ( g_GlobalTimer.TimeElapsedSeconds() > m_StateStartTime + 5.0f ) {
			RequestStateChange( KungFuGame::MainMenu );
		}
	}

	virtual void EndState( KungFuGame::eKungFuGame_State nextState ) override {

		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if ( pTargetEnt->GetComponent<KungFuSheepComponent>() ) {
				g_pGame->RemoveGameEntity( pTargetEnt );
				continue;
			}

			if ( pTargetEnt->GetComponent<KungFuSnolafComponent>() ) {
				pTargetEnt->DisableAllComponents();
				g_pGame->GetLevelComponent<KungFuLevelComponent>()->ReturnSnolafToPool( pTargetEnt->GetComponent<KungFuSnolafComponent>() );
			}
		}
	}


	float m_StateStartTime;
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
	m_WaterDropletFXStartTime = -1.0f;
	m_LastWaterSplashSoundTime = 0.0f;
	m_pHealthBarUI = nullptr;
	m_pCannonBallUI = nullptr;

	m_LevelLength = 100.0f;
}

/**
 *	KungFuLevelComponent::SetEnable_Internal
 */
void KungFuLevelComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_pHealthBarUI = nullptr;
	m_pCannonBallUI = nullptr;

	if ( bEnable ) {

		if ( g_UseEditor == false ) {
			g_ResourceManager.GetPackage( "./assets/Packages/fx.kbPkg" );
			g_ResourceManager.GetPackage( "./assets/Packages/Snolaf.kbPkg" );
			g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );
			g_ResourceManager.GetPackage( "./assets/Packages/3000Ton.kbPkg" );
		}

		g_pKungFuDirector = new KungFuSheep_Director();

		KungFuGame_BaseState * pGameStates[KungFuGame::NumStates] = {
			new KungFuGame_MainMenuState( this ),
			new KungFuGame_IntroMenuState( this ),
			new KungFuGame_GameplayState( this ),
			new KungFuGame_PlayerDeadState( this ),
			new KungFuGame_PausedState( this ),
		};

		g_pKungFuDirector->InitializeStates( pGameStates );
		g_pKungFuDirector->RequestStateChange( KungFuGame::MainMenu );

		if ( m_WaterDropletScreenFX.GetEntity() != nullptr ) {

			for ( int i = 0; i < NumWaterSplashes; i++ ) {
				kbErrorCheck( m_WaterSplashFXInst[i].m_Entity.GetEntity() == nullptr, "KungFuLevelComponent::SetEnable_Internal() - Water Droplet Screen FX Instance already allocated" );

				m_WaterSplashFXInst[i].m_Entity.SetEntity( g_pGame->CreateEntity( m_WaterDropletScreenFX.GetEntity() ) );
				kbStaticModelComponent *const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>();
				pSM->Enable( false );
			}
		}

		// SetPlayLevelMusic( true );

		for ( int i = 0; i < 50; i++ ) {
			kbGameEntity *const pSnolaf = g_pGame->CreateEntity( m_SnolafPrefab.GetEntity() );
			for ( int iComp = 0; iComp < pSnolaf->NumComponents(); iComp++ ) {
				pSnolaf->GetComponent(iComp)->Enable( false );
			}
			ReturnSnolafToPool( pSnolaf->GetComponent<KungFuSnolafComponent>() );
		}
	} else {

		for ( int i = 0; i < NumWaterSplashes; i++ ) {
			kbGameEntity *const pEnt = m_WaterSplashFXInst[i].m_Entity.GetEntity();
			if ( pEnt != nullptr ) {
				g_pGame->RemoveGameEntity( pEnt );
				m_WaterSplashFXInst[i].m_Entity.SetEntity( nullptr );
			}	
		}
		while( m_SnolafPool.size() > 0 ) {
			kbGameEntity *const pSnolaf = m_SnolafPool.back();
			m_SnolafPool.pop_back();
			g_pGame->RemoveGameEntity( pSnolaf );
		}
		m_SnolafPool.clear();
		delete g_pKungFuDirector;
		g_pKungFuDirector = nullptr;
	}
}

/**
 *	KungFuLevelComponent::Update_Internal
 */
const kbVec4 g_WaterDropletNormalFactorScroll[] = {
		kbVec4( 0.1000f, 0.1000f, 0.00000f, 0.01f ),
		kbVec4( 0.1000f, 0.1000f, 0.00000f, 0.007f ) };

const float g_WaterDropStartDelay[] = { 0.1f, 0.01f };

void KungFuLevelComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( g_UseEditor ) {
		return;
	}

	g_pKungFuDirector->UpdateStateMachine();

	// Not all game entities are loaded in SetEnable_Internal unfortunately
	if ( m_pHealthBarUI == nullptr || m_pCannonBallUI == nullptr ) {
		m_pHealthBarUI = nullptr;
		m_pCannonBallUI = nullptr;
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size() && ( m_pHealthBarUI == nullptr || m_pCannonBallUI == nullptr ); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if ( m_pHealthBarUI == nullptr ) {
				m_pHealthBarUI = pTargetEnt->GetComponent<CannonHealthBarUIComponent>();
			}

			if ( m_pCannonBallUI == nullptr ) {
				m_pCannonBallUI = pTargetEnt->GetComponent<CannonBallUIComponent>();
			}
		}
	}

	if ( m_WaterDropletFXStartTime > 0.0f ) {

		int numFinished = 0;

		for ( int i = 0; i < NumWaterSplashes; i++ ) {

			const float curTime = g_GlobalTimer.TimeElapsedSeconds();
			const float fxStartTime = m_WaterDropletFXStartTime + m_WaterSplashFXInst[i].m_InitialDelay;

			if ( curTime < fxStartTime ) {
				continue;
			}

			const float fxDuration = m_WaterSplashFXInst[i].m_Duration;
			float normalizedTime = ( g_GlobalTimer.TimeElapsedSeconds() - fxStartTime ) / fxDuration;
			kbStaticModelComponent *const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>(); 

			if ( normalizedTime > 1.0f ) {
				pSM->Enable( false );
				numFinished++;
			} else {
				pSM->Enable(  true );
				const float delayScrollTime = g_WaterDropStartDelay[i];
				if ( normalizedTime > delayScrollTime ) {
					normalizedTime = kbClamp( ( normalizedTime - delayScrollTime ) * ( 1.0f / delayScrollTime ), 0.0f, 999.0f );
					static kbString normalFactor_scrollRate( "normalFactor_scrollRate" );
					kbVec4 scroll = g_WaterDropletNormalFactorScroll[i];
					scroll.w *= -normalizedTime;

					pSM->SetMaterialParamVector( 0, normalFactor_scrollRate.stl_str(), scroll );

					// Blend out time
					{
						const float blendOutStart = fxStartTime + ( fxDuration * 0.75f );
						const float blendOutTime = kbClamp( ( g_GlobalTimer.TimeElapsedSeconds() - blendOutStart ) / ( fxDuration * 0.25f ), 0.0f, 1.0f );
						static kbString colorFactor( "colorFactor" );
						pSM->SetMaterialParamVector( 0, colorFactor.stl_str(), kbVec4( 1.0f, 1.0f, 1.0f, 1.0f - blendOutTime ) );
					}
				}
			}
		}

		if ( numFinished == NumWaterSplashes ) {
			m_WaterDropletFXStartTime = -1.0f;
		}
	}
}

/**
 *	KungFuLevelComponent::DoAttack
 */
AttackHitInfo_t KungFuLevelComponent::DoAttack( const DealAttackInfo_t<KungFuGame::eAttackType> & attackInfo ) {
	return g_pKungFuDirector->DoAttack( attackInfo );
}

/**
 *	KungFuLevelComponent::UpdateSheepHealthBar
 */
void KungFuLevelComponent::UpdateSheepHealthBar( const float healthVal ) {

	if ( m_pHealthBarUI == nullptr ) {
		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size() && ( m_pHealthBarUI == nullptr ); i++ ) {
			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if ( m_pHealthBarUI == nullptr ) {
				m_pHealthBarUI = pTargetEnt->GetComponent<CannonHealthBarUIComponent>();
			}
		}
	}

	m_pHealthBarUI->SetTargetHealth( healthVal );
}

/**
 *	KungFuLevelComponent::UpdateCannonBallMeter
 */
void KungFuLevelComponent::UpdateCannonBallMeter( const float fillVal, const bool bActivated ) {

	if ( m_pCannonBallUI == nullptr ) {
		return;
	}

	if ( bActivated ) {
		m_pCannonBallUI->CannonBallActivatedCB();
	} else {
		m_pCannonBallUI->SetFill( fillVal );
	}
}


/**
 *	KungFuLevelComponent::SpawnSheep
 */
void KungFuLevelComponent::SpawnSheep() {
	kbGameEntity *const sheep = g_pGame->CreateEntity( m_SheepPrefab.GetEntity() );

	sheep->SetPosition( g_SheepStartPos );
	sheep->SetOrientation( g_SheepStartRot );
}

/**
 *	KungFuLevelComponent::SpawnEnemy
 */
void KungFuLevelComponent::SpawnEnemy( const bool bSpawnLeft, const int waveSize ) {

	if ( m_SnolafPrefab.GetEntity() == nullptr ) {
		return;
	}

	const float startSpawnDist = 9.5f;
	const float spawnOffsets = 1.0f;
	kbVec3 nextNegZSpawnPos = g_pCannonGame->GetPlayer()->GetOwnerPosition() + kbVec3( 0.0f, 0.0f, -startSpawnDist );
	kbVec3 nextPosZSpawnPos = g_pCannonGame->GetPlayer()->GetOwnerPosition() + kbVec3( 0.0f, 0.0f, startSpawnDist );

	for ( int i = 0; i < waveSize; i++ ) {
		kbVec3 spawnPos = kbVec3::zero;
		if ( bSpawnLeft == false && rand() % 2 == 0 ) {
			spawnPos = nextNegZSpawnPos;
			nextNegZSpawnPos.z -= spawnOffsets;
		} else {
			spawnPos = nextPosZSpawnPos;
			nextPosZSpawnPos.z += spawnOffsets;
		}

		//kbLog( "Snolaf popped of list of size %d", m_SnolafPool.size() );

		kbGameEntity *const pSnolaf = m_SnolafPool.back();
		m_SnolafPool.pop_back();//g_pGame->CreateEntity( m_SnolafPrefab.GetEntity() );
		pSnolaf->SetPosition( spawnPos );
		pSnolaf->SetOrientation( GetOwnerRotation() );

		KungFuSnolafComponent *const pSnolafComp = pSnolaf->GetComponent<KungFuSnolafComponent>();
		pSnolafComp->RequestStateChange( KungFuSnolafState::Idle );
		pSnolafComp->ResetFromPool();
	}
}

/**
 *	KungFuLevelComponent::DoWaterDropletScreenFX
 */
void KungFuLevelComponent::DoWaterDropletScreenFX() {

	if ( m_WaterDropletScreenFX.GetEntity() == nullptr || m_WaterDropletFXStartTime > 0.0f ) {
		return;
	}

	m_WaterDropletFXStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_WaterSplashFXInst[0].m_Duration = 1.5f;
	m_WaterSplashFXInst[0].m_InitialDelay = 1.0f;

	m_WaterSplashFXInst[1].m_Duration = 2.0f;
	m_WaterSplashFXInst[1].m_InitialDelay = 0.75f;

	//m_WaterDropletScreenFXInst.SetEntity( g_pGame->CreateEntity( m_WaterDropletScreenFX.GetEntity() ) );

	for ( int i = 0; i < NumWaterSplashes; i++ ) {
		kbStaticModelComponent *const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>();
	//	pSM->Enable( true );


		static kbString startUVOffsetParam( "startUVOffset" );
		pSM->SetMaterialParamVector( 0, startUVOffsetParam.stl_str(), kbVec4( kbfrand(), kbfrand(), 0.0f, 0.0f ) );

		static kbString normalFactor_scrollRate( "normalFactor_scrollRate" );
		pSM->SetMaterialParamVector( 0, normalFactor_scrollRate.stl_str(), kbVec4( g_WaterDropletNormalFactorScroll[i].x, g_WaterDropletNormalFactorScroll[i].y, 0.0f, 0.0f ) );

		static kbString colorFactor( "colorFactor" );
		pSM->SetMaterialParamVector( 0, colorFactor.stl_str(), kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	}
}

/**
 *	KungFuLevelComponent::DoSplashSound
 */
void KungFuLevelComponent::DoSplashSound() {

	if ( m_WaterSplashSound.size() == 0 ) {
		return;
	}

	if ( g_GlobalTimer.TimeElapsedSeconds() < m_LastWaterSplashSoundTime + 2.0f ) {
		return;
	}

	m_LastWaterSplashSoundTime = g_GlobalTimer.TimeElapsedSeconds();
	m_WaterSplashSound[rand() % m_WaterSplashSound.size()].PlaySoundAtPosition( kbVec3( 0.0f, 0.0f, 0.0f ) );
}

/**
 *	KungFuLevelComponent::SetPlayLevelMusic
 */
void KungFuLevelComponent::SetPlayLevelMusic( const bool bPlay ) {

	if ( m_LevelMusic.size() == 0 ) {
		return;
	}

	if ( bPlay ) {
		m_LevelMusic[0].PlaySoundAtPosition( kbVec3::zero );
	} else {
		m_LevelMusic[0].StopSound();
	}
}

/**
 *	KungFuLevelComponent::GetDistancePlayerHasTraveled
 */
float KungFuLevelComponent::GetDistancePlayerHasTraveled() {

	return ( g_pCannonGame->GetPlayer()->GetOwnerPosition() - g_SheepStartPos ).Length();
}

/**
 *	KungFuLevelComponent::ReturnSnolafToPool
 */
void KungFuLevelComponent::ReturnSnolafToPool( KungFuSnolafComponent *const pSnolafComp ) {

	kbGameEntity *const pSnolaf = pSnolafComp->GetOwner();
	pSnolaf->DisableAllComponents();

	m_SnolafPool.push_back( pSnolaf );

//	kbLog( "Snolaf returned.  Pool size = %d", m_SnolafPool.size() );
}
