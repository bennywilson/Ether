//===================================================================================================
// CapOnItLevelComponent.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "CannonGame.h"
#include "UI/CannonUI.h"
#include "CapOnItLevelComponent.h"
#include "CannonPlayer.h"

namespace CapOnIt {

	enum eSkipCheats {
		Skip_None,
		Skip_ToSkkt,
	};

};

CapOnIt::eSkipCheats g_SkipCheat = CapOnIt::Skip_ToSkkt;

/// KungFuGame_MainMenuState
class CapOnIt_MainMenuState : public CapOnIt_BaseState {

	//---------------------------------------------------------------------------------------------------
public:

	CapOnIt_MainMenuState(CapOnItLevelComponent* const pLevelComponent) : CapOnIt_BaseState(pLevelComponent) { }


private:

	virtual void WidgetEventCB(kbUIWidgetComponent* const pWidget, const kbInput_t* pInput) override {
	}

	virtual void BeginState_Internal(CapOnIt::eCapOnIt_State previousState) override { }

	virtual void UpdateState_Internal() override {

		/*if ( m_bCameraSet == false ) {
			if ( g_pCannonGame->GetMainCamera() != nullptr ) {
				m_bCameraSet = true;
				g_pCannonGame->GetMainCamera()->SetTarget( nullptr, -1.0f );
				g_pCannonGame->GetMainCamera()->SetOwnerPosition( KungFuGame::kCameraStartPos );
				g_pCannonGame->GetMainCamera()->SetOwnerRotation( KungFuGame::kCameraRotation );
			}
		}
		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		if ( pSheep == nullptr ) {
			pSheep = m_pLevelComponent->SpawnSheep();
		}

		if ( g_SkipCheat == KungFuGame::Skip_MainMenuAndIntro || g_SkipCheat == KungFuGame::Skip_ToEnd ) {

			static const kbString IdleL_Anim("IdleLeft_Basic");
			pSheep->PlayAnimation( IdleL_Anim, 0.2f );
			RequestStateChange( KungFuGame::Intro );

			if ( g_SkipCheat == KungFuGame::Skip_ToEnd ) {
				pSheep->SetOwnerPosition( Vec3( 77.10445f, -52.6362f, KungFuGame::kOutroStartZ ) );
			}
			return;
		}

		pSheep->ExternalRequestStateChange( KungFuSheepState::Cinema );

		static const kbString JumpingJacks_Anim("JumpingJacks");
		pSheep->PlayAnimation( JumpingJacks_Anim, 0.15f );
		pSheep->SetTargetFacingDirection( Vec3( -1.0f, 0.0f, -1.0f ).Normalized() );
		pSheep->SetOwnerPosition( KungFuGame::kSheepStartPos );*/
	}

	virtual void EndState_Internal(CapOnIt::eCapOnIt_State nextState) override {
	}

};

CapOnItLevelComponent* CapOnItLevelComponent::s_Inst = nullptr;

/// CapOnItLevelComponent::Constructor
void CapOnItLevelComponent::Constructor() {
}

/// CapOnItLevelComponent::enable_internal
void CapOnItLevelComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (bEnable) {
		blk::error_check(s_Inst == nullptr, "KungFuLevelComponent::enable_internal() - Multiple enabled instances of KungFuLevelComponent");
		s_Inst = this;
	} else {
	}
}

/// CapOnItLevelComponent::update_internal
void CapOnItLevelComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (g_UseEditor) {
		return;
	}



	UpdateDebugAndCheats();
}

/// CapOnItLevelComponent::UpdateDebugAndCheats
void CapOnItLevelComponent::UpdateDebugAndCheats() {

	/*const auto input = g_pInputManager->GetInput();
	if ( input.IsNonCharKeyPressedOrDown( kbInput_t::LCtrl ) ) {

		if ( input.IsKeyPressedOrDown( 'D' ) ) {
			DealAttackInfo_t<KungFuGame::eAttackType> damageInfo;
			damageInfo.m_BaseDamage = 999999.0f;
			damageInfo.m_pAttacker = nullptr;
			damageInfo.m_Radius = 10.0f;
			damageInfo.m_AttackType = KungFuGame::DebugDeath;

			m_pSheep->TakeDamage( damageInfo );
			g_pCannonGame->GetMainCamera()->SetTarget( nullptr, -1.0f );
		}

		if ( input.IsKeyPressedOrDown( 'C' ) ) {
			m_pSheep->m_CannonBallMeter = 2.0f;
			KungFuLevelComponent::Get()->UpdateCannonBallMeter( m_pSheep->m_CannonBallMeter, false );
		}
	}*/
}

/// CapOnItDirector::CapOnItDirector
CapOnItDirector::CapOnItDirector() {
}

/// CapOnItDirector::~CapOnItDirector
CapOnItDirector::~CapOnItDirector() {

}

/// CapOnItDirector::InitializeStateMachine_Internal
void CapOnItDirector::InitializeStateMachine_Internal() {

	kbLevelDirector::InitializeStateMachine_Internal();
}

/// CapOnItDirector::ShutdownStateMachine_Internal
void CapOnItDirector::ShutdownStateMachine_Internal() {

	kbLevelDirector::ShutdownStateMachine_Internal();
}

/// CapOnItDirector::UpdateStateMachine
void CapOnItDirector::UpdateStateMachine() {
	kbLevelDirector::UpdateStateMachine();

}

/// CapOnItDirector::StateChangeCB
void CapOnItDirector::StateChangeCB(const CapOnIt::eCapOnIt_State previousState, const CapOnIt::eCapOnIt_State nextState) {

}

/// CapOnItDirector::WidgetEventCB
void CapOnItDirector::WidgetEventCB(kbUIWidgetComponent* const pWidget, const kbInput_t* const pInput) {

	m_States[m_CurrentState]->WidgetEventCB(pWidget, pInput);
}
