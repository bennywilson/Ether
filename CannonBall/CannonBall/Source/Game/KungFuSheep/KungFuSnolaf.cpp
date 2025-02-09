/// KungFuSnolaf.cpp
///
/// 2019-2025 blk1.0

#include <DirectXMath.h>
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuLevelComponent.h"
#include "KungFuSnolaf.h"
#include "KungFuSheep.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"
#include <DX11\kbRenderer_DX11.h>

/// KungFuSnolafStateIdle
template<typename T>
class KungFuSnolafStateIdle : public KungFuSnolafStateBase<T> {
public:
	KungFuSnolafStateIdle(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T prevState) override {
		static const kbString Idle_Anim("Idle_1");
		this->m_pActorComponent->PlayAnimation(Idle_Anim, 0.05f);
	}

	virtual void UpdateState_Internal() override {
		if (this->GetTarget() != nullptr) {
			this->RotateTowardTarget();
			if (this->GetDistanceToTarget() >= KungFuGame::kDistToChase) {
				this->RequestStateChange(KungFuSnolafState::Run);
				return;
			}
		}
	}

	virtual void EndState_Internal(T) override { }
};

/// KungFuSnolafStateRun
template<typename T>
class KungFuSnolafStateRun : public KungFuSnolafStateBase<T> {
private:
	float m_HugStartTime = -1.0f;

public:
	KungFuSnolafStateRun(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		static const kbString Run_Anim("Run");
		this->m_pActorComponent->PlayAnimation(Run_Anim, 0.05f);

		this->GetSnolaf()->EnableSmallLoveHearts(true);
		m_HugStartTime = -1.0f;
	}

	virtual void UpdateState_Internal() override {

		const float frameDT = g_pGame->GetFrameDT();

		if (this->GetTarget() == nullptr) {
			this->RequestStateChange(KungFuSnolafState::Idle);
			return;
		}

		const Vec3 snolafPos = this->m_pActorComponent->owner_position();
		const Vec3 snolafFacingDir = this->m_pActorComponent->owner_rotation().to_mat4()[2].ToVec3();

		// TODO - Optimize
		// Look for actors to hug
		bool bFoundHugger = false;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent* const pTargetActor = pGameEnt->GetComponent<CannonActorComponent>();
			if (pTargetActor == nullptr || pTargetActor == this->m_pActorComponent) {
				continue;
			}

			if (pTargetActor->GetAs<KungFuSheepComponent>() == nullptr && pTargetActor->GetAs<KungFuSnolafComponent>() == nullptr) {
				continue;
			}

			const Vec3 targetPos = pTargetActor->owner_position();
			const Vec3 vSnolafToTarget = targetPos - snolafPos;
			const float snolafToTargetDist = (targetPos - snolafPos).length();
			auto pSnolafComponent = pTargetActor->GetAs<KungFuSnolafComponent>();
			if (pSnolafComponent != nullptr && pSnolafComponent->GetState() != KungFuSnolafState::Prehug && pSnolafComponent->GetState() != KungFuSnolafState::Hug && pSnolafComponent->GetState() != KungFuSnolafState::WatchCannonBall) {
				continue;
			}

			const float radius = (pSnolafComponent != nullptr) ? (KungFuGame::kDistToHugSnolaf) : (KungFuGame::kDistToHugSheep);
			if (snolafToTargetDist < radius) {
				if (vSnolafToTarget.dot(snolafFacingDir) > 0.0f) {
					continue;
				}

				this->RequestStateChange(KungFuSnolafState::Prehug);

				return;
			}
		}

		// Move towards target
		this->RotateTowardTarget();

		Vec3 moveDir(0.0f, 0.0f, 0.0f);
		if (this->IsTargetOnLeft()) {
			moveDir.z = 1.0f;
		} else {
			moveDir.z = -1.0f;
		}
		const Vec3 newSnolafPos = this->m_pActorComponent->owner_position() + moveDir * frameDT * this->m_pActorComponent->GetMaxRunSpeed();
		this->m_pActorComponent->SetOwnerPosition(newSnolafPos);
	}

	virtual void EndState_Internal(T) override {
		this->GetSnolaf()->EnableSmallLoveHearts(false);
	}
};

