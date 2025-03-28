//===================================================================================================
// KungFuLevelComponent.cpp
//
// 2019-2020 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "CannonGame.h"
#include "UI/CannonUI.h"
#include "KungFuLevelComponent.h"
#include "CannonPlayer.h"
#include "KungFuSheep.h"
#include "KungFuSnolaf.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

kbConsoleVariable g_CullGrass("cullgrass", false, kbConsoleVariable::Console_Bool, "", "");


namespace KungFuGame {

	enum eSkipCheats {
		Skip_None,
		Skip_MainMenuAndIntro,
		Skip_ToEnd,
		Skip_NoEnemies,
	};

};

KungFuGame::eSkipCheats g_SkipCheat = KungFuGame::Skip_None;

#include "Cinema\Outro.inl"

/// KungFuGame_MainMenuState
class KungFuGame_MainMenuState : public KungFuGame_BaseState {

	//---------------------------------------------------------------------------------------------------
public:

	KungFuGame_MainMenuState(KungFuLevelComponent* const pLevelComponent) : KungFuGame_BaseState(pLevelComponent) { }

private:

	virtual void WidgetEventCB(kbUIWidgetComponent* const pWidget, const kbInput_t* pInput) override {

		if (pInput == nullptr || WasConfirmationButtonPressed(pInput) == false) {
			return;
		}

		const auto pMainMenu = pWidget->GetAs<CannonBallMainMenuComponent>();
		if (pMainMenu != nullptr) {
			if (pMainMenu->GetSelectedIndex() == 0) {
				RequestStateChange(KungFuGame::Intro);
			} else if (pMainMenu->GetSelectedIndex() == 1) {
				RequestStateChange(KungFuGame::Paused);
			} else {
				g_pCannonGame->RequestQuitGame();
			}
		}
	}

	virtual void BeginState_Internal(KungFuGame::eKungFuGame_State previousState) override {

		static const kbString JumpingJacks_Anim("JumpingJacks");

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		if (pSheep == nullptr) {
			pSheep = m_pLevelComponent->SpawnSheep();
		}
		pSheep->ExternalRequestStateChange(KungFuSheepState::Cinema);
		pSheep->PlayAnimation(JumpingJacks_Anim, 0.15f);
		pSheep->SetTargetFacingDirection(Vec3(-1.0f, 0.0f, -1.0f).normalize_safe());
		pSheep->SetOwnerPosition(KungFuGame::kSheepStartPos);

		KungFuLevelComponent::Get()->ShowCredits(false);
	}

	virtual void UpdateState_Internal() override {

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();

		if (g_pCannonGame->GetMainCamera() != nullptr) {
			g_pCannonGame->GetMainCamera()->SetTarget(nullptr, -1.0f);
			g_pCannonGame->GetMainCamera()->SetOwnerPosition(KungFuGame::kCameraStartPos);
			g_pCannonGame->GetMainCamera()->SetOwnerRotation(KungFuGame::kCameraRotation);
		}

		if (g_SkipCheat == KungFuGame::Skip_MainMenuAndIntro || g_SkipCheat == KungFuGame::Skip_ToEnd || g_SkipCheat == KungFuGame::Skip_NoEnemies) {

			static const kbString IdleL_Anim("IdleLeft_Basic");
			pSheep->PlayAnimation(IdleL_Anim, 0.2f);
			RequestStateChange(KungFuGame::Intro);

			if (g_SkipCheat == KungFuGame::Skip_ToEnd) {
				pSheep->SetOwnerPosition(Vec3(77.10445f, -52.6362f, KungFuGame::kOutroStartZ));
			}
			return;
		}
	}

	KungFuSheepComponent* m_pSheep = nullptr;
};

/// KungFuGame_IntroGameState
class KungFuGame_IntroGameState : public KungFuGame_BaseState {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_IntroGameState(KungFuLevelComponent* const pLevelComponent) : KungFuGame_BaseState(pLevelComponent) { }

private:
	virtual void BeginState_Internal(KungFuGame::eKungFuGame_State previousState) override {

		m_CurrentState = 0;
	}

	virtual void UpdateState_Internal() override {

		if (g_SkipCheat == KungFuGame::Skip_MainMenuAndIntro || g_SkipCheat == KungFuGame::Skip_ToEnd || g_SkipCheat == KungFuGame::Skip_NoEnemies) {
			RequestStateChange(KungFuGame::Gameplay);
			return;
		}

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();

		if (m_CurrentState == 0) {

			if (GetTimeSinceStateBegan() > 1.75f) {
				KungFuLevelComponent::Get()->SetPlayLevelMusic(0, true);
				m_CurrentState = 1;
			}

		} else if (m_CurrentState == 1) {

			static const kbString Run_Anim("Run_Basic");

			pSheep->PlayAnimation(Run_Anim, 0.15f);
			pSheep->SetTargetFacingDirection(Vec3(0.0f, 0.0f, -1.0f));
			m_CurrentState = 2;

		} else if (m_CurrentState == 2) {

			if (pSheep->owner_position().z < -391.559f) {
				const float WalkSpeedMultiplier = 0.65f;	// Sheep takes his time getting to the start point
				const float StartZ = -391.559f;

				auto frameDt = g_pGame->GetFrameDT();
				Vec3 targetPos = pSheep->owner_position() + Vec3(0.0f, 0.0f, 1.0f) * frameDt * pSheep->GetMaxRunSpeed() * WalkSpeedMultiplier;
				if (targetPos.z >= StartZ) {
					targetPos.z = StartZ;
					m_CurrentState = 3;

					static const kbString IdleL_Anim("Black Power");
					pSheep->PlayAnimation(IdleL_Anim, 0.2f);
					KungFuLevelComponent::Get()->ShowBLM(true);
				}
				pSheep->SetOwnerPosition(targetPos);
			}
		} else if (m_CurrentState == 3) {

			if (GetTimeSinceStateBegan() > 7.8f) {
				pSheep->PlayBaa(0);
				m_CurrentState = 4;
				KungFuLevelComponent::Get()->ShowBLM(false);

			}
		} else if (m_CurrentState == 4) {
			if (GetTimeSinceStateBegan() > 8.0f) {
				RequestStateChange(KungFuGame::Gameplay);
			}
		}
	}

