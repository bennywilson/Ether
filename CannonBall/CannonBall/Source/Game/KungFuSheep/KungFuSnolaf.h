//==============================================================================
// KungFuSnolaf.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFUSNOLAF_H_
#define _KUNGFUSNOLAF_H_


/// KungFuSheepState
namespace KungFuSnolafState {
	enum SnolafState_t {
		Idle = 0,
		Run,
		Prehug,
		Hug,
		Dead,
		ForcePoofDeath,
		WatchCannonBall,
		RunAway,
		Cinema,
		NumStates
	};
}

/// KungFuSnolafComponent

template<typename T>
class KungFuSnolafStateBase;

class KungFuSnolafComponent : public CannonActorComponent, public IStateMachine<KungFuSnolafStateBase<KungFuSnolafState::SnolafState_t>, KungFuSnolafState::SnolafState_t> {
	KB_DECLARE_COMPONENT(KungFuSnolafComponent, CannonActorComponent);

	//---------------------------------------------------------------------------------------------------'
public:

	void										ResetFromPool();

	void										EnableSmallLoveHearts(const bool bEnable);
	void										EnableLargeLoveHearts(const bool bEnable);

	void										TakeDamage(const DealAttackInfo_t<KungFuGame::eAttackType>& dealAttackInfo);

	void										DoPoofDeath();
	void										SpawnAndFlingDecapHead();
	void										SpawnAndFlingTopAndBottomHalf();
	void										SpawnSplash();

	KungFuSnolafState::SnolafState_t			GetState() const { return m_CurrentState; }
	const DealAttackInfo_t<KungFuGame::eAttackType>& GetLastAttackInfo() const { return m_LastAttackInfo; }

protected:

	virtual void								SetEnable_Internal(const bool bEnable) override;
	virtual void								Update_Internal(const float DeltaTime) override;

private:

	// Data
	kbGameEntityPtr								m_FootStepImpactFX;
	kbGameEntityPtr								m_PoofDeathFX;
	kbGameEntityPtr								m_DecapitatedHead;
	kbGameEntityPtr								m_TopHalfOfBody;
	kbGameEntityPtr								m_BottomHalfOfBody;
	kbGameEntityPtr								m_SplashFX;

	// Game
	kbParticleComponent* m_pSmallLoveHearts;
	kbParticleComponent* m_pLargeLoveHearts;

	DealAttackInfo_t<KungFuGame::eAttackType>	m_LastAttackInfo;

	float										m_DeathTimer;

	//---------------------------------------------------------------------------------------------------
		// IAnimEventListener
	virtual void								OnAnimEvent(const kbAnimEventInfo_t& animEvent) override;
};

template<typename T>
class KungFuSnolafStateBase : public CannonBallCharacterState<T> {

	//---------------------------------------------------------------------------------------------------'
public:

	KungFuSnolafStateBase(CannonActorComponent* const pPlayerComponent) : CannonBallCharacterState<T>(pPlayerComponent) { }

protected:

	KungFuSnolafComponent* GetSnolaf() const { return (KungFuSnolafComponent*)this->m_pActorComponent; }
	CannonActorComponent* GetTarget() const { return g_pCannonGame->GetPlayer(); }

	float GetDistanceToTarget() {
		if (GetTarget() == nullptr) {
			return -1.0f;
		}

		const Vec3 targetPos = GetTarget()->owner_position();
		const Vec3 snolafPos = this->m_pActorComponent->owner_position();
		if (targetPos.compare(snolafPos) == true) {
			return 0.0f;
		}

		return (targetPos - snolafPos).length();
	}

	void RotateTowardTarget() {
		if (GetTarget() == nullptr) {
			return;
		}

		const Vec3 targetPos = GetTarget()->owner_position();
		const Vec3 snolafPos = this->m_pActorComponent->owner_position();

		this->m_pActorComponent->SetTargetFacingDirection((snolafPos - targetPos).normalize_safe());
	}

	bool IsTargetOnLeft() const {
		if (GetTarget() == nullptr) {
			return false;
		}

		const Vec3 targetPos = GetTarget()->owner_position();
		const Vec3 snolafPos = this->m_pActorComponent->owner_position();
		return (targetPos.z > snolafPos.z);
	}
};


#endif