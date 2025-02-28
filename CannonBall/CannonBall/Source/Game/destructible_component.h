/// EtherSkelModel.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "kbComponent.h"
#include "model_component.h"

/// EtherAnimComponent
class EtherAnimComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(EtherAnimComponent, kbGameComponent);

	friend class EtherSkelModelComponent;

public:
	const kbString& GetAnimationName() const { return m_AnimationName; }

private:
	kbString m_AnimationName;
	kbAnimation* m_pAnimation;
	f32 m_TimeScale;
	bool m_bIsLooping;
	std::vector<kbAnimEvent> m_AnimEvents;

	f32 m_CurrentAnimationTime;
	kbString m_DesiredNextAnimation;
	f32	m_DesiredNextAnimBlendLength;
};

/// EtherSkelModelComponent
class EtherSkelModelComponent : public SkeletalModelComponent {
	KB_DECLARE_COMPONENT(EtherSkelModelComponent, SkeletalModelComponent);

public:
	void PlayAnimation(const kbString& AnimationName, const float BlendLength, bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation = kbString::EmptyString, const float desiredNextAnimBlendLength = 0.0f);
	bool IsPlaying(const kbString& AnimationName) const;

	bool IsFirstPersonModel() const { return m_bFirstPersonModel; }

	bool HasFinishedAnimation() const { return IsTransitioningAnimations() == false && (m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL || m_Animations[m_CurrentAnimation].m_CurrentAnimationTime >= m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds()); }
	bool IsTransitioningAnimations() const { return m_CurrentAnimation != -1 && m_NextAnimation != -1; }

	f32	GetCurAnimTimeSeconds() const { if (m_CurrentAnimation == -1) return -1.0f; return m_Animations[m_CurrentAnimation].m_CurrentAnimationTime; }
	f32	GetNormalizedAnimTime() const { return GetCurAnimTimeSeconds() / GetCurAnimLengthSeconds(); }
	f32	GetCurAnimLengthSeconds() const { if (m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL) return -1.0f; return m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds(); }

	const kbString* GetCurAnimationName() const;

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

	std::vector<EtherAnimComponent>	m_Animations;
	i32	m_DebugAnimIdx;

	i32	m_CurrentAnimation;
	i32	m_NextAnimation;
	f32	m_BlendStartTime;
	f32	m_BlendLength;

	bool m_bFirstPersonModel;
};

enum EDestructibleBehavior {
	PushFromImpactPoint,
	UserVelocity,
};

/// EtherDestructibleComponent
class EtherDestructibleComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(EtherDestructibleComponent, kbGameComponent);

public:
	virtual void editor_change(const std::string& propertyName) override;

	void TakeDamage(const float damageAmt, const Vec3& damagePosition, const float damageRadius);

	bool IsSimulating() const { return m_bIsSimulating; }

	struct destructibleBone_t {
		Vec3 m_Position;
		Vec3 m_Acceleration;
		Vec3 m_Velocity;

		Vec3 m_RotationAxis;
		f32	m_RotationSpeed;
		f32	m_CurRotationAngle;
	};
	const std::vector<destructibleBone_t>& GetBonesList() const { return m_BonesList; }

private:
	void enable_internal(const bool bEnable) override;
	void update_internal(const float deltaTime) override;

	// Editor
	EDestructibleBehavior m_DestructibleType;
	f32 m_MaxLifeTime;
	Vec3 m_Gravity;
	Vec3 m_MinLinearVelocity;
	Vec3 m_MaxLinearVelocity;
	f32 m_MinAngularVelocity;
	f32 m_MaxAngularVelocity;
	f32 m_StartingHealth;

	kbGameEntityPtr	m_CompleteDestructionFX;
	Vec3 m_DestructionFXLocalOffset;

	bool m_bDebugResetSim;

	// Run time
	std::vector<destructibleBone_t>	m_BonesList;

	f32 m_Health;
	const EtherSkelModelComponent* m_pSkelModel;
	f32	m_SimStartTime;
	Vec3 m_LastHitLocation;
	bool m_bIsSimulating;
};