	virtual void EndState_Internal(KungFuGame::eKungFuGame_State nextState) override {
		KungFuLevelComponent::Get()->GetSheep()->ExternalRequestStateChange(KungFuSheepState::Idle);
	}

	int m_CurrentState = 0;
};

/// KungFuGame_GameplayState
class KungFuGame_GameplayState : public KungFuGame_BaseState {

	//---------------------------------------------------------------------------------------------------
public:

	KungFuGame_GameplayState(KungFuLevelComponent* const pLevelComponent) : KungFuGame_BaseState(pLevelComponent) { }

	virtual AttackHitInfo_t DoAttack(const DealAttackInfo_t<KungFuGame::eAttackType>& dealAttackInfo) override {
		blk::error_check(dealAttackInfo.m_pAttacker != nullptr, "KungFuGame_GameplayState::DoAttack() - null attacker");
		AttackHitInfo_t retVal;

		const auto pAttackerComp = dealAttackInfo.m_pAttacker;
		const Vec3 attackerPos = pAttackerComp->owner_position();

		// TODO - Optimize
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent* const pTargetComp = pTargetEnt->GetComponent<CannonActorComponent>();
			if (pTargetComp == nullptr || pTargetComp == pAttackerComp) {
				continue;
			}

			if (pTargetComp->IsDead() || pTargetComp->IsEnabled() == false) {
				continue;
			}

			const Vec3 targetPos = pTargetComp->owner_position();
			if (dealAttackInfo.m_Radius == 0.0f) {

				// Slap a %@3$&!!
				if ((attackerPos - targetPos).length() > KungFuGame::kSheepAttackDist) {
					continue;
				}

				const Vec3 vecToTarget = (targetPos - attackerPos).normalize_safe();
				const Vec3 attackerFacingDir = pAttackerComp->owner_rotation().to_mat4()[2].ToVec3();
				if (vecToTarget.dot(attackerFacingDir) > 0.0f) {
					continue;
				}
			} else {
				// Slap that %@3$&!!
				if ((attackerPos - targetPos).length() > dealAttackInfo.m_Radius) {
					continue;
				}
			}
			KungFuSheepComponent* const pAttackerSheep = pAttackerComp->GetAs<KungFuSheepComponent>();
			if (pAttackerSheep != nullptr) {
				KungFuSnolafComponent* const pTargetSnolaf = pTargetComp->GetAs<KungFuSnolafComponent>();
				if (pTargetSnolaf != nullptr) {
					pTargetSnolaf->TakeDamage(dealAttackInfo);
					if (pTargetSnolaf->IsDead()) {
						m_NumSnolafsKilled++;

						if (m_NumSnolafsKilled == 1) {
							KungFuLevelComponent::Get()->SetPlayLevelMusic(1, true);
						}
					}
				}
			} else {
				KungFuSnolafComponent* const pAttackerSnolaf = pAttackerComp->GetAs<KungFuSnolafComponent>();
				if (pAttackerSnolaf != nullptr) {
					KungFuSheepComponent* const pTargetSheep = pTargetComp->GetAs<KungFuSheepComponent>();
					if (pTargetSheep != nullptr) {
						pTargetSheep->TakeDamage(dealAttackInfo);
					}
				}
			}
			retVal.m_bHit = true;

			if (dealAttackInfo.m_Radius == 0.0f) {
				break;
			}
		}
		return retVal;
	}

private:

	virtual void BeginState_Internal(KungFuGame::eKungFuGame_State previousState) override {

		if (previousState == KungFuGame::Paused) {
			KungFuLevelComponent::Get()->SetPlayLevelMusic(1, false);
		} else if (previousState == KungFuGame::Intro) {
			m_NumSnolafsKilled = 0;
			m_GamePlayStartTime = g_GlobalTimer.TimeElapsedSeconds();
			m_bFirstUpdate = true;
		}
	}

	virtual void UpdateState_Internal() override {
		const auto pKungFuSheep = KungFuLevelComponent::Get()->GetSheep();
		//g_pRenderer->DrawDebugText(gpuTimings, 0.55f, curY, g_DebugTextSize, g_DebugTextSize, kbColor::green);

		std::string distanceTraveled = std::to_string(KungFuLevelComponent::Get()->GetPlayerTravelDistance());
		distanceTraveled += "   ";
		distanceTraveled += std::to_string(pKungFuSheep->owner_position().x);
		distanceTraveled += ", ";
		distanceTraveled += std::to_string(pKungFuSheep->owner_position().y);
		distanceTraveled += ", ";
		distanceTraveled += std::to_string(pKungFuSheep->owner_position().z);
		// 
		// 		g_pRenderer->DrawDebugText( distanceTraveled, 0.1f, 0.1f, g_DebugTextSize, g_DebugTextSize, kbColor::red );

		if (m_bFirstUpdate) {
			m_bFirstUpdate = false;
			g_pCannonGame->GetMainCamera()->SetTarget(g_pCannonGame->GetPlayer()->GetOwner(), -1.0f);
		}


		if (pKungFuSheep->owner_position().z > KungFuGame::kOutroStartZ) {
			RequestStateChange(KungFuGame::Outro);
			return;
		}

		const kbInput_t& input = g_pInputManager->get_input();
		if (WasStartButtonPressed() || input.WasNonCharKeyJustPressed(kbInput_t::Escape)) {
			RequestStateChange(KungFuGame::Paused);
			return;
		}

		if (g_GlobalTimer.TimeElapsedSeconds() < m_GamePlayStartTime + 1.25f) {
			return;
		}

		int numSnolafs = 0;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent* const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if (pSnolaf == nullptr || pSnolaf->IsEnabled() == false) {
				continue;
			}

			numSnolafs++;
		}

		if (g_SkipCheat != KungFuGame::Skip_ToEnd && g_SkipCheat != KungFuGame::Skip_NoEnemies) {
			auto pLevelComponent = KungFuLevelComponent::Get();
			if (m_NumSnolafsKilled == 0) {

				if (numSnolafs == 0) {
					pLevelComponent->SpawnEnemy(true, 1);
				}
			} else if (g_GlobalTimer.TimeElapsedSeconds() > m_LastSpawnTime + KungFuGame::kTimeBetweenSnolafWaves) {

				const float distTraveled = pLevelComponent->GetPlayerTravelDistance();
				const float normalizedDistTraveled = kbSaturate(distTraveled / KungFuGame::kLevelLength);

				// TODO - Polish
				const int numToSpawn = ((int)(normalizedDistTraveled * KungFuGame::kMaxSnolafWaveSize) + 1) & 0xfffffffe;
				pLevelComponent->SpawnEnemy(false, numToSpawn);

				m_LastSpawnTime = g_GlobalTimer.TimeElapsedSeconds();
			}
		}

		if (g_pCannonGame->GetPlayer()->IsDead()) {
			RequestStateChange(KungFuGame::PlayerDead);
		}
	}

	float m_GamePlayStartTime;
	float m_LastSpawnTime = 0.0f;
	bool m_bFirstUpdate = false;
	int m_NumSnolafsKilled = 0;
};