/// KungFuSnolafeStatePrehug
template<typename T>
class KungFuSnolafeStatePrehug : public KungFuSnolafStateBase<T> {
public:

	KungFuSnolafeStatePrehug(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		static const kbString HugForward_Anim("Hug_Forward");
		this->m_pActorComponent->PlayAnimation(HugForward_Anim, 0.15f);
	}

	virtual void UpdateState_Internal() override {
		if (this->GetTimeSinceStateBegan() > KungFuGame::kPrehugLengthSec) {
			this->RequestStateChange(KungFuSnolafState::Hug);
			return;
		}

		const Vec3 snolafPos = this->m_pActorComponent->owner_position();
		const Vec3 snolafFacingDir = this->m_pActorComponent->owner_rotation().to_mat4()[2].ToVec3();

		// TODO - Optimize
		bool bAnyoneInFront = false;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {
			kbGameEntity* const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent* const pTargetActor = pGameEnt->GetComponent<CannonActorComponent>();
			if (pTargetActor == nullptr || pTargetActor == this->m_pActorComponent) {
				continue;
			}

			if (pTargetActor->IsEnabled() == false || pTargetActor->IsDead()) {
				continue;
			}

			const Vec3 targetPos = pTargetActor->owner_position();
			const Vec3 vSnolafToTarget = targetPos - snolafPos;
			const float snolafToTargetDist = (targetPos - snolafPos).length();
			if (snolafToTargetDist < KungFuGame::kDistToChase) {
				if (vSnolafToTarget.dot(snolafFacingDir) > 0.0f) {
					continue;
				}

				bAnyoneInFront = true;
				break;
			}
		}

		if (bAnyoneInFront == false) {
			this->RequestStateChange(KungFuSnolafState::Run);
			return;
		}

	}

	virtual void EndState_Internal(T) override { }
};

/// KungFuSnolafStateHug - Warm Hugs!
template<typename T>
class KungFuSnolafStateHug : public KungFuSnolafStateBase<T> {
public:
	KungFuSnolafStateHug(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		static const kbString HugLeft_Anim("Hug_Left");
		static const kbString HugRight_Anim("Hug_Right");
		static const kbString HugForward_Anim("Hug_Forward");

		if (this->GetTarget() == nullptr) {
			this->RequestStateChange(KungFuSnolafState::Idle);
			return;
		}

		KungFuSheepComponent* const pSheep = this->GetTarget()->GetAs<KungFuSheepComponent>();
		if (pSheep->IsCannonBalling()) {
			this->m_pActorComponent->PlayAnimation(HugForward_Anim, 0.15f);
			m_bCanWatchCannonball = false;
		} else {

			if (this->IsTargetOnLeft()) {
				this->m_pActorComponent->PlayAnimation(HugLeft_Anim, 0.05f);
			} else {
				this->m_pActorComponent->PlayAnimation(HugRight_Anim, 0.05f);
			}

			this->GetSnolaf()->EnableLargeLoveHearts(true);
			m_bCanWatchCannonball = true;
		}

		m_HugStartTime = g_GlobalTimer.TimeElapsedSeconds();
		m_bFirstHitYet = false;
	}

