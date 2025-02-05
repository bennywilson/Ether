//==============================================================================
// KungFuSheep.h
//
// 2019-2025 kbEngine 2.0
//==============================================================================
#ifndef _KBKUNGFUSHEEP_H_
#define _KBKUNGFUSHEEP_H_

#include "kbCore.h"
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuLevelComponent.h"
#include "KungFuSnolaf.h"

/**
 *	KungFuSheepStateBase
 */
namespace KungFuSheepState {
	enum SheepStates_t {
		Idle = 0,
		Run,
		Attack,
		Hugged,
		Dead,
		CannonBall,
		Cinema,
		NumStates
	};
}

/**
 *	KungFuSheepComponent
 */

template<typename T>
class KungFuSheepStateBase;

class KungFuSheepComponent : public CannonActorComponent, IStateMachine<KungFuSheepStateBase<KungFuSheepState::SheepStates_t>, KungFuSheepState::SheepStates_t> {
	KB_DECLARE_COMPONENT(KungFuSheepComponent, CannonActorComponent);

	friend class KungFuSheepStateBase<KungFuSheepState::SheepStates_t>;
	friend class KungFuLevelComponent;

	//---------------------------------------------------------------------------------------------------
public:

	bool										IsCannonBalling() const;
	void										TakeDamage(const DealAttackInfo_t<KungFuGame::eAttackType>& dealAttackInfo);

	void										PlayShakeNBakeFX();
	void										PlayBaa(const int baaType);
	void										HitASnolaf();
	void										PlayImpactSound();

	void										PlayCannonBallFX(const kbVec3 location);
	void										PlayCameraShake();

	void										EnableHeadBand(const bool bEnable);

	void										SpawnSplash();

	float										GetCannonBallMeterFill() const { return m_CannonBallMeter; }
	void										CannonBallActivatedCB();

	void										ExternalRequestStateChange(const KungFuSheepState::SheepStates_t);

protected:

	virtual void								SetEnable_Internal(const bool bEnable) override;
	virtual void								Update_Internal(const float DeltaTime) override;

private:

	// Data
	kbGameEntityPtr								m_CannonBallImpactFX;
	kbGameEntityPtr								m_ShakeNBakeFX;
	kbGameEntityPtr								m_SplashFX;

	std::vector<kbSoundData>					m_CannonBallVO;
	std::vector<kbSoundData>					m_BaaaVO;
	std::vector<kbSoundData>					m_CannonBallImpactSound;
	std::vector<kbSoundData>					m_BasicAttackImpactSound;

	float										m_JumpSmearMagnitude;
	float										m_DropSmearMagnitude;

	// Run time
	kbGameEntityPtr								m_HeadBand;
	kbGameEntityPtr								m_HeadBandInstance[2];

	float										m_CannonBallMeter;

	//---------------------------------------------------------------------------------------------------

		// IAnimEventListener
	virtual void								OnAnimEvent(const kbAnimEventInfo_t& animEvent) override;
};

template<typename T>
class KungFuSheepStateBase : public CannonBallCharacterState<T> {

	//---------------------------------------------------------------------------------------------------'
public:

	KungFuSheepStateBase(CannonActorComponent* const pPlayerComponent) : CannonBallCharacterState<T>(pPlayerComponent) { }

protected:

	KungFuSheepComponent* GetSheep() const { return (KungFuSheepComponent*)this->m_pActorComponent; }

	bool CheckForBlocker(const kbVec3 moveVec) {

		if (KungFuSheepDirector::Get()->GetNumHuggersAndPrehuggers() > 0) {
			return true;
		}

		const float SheepZ = GetSheep()->GetOwnerPosition().z;
		const kbVec3 SheepDest = GetSheep()->GetOwnerPosition() + moveVec;
		//g_pRenderer->DrawLine( GetSheep()->GetOwnerPosition() ,  GetSheep()->GetOwnerPosition() + moveVec + kbVec3( 0, 1, 0 ) , kbColor::red );

		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			KungFuSnolafComponent* const pSnolaf = pGameEnt->GetComponent<KungFuSnolafComponent>();
			if (pSnolaf == nullptr || pSnolaf->IsDead() || pSnolaf->IsEnabled() == false) {
				continue;
			}

			const kbVec3 snolafPos = pSnolaf->GetOwnerPosition();
			if (snolafPos.z < SheepZ && snolafPos.z > SheepDest.z) {
				return true;
			}

			if (snolafPos.z > SheepZ && snolafPos.z < SheepDest.z) {
				return true;
			}
		}

		return false;
	}
};


#endif