/// KungFuGame_PausedState
class KungFuGame_PausedState : public KungFuGame_BaseState {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_PausedState(KungFuLevelComponent* const pLevelComponent) : KungFuGame_BaseState(pLevelComponent) {
		m_pPauseMenu = nullptr;
	}

private:

	virtual void BeginState_Internal(KungFuGame::eKungFuGame_State previousState) override {

		if (previousState != KungFuGame::MainMenu) {
			g_pGame->SetDeltaTimeScale(0.0f);
		}

		m_pPauseMenu = nullptr;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pEnt = g_pCannonGame->GetGameEntities()[i];
			m_pPauseMenu = pEnt->GetComponent<CannonBallPauseMenuUIComponent>();
			if (m_pPauseMenu != nullptr) {
				m_pPauseMenu->Enable(true);
				break;
			}
		}

		KungFuLevelComponent* const pLevelComp = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
		pLevelComp->SetPlayLevelMusic(1, false);
	}

	virtual void EndState_Internal(KungFuGame::eKungFuGame_State nextState) override {
		g_pGame->SetDeltaTimeScale(1.0f);

		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pEnt = g_pCannonGame->GetGameEntities()[i];
			CannonBallPauseMenuUIComponent* const pPauseMenu = pEnt->GetComponent<CannonBallPauseMenuUIComponent>();
			if (pPauseMenu == nullptr) {
				continue;
			}

			pPauseMenu->Enable(false);
		}

		KungFuLevelComponent::Get()->SetPlayLevelMusic(1, true);

		if (nextState == KungFuGame::MainMenu && KungFuSheepDirector::Get()->GetPreviousState() == KungFuGame::Gameplay) {
			KungFuLevelComponent::Get()->RemoveSheep();
		}
	}

	virtual void WidgetEventCB(kbUIWidgetComponent* const pWidget, const kbInput_t* pInput) override {

		const auto pPauseMenu = KungFuSheepDirector::Get()->GetPauseMenu();
		blk::error_check(pPauseMenu != nullptr, "KungFuGame_PausedState::WidgetEventCB() - null pause menu component");

		if (WasConfirmationButtonPressed(pInput)) {
			if (pPauseMenu->GetSelectedWidgetIdx() == 0) {
				if (KungFuSheepDirector::Get()->GetPreviousState() == KungFuGame::MainMenu) {
					RequestStateChange(KungFuGame::MainMenu);
				} else {
					RequestStateChange(KungFuGame::Gameplay);
				}
			} else if (pPauseMenu->GetSelectedWidgetIdx() == 4) {
				RequestStateChange(KungFuGame::MainMenu);
			}
			return;
		}
	}

private:
	CannonBallPauseMenuUIComponent* m_pPauseMenu;
};

/// KungFuGame_PlayerDeadState
class KungFuGame_PlayerDeadState : public KungFuGame_BaseState {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuGame_PlayerDeadState(KungFuLevelComponent* const pLevelComponent) : KungFuGame_BaseState(pLevelComponent) { }

private:

	virtual void BeginState_Internal(KungFuGame::eKungFuGame_State previousState) override {

		g_pCannonGame->GetMainCamera()->SetTarget(nullptr, -1.0f);

		// TODO - Optimize
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent* const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if (pSnolaf == nullptr || pSnolaf->IsEnabled() == false || pSnolaf->IsDead()) {
				continue;
			}

			pSnolaf->RequestStateChange(KungFuSnolafState::RunAway);
		}

		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void UpdateState_Internal() override {

		if (g_GlobalTimer.TimeElapsedSeconds() > m_StateStartTime + 5.0f) {
			RequestStateChange(KungFuGame::MainMenu);
		}
	}

	virtual void EndState_Internal(KungFuGame::eKungFuGame_State nextState) override {

		KungFuLevelComponent::Get()->RemoveSheep();
	}

	float m_StateStartTime;
};


KungFuLevelComponent* KungFuLevelComponent::s_Inst = nullptr;

/// KungFuLevelComponent::Constructor
void KungFuLevelComponent::Constructor() {

	// Editor
	m_LevelLength = 100.0f;

	m_pBasePortraitTexture = nullptr;
	m_pHuggedPortraitTexture = nullptr;
	m_pDeadPortriatTexture = nullptr;

	// Runtime
	m_WaterDropletFXStartTime = -1.0f;
	m_LastWaterSplashSoundTime = 0.0f;

	m_pHealthBarUI = nullptr;
	m_pCannonBallUI = nullptr;

	m_EndSnolafs[0] = m_EndSnolafs[1] = nullptr;

	m_pSheep = nullptr;
	m_p3000Ton = nullptr;

	m_pFox = nullptr;
}