	virtual void UpdateState_Internal() override {

		const float frameDT = g_pGame->GetFrameDT();

		if (this->GetTarget() == nullptr) {
			this->RequestStateChange(KungFuSnolafState::Idle);
			return;
		}

		KungFuSheepComponent* const pSheep = this->GetTarget()->GetAs<KungFuSheepComponent>();
		if (m_bCanWatchCannonball && pSheep->IsCannonBalling()) {
			this->RequestStateChange(KungFuSnolafState::WatchCannonBall);
			return;
		}

		if (pSheep != nullptr && m_bFirstHitYet == false && g_GlobalTimer.TimeElapsedSeconds() > m_HugStartTime + 0.25f) {
			m_bFirstHitYet = true;

			DealAttackInfo_t<KungFuGame::eAttackType> dealAttackInfo;
			dealAttackInfo.m_BaseDamage = 999999.0f;
			dealAttackInfo.m_pAttacker = this->m_pActorComponent;
			dealAttackInfo.m_Radius = 0.0f;
			dealAttackInfo.m_AttackType = KungFuGame::Hug;
			KungFuSheepDirector::Get()->DoAttack(dealAttackInfo);
		}

		const Vec3 snolafPos = this->m_pActorComponent->owner_position();
		const Vec3 snolafFacingDir = this->m_pActorComponent->owner_rotation().to_mat4()[2].ToVec3();

		bool bAnyoneInFront = false;
		for (int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++) {

			kbGameEntity* const pGameEnt = g_pCannonGame->GetGameEntities()[i];
			CannonActorComponent* const pTargetActor = pGameEnt->GetComponent<CannonActorComponent>();
			if (pTargetActor == nullptr || pTargetActor == this->m_pActorComponent) {
				continue;
			}

			if (pTargetActor->IsEnabled() == false || pTargetActor->IsDead()) {
				continue;
			}

			const Vec3 targetPos = pTargetActor->owner_position();
			const Vec3 vSnolafToTarget = targetPos - snolafPos;
			const float snolafToTargetDist = (targetPos - snolafPos).length();
			if (snolafToTargetDist < KungFuGame::kDistToChase) {

				if (vSnolafToTarget.dot(snolafFacingDir) > 0.0f) {
					continue;
				}

				bAnyoneInFront = true;
				break;
			}
		}

		if (bAnyoneInFront == false) {
			this->RequestStateChange(KungFuSnolafState::Run);
			return;
		}
	}

	virtual void EndState_Internal(T) override {
		this->GetSnolaf()->EnableLargeLoveHearts(false);
	}

private:
	bool m_bCanWatchCannonball = false;
	float m_HugStartTime = 0.0f;
	bool m_bFirstHitYet = false;
};

/// KungFuSnolafStateWatchCannonBall
template<typename T>
class KungFuSnolafStateWatchCannonBall : public KungFuSnolafStateBase<T> {
public:
	KungFuSnolafStateWatchCannonBall(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }


	virtual void BeginState_Internal(T) override {

		static const kbString Watch_CannonBall("Watch_CannonBall");
		this->m_pActorComponent->PlayAnimation(Watch_CannonBall, 0.05f);
	}

	virtual void UpdateState_Internal() override { }

	virtual void EndState_Internal(T) override { }
};

