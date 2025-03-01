/// render_component.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "render_component.h"
#include "kbModel.h"

class kbAnimation;

/// RenderComponent
class kbStaticModelComponent : public RenderComponent {
	KB_DECLARE_COMPONENT(kbStaticModelComponent, RenderComponent);

public:
	virtual ~kbStaticModelComponent();

	void set_model(class kbModel* pModel) { m_model = pModel; }
	const kbModel* model() const { return m_model; }

	virtual void editor_change(const std::string& propertyName);

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

private:
	class kbModel* m_model;
};

/// kbAnimComponent
class kbAnimComponent : public kbGameComponent {
	friend class SkeletalModelComponent;

	KB_DECLARE_COMPONENT(kbAnimComponent, kbGameComponent);

public:
	const kbString& animation_name() const { return m_animation_name; }

private:
	kbString m_animation_name;
	kbAnimation* m_animation;
	float m_time_scale;
	bool m_is_looping;
	std::vector<kbAnimEvent> m_anim_events;

	float m_current_animation_time;
	kbString m_desired_next_animation;
	float m_desired_next_anim_blend_length;
};


/// SkeletalModelComponent
class SkeletalModelComponent : public RenderComponent {
	KB_DECLARE_COMPONENT(SkeletalModelComponent, RenderComponent);

public:
	virtual	~SkeletalModelComponent();

	void set_model(class kbModel* const pModel);
	const kbModel* model() const { return m_model; }

	virtual void editor_change(const std::string& propertyName);

	int GetBoneIndex(const kbString& boneName);
	kbBoneMatrix_t GetBoneRefMatrix(const int index);

	bool GetBoneWorldPosition(const kbString& boneName, Vec3& outWorldPosition);
	bool GetBoneWorldMatrix(const kbString& boneName, kbBoneMatrix_t& boneMatrix);
	std::vector<kbBoneMatrix_t>& GetFinalBoneMatrices() { return m_BindToLocalSpaceMatrices; }
	const std::vector<kbBoneMatrix_t>& GetFinalBoneMatrices() const { return m_BindToLocalSpaceMatrices; }

	void										SetAnimationTimeScaleMultiplier(const kbString& animationName, const float factor);

	// Animation
	void PlayAnimation(const kbString& AnimationName, const float BlendLength, bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation = kbString::EmptyString, const float desiredNextAnimBlendLength = 0.0f);
	bool IsPlaying(const kbString& AnimationName) const;

	bool HasFinishedAnimation() const { return IsTransitioningAnimations() == false && (m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_animation == NULL || m_Animations[m_CurrentAnimation].m_current_animation_time >= m_Animations[m_CurrentAnimation].m_animation->GetLengthInSeconds()); }
	bool IsTransitioningAnimations() const { return m_CurrentAnimation != -1 && m_NextAnimation != -1; }

	float GetCurAnimTimeSeconds() const { if (m_CurrentAnimation == -1) return -1.0f; return m_Animations[m_CurrentAnimation].m_current_animation_time; }
	float GetNormalizedAnimTime() const { return GetCurAnimTimeSeconds() / GetCurAnimLengthSeconds(); }
	float GetCurAnimLengthSeconds() const { if (m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_animation == NULL) return -1.0f; return m_Animations[m_CurrentAnimation].m_animation->GetLengthInSeconds(); }

	const kbString* GetCurAnimationName() const;
	const kbString* GetNextAnimationName() const;

	void RegisterAnimEventListener(IAnimEventListener* const pListener);
	void UnregisterAnimEventListener(IAnimEventListener* const pListener);

	void RegisterSyncSkelModel(SkeletalModelComponent* const pSkelModel);
	void UnregisterSyncSkelModel(SkeletalModelComponent* const pSkelModel);

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

	std::vector<IAnimEventListener*> m_AnimEventListeners;

	// Editor
	class kbModel* m_model;
	std::vector<kbAnimComponent> m_Animations;

	// Game
	std::vector<kbBoneMatrix_t>	m_BindToLocalSpaceMatrices;

	int	m_CurrentAnimation;
	int	m_NextAnimation;
	float m_BlendStartTime;
	float m_BlendLength;

	std::vector<float>	m_AnimationTimeScaleMultipliers;

	std::vector<SkeletalModelComponent*>	m_SyncedSkelModels;
	SkeletalModelComponent* m_pSyncParent;

	// Debug
	int	m_DebugAnimIdx;
	float m_DebugAnimTime;
};

/// kbFlingPhysicsComponent
class kbFlingPhysicsComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbFlingPhysicsComponent, kbGameComponent);

public:
	void ResetToStartPos() { if (m_bOwnerStartSet) { SetOwnerPosition(m_OwnerStartPos); SetOwnerRotation(m_OwnerStartRotation); } }

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

private:
	// Editor
	Vec3 m_min_linear_vel;
	Vec3 m_max_linear_vel;
	float m_MinAngularSpeed;
	float m_MaxAngularSpeed;
	Vec3 m_gravity;

	// Run time
	Vec3 m_OwnerStartPos;
	Quat4 m_OwnerStartRotation;

	Vec3 m_velocity;
	Vec3 m_rotation_axis;

	float m_cur_rotation_angle;
	float m_rotation_speed;

	float m_FlingStartTime;

	bool m_bOwnerStartSet;
};