/// KungFuLevelComponent::enable_internal
void KungFuLevelComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	m_pHealthBarUI = nullptr;
	m_pCannonBallUI = nullptr;
	m_EndSnolafs[0] = m_EndSnolafs[1] = nullptr;

	if (bEnable) {

		blk::error_check(s_Inst == nullptr, "KungFuLevelComponent::enable_internal() - Multiple enabled instances of KungFuLevelComponent");
		s_Inst = this;

		if (g_UseEditor == false) {
			g_ResourceManager.GetPackage("./assets/Packages/fx.kbPkg");
			g_ResourceManager.GetPackage("./assets/Packages/Snolaf.kbPkg");
			g_ResourceManager.GetPackage("./assets/Packages/Sheep.kbPkg");
			g_ResourceManager.GetPackage("./assets/Packages/3000Ton.kbPkg");
		}

		if (m_WaterDropletScreenFX.GetEntity() != nullptr) {

			for (int i = 0; i < NumWaterSplashes; i++) {
				blk::error_check(m_WaterSplashFXInst[i].m_Entity.GetEntity() == nullptr, "KungFuLevelComponent::enable_internal() - Water Droplet Screen FX Instance already allocated");

				m_WaterSplashFXInst[i].m_Entity.SetEntity(g_pGame->CreateEntity(m_WaterDropletScreenFX.GetEntity()));
				kbStaticModelComponent* const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>();
				pSM->Enable(false);
			}
		}

		for (int i = 0; i < KungFuGame::kSnolafPoolSize; i++) {
			kbGameEntity* const pSnolaf = g_pGame->CreateEntity(m_SnolafPrefab.GetEntity());
			ReturnSnolafToPool(pSnolaf->GetComponent<KungFuSnolafComponent>());
		}

	} else {

		if (s_Inst == this) {
			s_Inst = nullptr;
		}

		for (int i = 0; i < NumWaterSplashes; i++) {
			kbGameEntity* const pEnt = m_WaterSplashFXInst[i].m_Entity.GetEntity();
			if (pEnt != nullptr) {
				g_pGame->RemoveGameEntity(pEnt);
				m_WaterSplashFXInst[i].m_Entity.SetEntity(nullptr);
			}
		}
		if (g_pCannonGame->IsPlaying())
		{
			while (m_SnolafPool.size() > 0)
			{
				kbGameEntity* const pSnolaf = m_SnolafPool.back();
				m_SnolafPool.pop_back();
				g_pGame->RemoveGameEntity(pSnolaf);
			}
			m_SnolafPool.clear();
		}
		KungFuSheepDirector::DeleteSingleton();
	}
}

/// KungFuLevelComponent::update_internal
const Vec4 g_WaterDropletNormalFactorScroll[] = {
		Vec4(0.1000f, 0.1000f, 0.00000f, 0.01f),
		Vec4(0.1000f, 0.1000f, 0.00000f, 0.007f) };

const float g_WaterDropStartDelay[] = { 0.1f, 0.01f };

void KungFuLevelComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	static bool bKeyDown = false;
	if (bKeyDown == false)
	{
		if (GetAsyncKeyState('P'))
		{
			bKeyDown = true;
			std::vector<kbGameEntity*> gameEnts;
			if (g_UseEditor)
			{
				for (int i = 0; i < g_Editor->GetGameEntities().size(); i++)
				{
					gameEnts.push_back(g_Editor->GetGameEntities()[i]->GetGameEntity());
				}
			} else
			{
				for (int i = 0; i < g_pGame->GetGameEntities().size(); i++)
				{
					gameEnts.push_back(g_pGame->GetGameEntities()[i]);
				}
			}

			for (int i = 0; i < gameEnts.size(); i++)
			{
				static kbString TreeName("BG Trees");
				kbGameEntity* const pTargetEnt = gameEnts[i];
				if (pTargetEnt->GetName() != TreeName)
				{
					continue;
				}

				static float minScale = 250.0f;
				static float maxScale = 275.0f;
				const float randScale = (kbfrand() * (maxScale - minScale)) + minScale;
				Vec3 scale = Vec3Rand(Vec3(randScale, randScale, randScale), Vec3(randScale, randScale, randScale));
				pTargetEnt->SetScale(scale);

				Mat4 rotationMat = Mat4::identity;
				rotationMat.make_identity();

				float randRot = kbfrand() * kbPI * 2.0f;
				rotationMat[0][0] = cos(randRot);
				rotationMat[2][0] = -sin(randRot);
				rotationMat[0][2] = sin(randRot);
				rotationMat[2][2] = cos(randRot);
				pTargetEnt->SetOrientation(Quat4::from_mat4(rotationMat));
			}
		} else
		{
			bKeyDown = FALSE;
		}
	} else if (GetAsyncKeyState('P') == false)
	{
		bKeyDown = false;
	}

	if (g_UseEditor)
	{
		return;
	}

	// Not all game entities are loaded in enable_internal unfortunately
	if (m_pHealthBarUI == nullptr || m_pCannonBallUI == nullptr) {
		m_pHealthBarUI = nullptr;
		m_pCannonBallUI = nullptr;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size() && (m_pHealthBarUI == nullptr || m_pCannonBallUI == nullptr); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if (m_pHealthBarUI == nullptr) {
				m_pHealthBarUI = pTargetEnt->GetComponent<CannonHealthBarUIComponent>();
			}

			if (m_pCannonBallUI == nullptr) {
				m_pCannonBallUI = pTargetEnt->GetComponent<CannonBallUIComponent>();
			}

			if (m_BLM.GetEntity() == nullptr) {
				static kbString sBLM("#BLM");
				if (pTargetEnt->GetName() == sBLM) {
					m_BLM.SetEntity(pTargetEnt);
					pTargetEnt->DisableAllComponents();
				}
			}

			if (m_Credits.GetEntity() == nullptr) {
				static kbString sCredits("#Credits");
				if (pTargetEnt->GetName() == sCredits) {
					m_Credits.SetEntity(pTargetEnt);
					pTargetEnt->DisableAllComponents();
				}

			}
		}
	}

	if (KungFuSheepDirector::Get()->IsInitialized() == false) {

		KungFuGame_BaseState* pGameStates[KungFuGame::NumStates] = {
			new KungFuGame_MainMenuState(this),
			new KungFuGame_IntroGameState(this),
			new KungFuGame_GameplayState(this),
			new KungFuGame_PlayerDeadState(this),
			new KungFuGame_PausedState(this),
			new KungFuGame_OutroState(this),
		};

		KungFuSheepDirector::Get()->InitializeStateMachine(pGameStates);
		KungFuSheepDirector::Get()->RequestStateChange(KungFuGame::MainMenu);
	}

	if (m_p3000Ton == nullptr || m_PresentsEnt[0].GetEntity() == nullptr) {

		static const kbString sBossName("3000 Ton");
		static const kbString sPresent_1("Outro - Present_1");
		static const kbString sPresent_2("Outro - Present_2");
		static const kbString sBreakBridgeDecal("Outro - Bridge Decal");
		static const kbString sFox("Fox");
		static const kbString sBridgeExplosionFX("Outro - Bridge Explosion FX");

		// TODO - Optimize
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pEnt = g_pCannonGame->GetGameEntities()[i];
			if (pEnt->GetName() == sBossName) {
				m_p3000Ton = pEnt->GetComponent<CannonActorComponent>();
			} else if (pEnt->GetName() == sPresent_1) {
				m_PresentsEnt[0].SetEntity(pEnt);
			} else if (pEnt->GetName() == sPresent_2) {
				m_PresentsEnt[1].SetEntity(pEnt);
			} else if (pEnt->GetName() == sBreakBridgeDecal) {
				m_BridgeBreakDecal.SetEntity(pEnt);
			} else if (pEnt->GetName() == sFox) {
				m_pFox = pEnt->GetComponent<CannonActorComponent>();
			} else if (pEnt->GetName() == sBridgeExplosionFX) {
				m_BridgeExplosionFX.SetEntity(pEnt);
			}
		}
	}

	KungFuSheepDirector::Get()->UpdateStateMachine();

	if (m_WaterDropletFXStartTime > 0.0f) {

		int numFinished = 0;

		for (int i = 0; i < NumWaterSplashes; i++) {

			const float curTime = g_GlobalTimer.TimeElapsedSeconds();
			const float fxStartTime = m_WaterDropletFXStartTime + m_WaterSplashFXInst[i].m_InitialDelay;

			if (curTime < fxStartTime) {
				continue;
			}

			const float fxDuration = m_WaterSplashFXInst[i].m_Duration;
			float normalizedTime = (g_GlobalTimer.TimeElapsedSeconds() - fxStartTime) / fxDuration;
			kbStaticModelComponent* const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>();

			if (normalizedTime > 1.0f) {
				pSM->Enable(false);
				numFinished++;
			} else {
				pSM->Enable(true);
				const float delayScrollTime = g_WaterDropStartDelay[i];
				if (normalizedTime > delayScrollTime) {
					normalizedTime = kbClamp((normalizedTime - delayScrollTime) * (1.0f / delayScrollTime), 0.0f, 999.0f);
					static kbString normalFactor_scrollRate("normalFactor_scrollRate");
					Vec4 scroll = g_WaterDropletNormalFactorScroll[i];
					scroll.w *= -normalizedTime;

					pSM->set_material_param_vec4(0, normalFactor_scrollRate.stl_str(), scroll);

					// Blend out time
					{
						const float blendOutStart = fxStartTime + (fxDuration * 0.75f);
						const float blendOutTime = kbClamp((g_GlobalTimer.TimeElapsedSeconds() - blendOutStart) / (fxDuration * 0.25f), 0.0f, 1.0f);
						static kbString colorFactor("colorFactor");
						pSM->set_material_param_vec4(0, colorFactor.stl_str(), Vec4(1.0f, 1.0f, 1.0f, 1.0f - blendOutTime));
					}
				}
			}
		}

		if (numFinished == NumWaterSplashes) {
			m_WaterDropletFXStartTime = -1.0f;
		}
	}

	// Global Fog
	{
		kbShaderParamOverrides_t shaderParam;
		shaderParam.SetVec4("globalFogColor", Vec4(174.0f / 256.0f, 183.0f / 256.0f, 198.0f / 256.0f, 1.0f));
		g_pRenderer->SetGlobalShaderParam(shaderParam);
	}

	// Global Sun
	{
		float sunIntensity = 0.0f;
		float travelDist = GetPlayerTravelDistance();
		float startBlendInDist = 30.0f;
		sunIntensity = kbSaturate((GetPlayerTravelDistance() - startBlendInDist) / 75.0f);

		kbShaderParamOverrides_t shaderParam;
		shaderParam.SetVec4("globalSunFactor", Vec4(sunIntensity, sunIntensity, sunIntensity, sunIntensity));
		g_pRenderer->SetGlobalShaderParam(shaderParam);

		// globalSunFactor
	}

	// UI
	{
		if (m_pSheep && m_pHealthBarUI && m_pHealthBarUI->GetStaticRenderComponent()) {
			if (m_pSheep->GetCurrentState() == KungFuSheepState::Hugged) {
				m_pHealthBarUI->set_material_param_texture("baseTexture", m_pHuggedPortraitTexture);
			} else if (m_pSheep->GetCurrentState() == KungFuSheepState::Dead) {
				m_pHealthBarUI->set_material_param_texture("baseTexture", m_pDeadPortriatTexture);
			} else {
				m_pHealthBarUI->set_material_param_texture("baseTexture", m_pBasePortraitTexture);
			}
		}
	}
	UpdateDebugAndCheats();
}

/// KungFuLevelComponent::UpdateSheepHealthBar
void KungFuLevelComponent::UpdateSheepHealthBar(const float healthVal) {

	if (m_pHealthBarUI == nullptr) {
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size() && (m_pHealthBarUI == nullptr); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if (m_pHealthBarUI == nullptr) {
				m_pHealthBarUI = pTargetEnt->GetComponent<CannonHealthBarUIComponent>();
			}
		}
	}

	m_pHealthBarUI->SetTargetHealth(healthVal);
}

/// KungFuLevelComponent::UpdateCannonBallMeter
void KungFuLevelComponent::UpdateCannonBallMeter(const float fillVal, const bool bActivated) {

	if (m_pCannonBallUI == nullptr) {
		return;
	}

	if (bActivated) {
		m_pCannonBallUI->CannonBallActivatedCB();
	} else {
		m_pCannonBallUI->SetFill(fillVal);
	}
}


/// KungFuLevelComponent::SpawnSheep
KungFuSheepComponent* KungFuLevelComponent::SpawnSheep() {
	kbGameEntity* const sheep = g_pGame->CreateEntity(m_SheepPrefab.GetEntity());

	sheep->SetPosition(KungFuGame::kSheepStartPos);
	sheep->SetOrientation(KungFuGame::kSheepStartRot);

	m_pSheep = sheep->GetComponent<KungFuSheepComponent>();
	return m_pSheep;
}