/// KungFuSnolafStateDead
template<typename T>
class KungFuSnolafStateDead : public KungFuSnolafStateBase<T> {
public:
	KungFuSnolafStateDead(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		m_DeathStartTime = g_GlobalTimer.TimeElapsedSeconds();

		this->GetSnolaf()->EnableSmallLoveHearts(false);
		this->GetSnolaf()->EnableLargeLoveHearts(false);

		const DealAttackInfo_t<KungFuGame::eAttackType>& lastAttackInfo = this->GetSnolaf()->GetLastAttackInfo();
		if (lastAttackInfo.m_AttackType == KungFuGame::Shake) {
			m_DeathSelection = 0;
		} else {
			const int numDeaths = 4;
			m_DeathSelection = rand() % numDeaths;
		}

		m_OwnerStartPos = this->GetSnolaf()->owner_position();
		m_OwnerStartRotation = this->m_pActorComponent->owner_rotation();

		kbGameEntity* const pOwner = this->GetSnolaf()->GetOwner();
		for (int i = 0; i < pOwner->NumComponents(); i++) {
			kbParticleComponent* const pParticle = pOwner->GetComponent(i)->GetAs<kbParticleComponent>();
			if (pParticle == nullptr) {
				continue;
			}

			pParticle->Enable(false);
		}

		Mat4 worldMatrix;
		this->m_pActorComponent->GetOwner()->CalculateWorldMatrix(worldMatrix);
		const XMMATRIX inverseMat = XMMatrixInverse(nullptr, XMMATRIXFromMat4(worldMatrix));
		worldMatrix = Mat4FromXMMATRIX(inverseMat);

		if (m_DeathSelection == 0) {

			// Super Fly Off
			m_Velocity = Vec3Rand(m_MinLinearVelocity, m_MaxLinearVelocity);
			if (m_Velocity.x < 0.0f) {
				m_Velocity.x -= 0.0025f;
			} else {
				m_Velocity.x += 0.0025f;
			}

			m_Velocity = m_Velocity * worldMatrix;

			m_RotationAxis = Vec3(kbfrand() - 1.3f, 0.0f, 0.0f);
			if (m_RotationAxis.length_sqr() < 0.01f) {
				m_RotationAxis.set(1.0f, 0.0f, 0.0f);
			} else {
				m_RotationAxis.normalize_self();
			}
			m_RotationSpeed = kbfrand() * (m_MaxAngularVelocity - m_MinAngularVelocity) + m_MinAngularVelocity;

			const Vec3 initialSnolafOffset = m_Velocity.normalize_safe() * 2.0f;
			m_OwnerPosOverride = this->m_pActorComponent->owner_position() + initialSnolafOffset;
			m_OwnerStartPos = m_OwnerPosOverride;
			this->GetSnolaf()->ApplyAnimSmear(-initialSnolafOffset * 0.75f, 0.067f);
			UpdateFlyingDeath(0.0f);

		} else if (m_DeathSelection == 1) {
			// Straight up poof, homie
			this->GetSnolaf()->DoPoofDeath();

		} else if (m_DeathSelection == 2) {
			// Decapitation
			this->GetSnolaf()->SpawnAndFlingDecapHead();
			m_Velocity = Vec3Rand(m_MinLinearVelocity, m_MaxLinearVelocity);
			if (m_Velocity.x < 0.0f) {
				m_Velocity.x -= 0.0025f;
			} else {
				m_Velocity.x += 0.0025f;
			}

			m_Velocity = 2.0f * m_Velocity * worldMatrix;

			m_RotationAxis = Vec3(kbfrand() - 1.3f, 0.0f, 0.0f);
			if (m_RotationAxis.length_sqr() < 0.01f) {
				m_RotationAxis.set(1.0f, 0.0f, 0.0f);
			} else {
				m_RotationAxis.normalize_self();
			}
			m_RotationSpeed = 0.15f * (kbfrand() * (m_MaxAngularVelocity - m_MinAngularVelocity) + m_MinAngularVelocity);

			const Vec3 initialSnolafOffset = m_Velocity.normalize_safe() * 2.0f;
			m_OwnerPosOverride = this->m_pActorComponent->owner_position() + initialSnolafOffset;
			m_OwnerStartPos = m_OwnerPosOverride;
			this->GetSnolaf()->ApplyAnimSmear(-initialSnolafOffset * 0.75f, 0.067f);
			UpdateFlyingDeath(0.0f);

			const static kbString clipMapMaskParam("clipMapMask");
			kbGameEntity* const pOwner = this->m_pActorComponent->GetOwner();
			for (int i = 0; i < pOwner->NumComponents(); i++) {
				kbSkeletalRenderComponent* const pSkelModelComp = pOwner->GetComponent(i)->GetAs<kbSkeletalRenderComponent>();
				if (pSkelModelComp == nullptr) {
					continue;
				}
				pSkelModelComp->set_material_param_vec4(0, clipMapMaskParam.stl_str(), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
			}
		} else if (m_DeathSelection == 3) {
			this->GetSnolaf()->SpawnAndFlingTopAndBottomHalf();
		}

		m_bSpawnedSplash = false;
	}

	void UpdateFlyingDeath(const float dt) {
		kbGameEntity* const pOwner = this->m_pActorComponent->GetOwner();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float elapsedDeathTime = curTime - m_DeathStartTime;

		m_OwnerPosOverride.x += m_Velocity.x * dt;
		m_OwnerPosOverride.z += m_Velocity.z * dt;

		m_OwnerPosOverride.y = m_OwnerStartPos.y + m_Velocity.y * elapsedDeathTime - (0.5f * -m_Gravity.y * elapsedDeathTime * elapsedDeathTime);

		this->m_pActorComponent->SetOwnerPosition(m_OwnerPosOverride);

		const static kbString spine3BoneName("Spine3");
		kbSkeletalRenderComponent* const pSnolafComp = pOwner->GetComponent<kbSkeletalRenderComponent>();
		Vec3 spine3WorldPos = Vec3::zero;

		if (pSnolafComp->GetBoneWorldPosition(spine3BoneName, spine3WorldPos)) {

			Vec3 vecOffset = m_OwnerPosOverride - spine3WorldPos;
			pOwner->SetPosition(m_OwnerPosOverride + vecOffset);
		}

		m_CurRotationAngle += m_RotationSpeed * dt;
		Quat4 rot;
		rot.from_axis_angle(m_RotationAxis, m_CurRotationAngle);
		rot = m_OwnerStartRotation * rot;
		this->m_pActorComponent->SetOwnerRotation(rot);
	}

	virtual void UpdateState_Internal() override {
		kbGameEntity* const pOwner = this->m_pActorComponent->GetOwner();
		const float curTime = g_GlobalTimer.TimeElapsedSeconds();
		const float elapsedDeathTime = curTime - m_DeathStartTime;

		const float dt = g_pGame->GetFrameDT();
		if (m_DeathSelection == 0 || m_DeathSelection == 2) {
			UpdateFlyingDeath(dt);
		}

		if (m_DeathSelection == 0) {
			if (this->GetSnolaf()->owner_position().y < -60.0f && m_bSpawnedSplash == false) {
				m_bSpawnedSplash = true;
				this->GetSnolaf()->SpawnSplash();
			}
		}
	}

	virtual void EndState_Internal(T) override { }

private:

	const Vec3 m_MinLinearVelocity = Vec3(-0.015f, 0.015f, 0.03f);
	const Vec3 m_MaxLinearVelocity = Vec3(0.015f, 0.025f, 0.02f);
	const float m_MinAngularVelocity = 10.0f;
	const float m_MaxAngularVelocity = 15.0f;
	const Vec3 m_Gravity = Vec3(0.0f, -20.0f, 0.0f);

	Vec3 m_OwnerStartPos = Vec3::zero;
	Quat4 m_OwnerStartRotation = Quat4(0.0f, 0.0f, 0.0f, 1.0f);
	Vec3 m_OwnerPosOverride = Vec3::zero;

	Vec3 m_Velocity = Vec3::zero;
	Vec3 m_RotationAxis = Vec3(1.0f, 0.0f, 0.0f);

	float m_CurRotationAngle = 0.0f;
	float m_RotationSpeed = 1.0f;
	int m_DeathSelection = 0;
	float m_DeathStartTime = 0.0f;

	bool m_bSpawnedSplash = false;
};

/// KungFuSnolafStatePoofDeath
template<typename T>
class KungFuSnolafStatePoofDeath : public KungFuSnolafStateBase<T> {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStatePoofDeath(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		this->GetSnolaf()->EnableSmallLoveHearts(false);
		this->GetSnolaf()->EnableLargeLoveHearts(false);

		this->GetSnolaf()->DoPoofDeath();
	}
};

/// KungFuSnolafStateRunAway
template<typename T>
class KungFuSnolafStateRunAway : public KungFuSnolafStateBase<T> {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateRunAway(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }

	virtual void BeginState_Internal(T) override {

		static const kbString Run_Anim("Run");
		this->m_pActorComponent->PlayAnimation(Run_Anim, 0.05f);

		Vec3 snolafFacingDir = this->GetSnolaf()->GetTargetFacingDirection();
		snolafFacingDir *= -1.0f;
		this->GetSnolaf()->SetTargetFacingDirection(snolafFacingDir);

		this->GetSnolaf()->EnableSmallLoveHearts(false);
		this->GetSnolaf()->EnableLargeLoveHearts(false);
	}

	virtual void UpdateState_Internal() override {

		const float frameDT = g_pGame->GetFrameDT();

		const Vec3 moveDir = this->GetSnolaf()->GetTargetFacingDirection();
		const Vec3 newSnolafPos = this->m_pActorComponent->owner_position() - moveDir * frameDT * this->m_pActorComponent->GetMaxRunSpeed();
		this->m_pActorComponent->SetOwnerPosition(newSnolafPos);
	}
};

/// KungFuSnolafStateCinema
template<typename T>
class KungFuSnolafStateCinema : public KungFuSnolafStateBase<T> {

	//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateCinema(CannonActorComponent* const pPlayerComponent) : KungFuSnolafStateBase<T>(pPlayerComponent) { }
};

/// KungFuSnolafComponent::Constructor
void KungFuSnolafComponent::Constructor() {
	m_pSmallLoveHearts = nullptr;
	m_pLargeLoveHearts = nullptr;
}

/// KungFuSnolafComponent::enable_internal
void KungFuSnolafComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	m_pSmallLoveHearts = nullptr;
	if (bEnable) {

		// TODO: NEEDED?
		if (m_SkelModelsList.size() > 1) {
			const static kbString smearParam("smearParams");
			m_SkelModelsList[1]->set_material_param_vec4(0, smearParam.stl_str(), Vec4::zero);
		}

		static const kbString SmallLoveHearts("Small_LoveHearts");
		static const kbString LargeLoveHearts("Large_LoveHearts");

		for (int i = 0; i < GetOwner()->NumComponents(); i++) {
			if (GetOwner()->GetComponent(i)->IsA(kbParticleComponent::GetType()) == false) {
				continue;
			}

			kbParticleComponent* const pParticle = (kbParticleComponent*)GetOwner()->GetComponent(i);

			if (pParticle->GetName() == SmallLoveHearts.stl_str()) {
				m_pSmallLoveHearts = pParticle;
				m_pSmallLoveHearts->EnableNewSpawns(false);
			} else if (pParticle->GetName() == LargeLoveHearts.stl_str()) {
				m_pLargeLoveHearts = pParticle;
				m_pLargeLoveHearts->EnableNewSpawns(false);
			}
		}

		KungFuSnolafStateBase<KungFuSnolafState::SnolafState_t>* snolafStates[] = {
			new KungFuSnolafStateIdle<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateRun<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafeStatePrehug<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateHug<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateDead<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStatePoofDeath<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateWatchCannonBall<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateRunAway<KungFuSnolafState::SnolafState_t>(this),
			new KungFuSnolafStateCinema<KungFuSnolafState::SnolafState_t>(this)
		};

		InitializeStateMachine(snolafStates);
		RequestStateChange(KungFuSnolafState::Idle);
	} else {

		ShutdownStateMachine();
	}
}