/// KungFuLevelComponent::SpawnEnemy
void KungFuLevelComponent::SpawnEnemy(const bool bSpawnLeft, const int waveSize) {

	if (m_SnolafPrefab.GetEntity() == nullptr) {
		return;
	}

	// TODO - Polish
	const int subWaveSize = 3;//max( waveSize >> 2, 2 );
	const float startSpawnDist = 9.5f;
	const float spawnOffsets = KungFuGame::kDistBetweenSnolafs;

	const auto sheepPos = KungFuLevelComponent::Get()->GetSheep()->owner_position();
	Vec3 nextLeftSpawnPos = sheepPos + Vec3(0.0f, 0.0f, startSpawnDist);
	Vec3 nextRightSpawnPos = sheepPos + Vec3(0.0f, 0.0f, -startSpawnDist);

	if (m_EndSnolafs[0] != nullptr && m_EndSnolafs[0]->IsEnabled() && m_EndSnolafs[0]->IsDead() == false) {
		const float leftSnolafZ = m_EndSnolafs[0]->owner_position().z;
		if (nextLeftSpawnPos.z < leftSnolafZ + spawnOffsets) {
			nextLeftSpawnPos.z = leftSnolafZ + spawnOffsets;
		}
	}

	if (m_EndSnolafs[1] != nullptr && m_EndSnolafs[1]->IsEnabled() && m_EndSnolafs[1]->IsDead() == false) {
		const float rightSnolafZ = m_EndSnolafs[1]->owner_position().z;
		if (nextRightSpawnPos.z > rightSnolafZ - spawnOffsets) {
			nextRightSpawnPos.z = rightSnolafZ - spawnOffsets;
		}
	}

	int lastDir = -1;
	int curSubWaveSize = -1;
	int curDir = 0;
	int numConseq = 1;

	for (int i = 0; i < waveSize; i++) {

		float offsetMultiplier = 1.0f;
		float offsetAdd = 0;
		curSubWaveSize++;
		if (curSubWaveSize == 0 || curSubWaveSize >= subWaveSize) {
			curDir = rand() % 2;
			if (curSubWaveSize > 0 && curDir != lastDir && subWaveSize > 1 || numConseq > 4) {
				offsetAdd = KungFuGame::kSubWaveDirChangeInitialOffset;
				offsetMultiplier = KungFuGame::kSubWaveDirChangePerSnolafOffset;
				numConseq = 1;
			} else {
				numConseq++;
			}
			curSubWaveSize = 0;
			lastDir = curDir;
		}

		kbGameEntity* const pSnolaf = m_SnolafPool.back();
		m_SnolafPool.pop_back();//g_pGame->CreateEntity( m_SnolafPrefab.GetEntity() );
		KungFuSnolafComponent* const pSnolafComp = pSnolaf->GetComponent<KungFuSnolafComponent>();

		Vec3 spawnPos = Vec3::zero;
		if (bSpawnLeft == false && curDir == 0) {

			nextRightSpawnPos.z -= (spawnOffsets + offsetAdd) * offsetMultiplier;
			nextLeftSpawnPos.z += spawnOffsets;

			spawnPos = nextRightSpawnPos;
			m_EndSnolafs[1] = pSnolafComp;

		} else {
			nextLeftSpawnPos.z += (spawnOffsets + offsetAdd) * offsetMultiplier;
			nextRightSpawnPos.z -= spawnOffsets;

			spawnPos = nextLeftSpawnPos;
			m_EndSnolafs[0] = pSnolafComp;
		}
		//blk::log("Snolaf popped of list of size %d", m_SnolafPool.size() );

		pSnolaf->SetPosition(spawnPos);
		pSnolaf->SetOrientation(owner_rotation());

		pSnolafComp->RequestStateChange(KungFuSnolafState::Idle);
		pSnolafComp->ResetFromPool();
	}
}