/**
*	KungFuSnolafComponent::OnAnimEvent
*/
void KungFuSnolafComponent::OnAnimEvent(const kbAnimEventInfo_t& animEventInfo) {

	static const kbString LeftFootStep("Step_LeftFoot");
	static const kbString RightFootStep("Step_RightFoot");
	static const kbString LeftFootBone("L_Foot");
	static const kbString RightFootBone("R_Foot");

	const kbAnimEvent& animEvent = animEventInfo.m_AnimEvent;
	if (animEvent.GetEventName() == LeftFootStep || animEvent.GetEventName() == RightFootStep) {
		if (m_FootStepImpactFX.GetEntity() != nullptr) {
			kbGameEntity* const pFootStepFX = g_pGame->CreateEntity(m_FootStepImpactFX.GetEntity());

			const kbString footBone = (animEvent.GetEventName() == LeftFootStep) ? (LeftFootBone) : (RightFootBone);
			Vec3 decalPosition = Vec3::zero;

			//const kbString boneName
			m_SkelModelsList[0]->GetBoneWorldPosition(footBone, decalPosition);

			pFootStepFX->SetPosition(decalPosition);
			pFootStepFX->DeleteWhenComponentsAreInactive(true);
		}
	}
}

/// KungFuSnolafComponent::update_internal
void KungFuSnolafComponent::update_internal(const float DT) {
	Super::update_internal(DT);

	if (DT > 0.0f) {
		UpdateStateMachine();
	}

	if (IsDead()) {
		if (m_DeathTimer < 0.0f) {
			m_DeathTimer = 0.0f;
		}

		m_DeathTimer += DT;
		if (m_DeathTimer > 3.0f) {
			g_pCannonGame->GetLevelComponent<KungFuLevelComponent>()->ReturnSnolafToPool(this);
		}
	}
	Vec4 fxDot(1.0f, 0.0f, 0.0f, 0.0f);
	if (m_CurrentState == KungFuSnolafState::Hug) {
		fxDot.set(0.0f, 1.0f, 0.0f, 0.0f);
	} else if (m_CurrentState == KungFuSnolafState::Dead) {
		kbDeleteEntityComponent* const pDeleteComp = GetComponent<kbDeleteEntityComponent>();
		if (pDeleteComp == nullptr) {
			fxDot.set(0.0f, 0.0f, 1.0f, 0.0f);
		}
	}

	if (m_SkelModelsList.size() > 0) {
		const static kbString fxMapMaskParam("fxMapMask");
		if (m_OverridenFXMaskParams.r >= 0.0f && m_OverridenFXMaskParams.g >= 0.0f && m_OverridenFXMaskParams.b >= 0.0f && m_OverridenFXMaskParams.a >= 0.0f) {
			m_SkelModelsList[0]->set_material_param_vec4(0, fxMapMaskParam.stl_str(), m_OverridenFXMaskParams);
		} else {
			m_SkelModelsList[0]->set_material_param_vec4(0, fxMapMaskParam.stl_str(), fxDot);
		}
	}
}

/// KungFuSnolafComponent::ResetFromPool
void KungFuSnolafComponent::ResetFromPool() {

	m_AnimSmearDuration = 0.0f;
	m_AnimSmearVec = Vec3::zero;
	m_AnimSmearStartTime = -1.0f;
	m_DeathTimer = -1.0f;

	for (int i = 0; i < GetOwner()->NumComponents(); i++) {
		kbFlingPhysicsComponent* const pFling = GetOwner()->GetComponent(i)->GetAs<kbFlingPhysicsComponent>();
		if (pFling != nullptr) {
			pFling->Enable(false);
			continue;
		}

		GetOwner()->GetComponent(i)->Enable(false);
		GetOwner()->GetComponent(i)->Enable(true);
	}

	const static kbString clipMapMaskParam("clipMapMask");
	m_SkelModelsList[0]->set_material_param_vec4(0, clipMapMaskParam.stl_str(), Vec4(0.0f, 0.0f, 0.0f, 0.0f));
	m_SkelModelsList[1]->set_material_param_vec4(0, clipMapMaskParam.stl_str(), Vec4(0.0f, 0.0f, 0.0f, 0.0f));

	const static kbString smearParam("smearParams");
	m_SkelModelsList[1]->set_material_param_vec4(0, smearParam.stl_str(), Vec4::zero);

	m_Health = 1.0f;
}

/// KungFuSnolafComponent::EnableSmallLoveHearts
void KungFuSnolafComponent::EnableSmallLoveHearts(const bool bEnable) {
	if (m_pSmallLoveHearts == nullptr) {
		return;
	}

	m_pSmallLoveHearts->EnableNewSpawns(bEnable);
}

/// KungFuSnolafComponent::EnableLargeLoveHearts
void KungFuSnolafComponent::EnableLargeLoveHearts(const bool bEnable) {
	if (m_pLargeLoveHearts == nullptr) {
		return;
	}

	m_pLargeLoveHearts->EnableNewSpawns(bEnable);
}

/// KungFuSnolafComponent::TakeDamage
void KungFuSnolafComponent::TakeDamage(const DealAttackInfo_t<KungFuGame::eAttackType>& attackInfo) {

	m_LastAttackInfo = attackInfo;
	if (attackInfo.m_AttackType == KungFuGame::Shake) {

		const Vec3 attackerPos = attackInfo.m_pAttacker->owner_position();
		const Vec3 ourPos = owner_position();
		if (m_CurrentState == KungFuSnolafState::Hug || m_CurrentState == KungFuSnolafState::Prehug ||
			(attackerPos - ourPos).length() < KungFuGame::kShakeNBakeRadius) {
			m_Health = -1.0f;
			this->RequestStateChange(KungFuSnolafState::Dead);
		}
		return;
	}
	m_Health = -1.0f;
	this->RequestStateChange(KungFuSnolafState::Dead);
}