/// KungFuLevelComponent::DoWaterDropletScreenFX
void KungFuLevelComponent::DoWaterDropletScreenFX() {

	if (m_WaterDropletScreenFX.GetEntity() == nullptr || m_WaterDropletFXStartTime > 0.0f) {
		return;
	}

	m_WaterDropletFXStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_WaterSplashFXInst[0].m_Duration = 1.5f;
	m_WaterSplashFXInst[0].m_InitialDelay = 1.0f;

	m_WaterSplashFXInst[1].m_Duration = 2.0f;
	m_WaterSplashFXInst[1].m_InitialDelay = 0.75f;

	//m_WaterDropletScreenFXInst.SetEntity( g_pGame->CreateEntity( m_WaterDropletScreenFX.GetEntity() ) );

	for (int i = 0; i < NumWaterSplashes; i++) {
		kbStaticModelComponent* const pSM = m_WaterSplashFXInst[i].m_Entity.GetEntity()->GetComponent<kbStaticModelComponent>();
		//	pSM->Enable( true );


		static kbString startUVOffsetParam("startUVOffset");
		pSM->set_material_param_vec4(0, startUVOffsetParam.stl_str(), Vec4(kbfrand(), kbfrand(), 0.0f, 0.0f));

		static kbString normalFactor_scrollRate("normalFactor_scrollRate");
		pSM->set_material_param_vec4(0, normalFactor_scrollRate.stl_str(), Vec4(g_WaterDropletNormalFactorScroll[i].x, g_WaterDropletNormalFactorScroll[i].y, 0.0f, 0.0f));

		static kbString colorFactor("colorFactor");
		pSM->set_material_param_vec4(0, colorFactor.stl_str(), Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

/// KungFuLevelComponent::DoSplashSound
void KungFuLevelComponent::DoSplashSound() {

	if (m_WaterSplashSound.size() == 0) {
		return;
	}

	if (g_GlobalTimer.TimeElapsedSeconds() < m_LastWaterSplashSoundTime + 0.1f) {
		return;
	}

	m_LastWaterSplashSoundTime = g_GlobalTimer.TimeElapsedSeconds();
	m_WaterSplashSound[rand() % m_WaterSplashSound.size()].PlaySoundAtPosition(Vec3(0.0f, 0.0f, 0.0f));
}

/// KungFuLevelComponent::DoBreakBridgeEffect
void KungFuLevelComponent::DoBreakBridgeEffect(const bool bBreakIt) {

	m_BridgeBreakDecal.GetEntity()->GetComponent<kbStaticModelComponent>()->Enable(bBreakIt);
	m_BridgeExplosionFX.GetEntity()->GetComponent<kbParticleComponent>()->Enable(bBreakIt);
}

/// KungFuLevelComponent::SetPlayLevelMusic
void KungFuLevelComponent::SetPlayLevelMusic(const int idx, const bool bPlay) {

	if (bPlay) {
		m_LevelMusic[idx].PlaySoundAtPosition(Vec3::zero);
	} else {
		m_LevelMusic[idx].StopSound();
	}
}

/// KungFuLevelComponent::ShowBLM
void KungFuLevelComponent::ShowBLM(const bool bShow) {

	if (m_BLM.GetEntity() == nullptr) {
		return;
	}

	if (bShow) {
		m_BLM.GetEntity()->EnableAllComponents();
	} else {
		m_BLM.GetEntity()->DisableAllComponents();
	}
}

/// KungFuLevelComponent::ShowCredits
void KungFuLevelComponent::ShowCredits(const bool bShow) {
	kbGameEntity* const pEnt = m_Credits.GetEntity();
	if (pEnt == nullptr) {
		return;
	}

	if (bShow) {
		pEnt->EnableAllComponents();
	} else {
		pEnt->DisableAllComponents();
	}
}

/// KungFuLevelComponent::ShowCredits
void KungFuLevelComponent::ShowHealthBar(const bool bShow) {

	if (m_pHealthBarUI == nullptr) {
		return;
	}

	if (bShow) {
		m_pHealthBarUI->Enable(true);
		m_pCannonBallUI->GetOwner()->EnableAllComponents();
	} else {
		m_pHealthBarUI->Enable(false);
		m_pCannonBallUI->GetOwner()->DisableAllComponents();
	}
}

/// KungFuLevelComponent::GetPlayerTravelDistance
float KungFuLevelComponent::GetPlayerTravelDistance() {

	return (g_pCannonGame->GetPlayer()->owner_position() - KungFuGame::kSheepStartPos).length();
}

/// KungFuLevelComponent::GetSnolafFromPool
KungFuSnolafComponent* KungFuLevelComponent::GetSnolafFromPool() {
	kbGameEntity* const pSnolaf = m_SnolafPool.back();
	pSnolaf->GetComponent<KungFuSnolafComponent>()->ResetFromPool();
	m_SnolafPool.pop_back();//g_pGame->CreateEntity( m_SnolafPrefab.GetEntity() );
	return pSnolaf->GetComponent<KungFuSnolafComponent>();
}

/// KungFuLevelComponent::ReturnSnolafToPool
void KungFuLevelComponent::ReturnSnolafToPool(KungFuSnolafComponent* const pSnolafComp) {

	kbGameEntity* const pSnolaf = pSnolafComp->GetOwner();
	pSnolaf->DisableAllComponents();

	m_SnolafPool.push_back(pSnolaf);

	if (pSnolafComp == m_EndSnolafs[0]) {
		m_EndSnolafs[0] = nullptr;
	} else if (pSnolafComp == m_EndSnolafs[1]) {
		m_EndSnolafs[1] = nullptr;
	}

	//	blk::log("Snolaf returned.  Pool size = %d", m_SnolafPool.size() );
}

void KungFuLevelComponent::RemoveSheep() {
	g_pGame->RemoveGameEntity(m_pSheep->GetOwner());
	m_pSheep = nullptr;
}

/// KungFuLevelComponent::UpdateDebugAndCheats
void KungFuLevelComponent::UpdateDebugAndCheats() {
	const kbInput_t& input = g_pInputManager->get_input();

	if (input.IsNonCharKeyPressedOrDown(kbInput_t::LCtrl)) {
		if (input.IsKeyPressedOrDown('D')) {
			DealAttackInfo_t<KungFuGame::eAttackType> damageInfo;
			damageInfo.m_BaseDamage = 999999.0f;
			damageInfo.m_pAttacker = nullptr;
			damageInfo.m_Radius = 10.0f;
			damageInfo.m_AttackType = KungFuGame::DebugDeath;

			m_pSheep->TakeDamage(damageInfo);
			g_pCannonGame->GetMainCamera()->SetTarget(nullptr, -1.0f);
		}

		if (input.IsKeyPressedOrDown('C')) {
			m_pSheep->m_CannonBallMeter = 2.0f;
			KungFuLevelComponent::Get()->UpdateCannonBallMeter(m_pSheep->m_CannonBallMeter, false);
		}

		if (input.WasKeyJustPressed('S')) {
			static float lastPlayTime = 0.0f;
			if (g_GlobalTimer.TimeElapsedSeconds() - lastPlayTime > 2.0f) {
				m_pSheep->PlayShakeNBakeFX();
				lastPlayTime = g_GlobalTimer.TimeElapsedSeconds();
			}
		}
	}

	static bool bOldGrass = false;
	static kbString BGName("BG Trees");
	if (g_CullGrass.GetBool() != bOldGrass) {
		bOldGrass = g_CullGrass.GetBool();
		g_bCullGrass = bOldGrass;

		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if (pTargetEnt->GetName() == BGName) {
				if (g_bCullGrass) {
					pTargetEnt->EnableAllComponents();
				} else {
					pTargetEnt->DisableAllComponents();
				}
			}

			kbTerrainComponent* const pTerrain = pTargetEnt->GetComponent<kbTerrainComponent>();
			if (pTerrain != nullptr) {
				pTerrain->RegenerateTerrain();
			}
		}
	}
}

/// KungFuSheepDirector::KungFuSheepDirector
KungFuSheepDirector::KungFuSheepDirector() :
	m_pHealthBarUI(nullptr),
	m_pCannonBallUI(nullptr),
	m_pMainMenuUI(nullptr),
	m_pPauseMenuUI(nullptr),
	m_NumHuggers(0),
	m_NumPrehuggers(0) {
}

/// KungFuSheepDirector::~KungFuSheepDirector
KungFuSheepDirector::~KungFuSheepDirector() {

}

/// KungFuSheepDirector::InitializeStateMachine_Internal
void KungFuSheepDirector::InitializeStateMachine_Internal() {

	kbLevelDirector::InitializeStateMachine_Internal();

	m_pHealthBarUI = nullptr;
	m_pMainMenuUI = nullptr;
	m_pPauseMenuUI = nullptr;
	m_pCannonBallUI = nullptr;

	m_NumHuggers = 0;
	m_NumPrehuggers = 0;
}

/// KungFuSheepDirector::ShutdownStateMachine_Internal
void KungFuSheepDirector::ShutdownStateMachine_Internal() {

	kbLevelDirector::ShutdownStateMachine_Internal();
	if (m_pHealthBarUI != nullptr) {

		m_pMainMenuUI->UnregisterEventListener(this);
		m_pPauseMenuUI->UnregisterEventListener(this);
	}
}

/// KungFuSheepDirector::CollectUIElements
void KungFuSheepDirector::CollectUIElements() {

	m_pHealthBarUI = nullptr;
	m_pCannonBallUI = nullptr;
	m_pMainMenuUI = nullptr;
	m_pPauseMenuUI = nullptr;

	for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
		kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
		if (m_pHealthBarUI == nullptr) {
			m_pHealthBarUI = pTargetEnt->GetComponent<CannonHealthBarUIComponent>();
		}

		if (m_pCannonBallUI == nullptr) {
			m_pCannonBallUI = pTargetEnt->GetComponent<CannonBallUIComponent>();
		}

		if (m_pMainMenuUI == nullptr) {
			m_pMainMenuUI = pTargetEnt->GetComponent<CannonBallMainMenuComponent>();
			if (m_pMainMenuUI) {
				m_pMainMenuUI->RegisterEventListener(this);
			}
		}

		if (m_pPauseMenuUI == nullptr) {
			m_pPauseMenuUI = pTargetEnt->GetComponent<CannonBallPauseMenuUIComponent>();
			if (m_pPauseMenuUI) {
				m_pPauseMenuUI->RegisterEventListener(this);
			}
		}
	}
}

/// KungFuSheepDirector::UpdateStateMachine
void KungFuSheepDirector::UpdateStateMachine() {
	kbLevelDirector::UpdateStateMachine();

	if (m_pHealthBarUI == nullptr) {
		CollectUIElements();

		if (GetCurrentState() == KungFuGame::MainMenu) {
			m_pHealthBarUI->GetOwner()->DisableAllComponents();
			m_pCannonBallUI->GetOwner()->DisableAllComponents();
		}
	}
	// TODO - Optimize
	m_NumHuggers = 0;
	m_NumPrehuggers = 0;
	for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

		kbGameEntity* const pEnt = g_pCannonGame->GetGameEntities()[i];
		if (pEnt->GetActorComponent() == nullptr) {
			continue;
		}

		KungFuSnolafComponent* const pSnolaf = pEnt->GetActorComponent()->GetAs<KungFuSnolafComponent>();
		if (pSnolaf == nullptr || pSnolaf->IsEnabled() == false) {
			continue;
		}

		if (pSnolaf->GetState() == KungFuSnolafState::Hug) {
			m_NumHuggers++;
		} else if (pSnolaf->GetState() == KungFuSnolafState::Prehug) {
			m_NumPrehuggers++;
		}
	}
}

/// KungFuSheepDirector::DoAttack
AttackHitInfo_t KungFuSheepDirector::DoAttack(const DealAttackInfo_t<KungFuGame::eAttackType> attackInfo) {

	AttackHitInfo_t attackHitInfo;
	if (this->m_CurrentState < 0 || m_CurrentState >= KungFuGame::NumStates) {
		return attackHitInfo;
	}

	return m_States[m_CurrentState]->DoAttack(attackInfo);
}

/// KungFuSheepDirector::StateChangeCB
void KungFuSheepDirector::StateChangeCB(const KungFuGame::eKungFuGame_State previousState, const KungFuGame::eKungFuGame_State nextState) {

	// TODO - Optimize
	if (m_pHealthBarUI == nullptr) {
		CollectUIElements();
	}

	if (g_SkipCheat == KungFuGame::Skip_MainMenuAndIntro || g_SkipCheat == KungFuGame::Skip_ToEnd || g_SkipCheat == KungFuGame::Skip_NoEnemies) {

		m_pHealthBarUI->GetOwner()->EnableAllComponents();
		m_pCannonBallUI->GetOwner()->EnableAllComponents();
		m_pMainMenuUI->GetOwner()->DisableAllComponents();

		if (nextState == KungFuGame::MainMenu) {
			for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

				kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
				if (pTargetEnt->GetComponent<KungFuSheepComponent>()) {
					continue;
				}

				if (pTargetEnt->GetComponent<KungFuSnolafComponent>()) {
					pTargetEnt->DisableAllComponents();
					g_pGame->GetLevelComponent<KungFuLevelComponent>()->ReturnSnolafToPool(pTargetEnt->GetComponent<KungFuSnolafComponent>());
				}
			}
			KungFuLevelComponent::Get()->SetPlayLevelMusic(1, false);
		}

		return;
	}

	if (nextState == KungFuGame::Gameplay) {
		if (m_pHealthBarUI != nullptr) {
			m_pHealthBarUI->GetOwner()->EnableAllComponents();
			m_pCannonBallUI->GetOwner()->EnableAllComponents();
		}
	} else if (nextState == KungFuGame::MainMenu) {
		if (m_pHealthBarUI != nullptr) {
			m_pHealthBarUI->GetOwner()->DisableAllComponents();
			m_pCannonBallUI->GetOwner()->DisableAllComponents();

			m_pMainMenuUI->GetOwner()->DisableAllComponents();
			m_pMainMenuUI->GetOwner()->EnableAllComponents();
		}

		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if (pTargetEnt->GetComponent<KungFuSheepComponent>()) {
				continue;
			}

			if (pTargetEnt->GetComponent<KungFuSnolafComponent>()) {
				pTargetEnt->DisableAllComponents();
				g_pGame->GetLevelComponent<KungFuLevelComponent>()->ReturnSnolafToPool(pTargetEnt->GetComponent<KungFuSnolafComponent>());
			}
		}

		KungFuLevelComponent::Get()->SetPlayLevelMusic(1, false);

	} else if (nextState == KungFuGame::Intro) {
		if (m_pMainMenuUI != nullptr) {
			m_pMainMenuUI->SetAnimationFrame(1);
		}
	} else if (previousState == KungFuGame::MainMenu && nextState == KungFuGame::Paused) {
		if (m_pMainMenuUI != nullptr) {
			m_pMainMenuUI->GetOwner()->DisableAllComponents();
		}
	}
}

/// KungFuSheepDirector::WidgetEventCB
void KungFuSheepDirector::WidgetEventCB(kbUIWidgetComponent* const pWidget, const kbInput_t* const pInput) {

	m_States[m_CurrentState]->WidgetEventCB(pWidget, pInput);
}