/// KungFuSnolafComponent::DoPoofDeath
void KungFuSnolafComponent::DoPoofDeath() {

	if (m_PoofDeathFX.GetEntity() == nullptr) {
		return;
	}

	kbGameEntity* const pCannonBallImpact = g_pGame->CreateEntity(m_PoofDeathFX.GetEntity());
	pCannonBallImpact->SetPosition(owner_position());
	pCannonBallImpact->SetOrientation(owner_rotation());
	pCannonBallImpact->DeleteWhenComponentsAreInactive(true);

	m_SkelModelsList[0]->Enable(false);
	m_SkelModelsList[1]->Enable(false);
}

/// KungFuSnolafComponent::SpawnAndFlingDecapHead
void KungFuSnolafComponent::SpawnAndFlingDecapHead() {

	if (m_DecapitatedHead.GetEntity() == nullptr) {
		return;
	}

	kbGameEntity* const pDecapHead = g_pGame->CreateEntity(m_DecapitatedHead.GetEntity());
	const Vec3 headPos = owner_position() + Vec3(0.0f, 1.75f, 0.0f);
	pDecapHead->SetPosition(headPos);
	pDecapHead->SetOrientation(owner_rotation());
	pDecapHead->DeleteWhenComponentsAreInactive(true);

	kbFlingPhysicsComponent* const pFlingComp = pDecapHead->GetComponent<kbFlingPhysicsComponent>();
	if (pFlingComp != nullptr) {
		pFlingComp->Enable(false);
		pFlingComp->Enable(true);
	}
}

/// KungFuSnolafComponent::SpawnAndFlingTopAndBottomHalf
void KungFuSnolafComponent::SpawnAndFlingTopAndBottomHalf() {

	m_SkelModelsList[0]->Enable(false);
	m_SkelModelsList[1]->Enable(false);

	if (m_TopHalfOfBody.GetEntity() == nullptr || m_BottomHalfOfBody.GetEntity() == nullptr) {
		return;
	}

	kbGameEntity* const pTopHalf = g_pGame->CreateEntity(m_TopHalfOfBody.GetEntity());
	const Vec3 topPos = owner_position() + Vec3(0.0f, 1.0f, 0.0f);
	pTopHalf->SetPosition(topPos);
	pTopHalf->SetOrientation(owner_rotation());
	pTopHalf->DeleteWhenComponentsAreInactive(true);

	kbFlingPhysicsComponent* const pTopFlingComp = pTopHalf->GetComponent<kbFlingPhysicsComponent>();
	if (pTopFlingComp != nullptr) {
		pTopFlingComp->Enable(false);
		pTopFlingComp->Enable(true);
	}

	kbGameEntity* const pBottomHalf = g_pGame->CreateEntity(m_BottomHalfOfBody.GetEntity());
	const Vec3 bottomPos = owner_position() + Vec3(0.0f, 0.2f, 0.0f);
	pBottomHalf->SetPosition(bottomPos);
	pBottomHalf->SetOrientation(owner_rotation());
	pBottomHalf->DeleteWhenComponentsAreInactive(true);

	kbFlingPhysicsComponent* const pBottomFlingComp = pBottomHalf->GetComponent<kbFlingPhysicsComponent>();
	if (pBottomFlingComp != nullptr) {
		pBottomFlingComp->Enable(false);
		pBottomFlingComp->Enable(true);
	}
}

/// KungFuSnolafComponent::SpawnSplash
void KungFuSnolafComponent::SpawnSplash() {

	if (m_SplashFX.GetEntity() == nullptr) {
		return;
	}

	kbGameEntity* const pSplash = g_pGame->CreateEntity(m_SplashFX.GetEntity());
	pSplash->SetPosition(owner_position());
	pSplash->DeleteWhenComponentsAreInactive(true);

	KungFuLevelComponent* const pLevelComponent = g_pCannonGame->GetLevelComponent<KungFuLevelComponent>();
	pLevelComponent->DoSplashSound();

	if (kbfrand() > 0.75f) {
		pLevelComponent->DoWaterDropletScreenFX();
	}
}