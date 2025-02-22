/// model_component.cpp
/// model_component.cpp
///
/// 2016-2025 blk 1.0

#include "blk_containers.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"
#include "renderer.h"
#include "dx11/kbRenderer_DX11.h"

KB_DEFINE_COMPONENT(kbStaticModelComponent)

/// RenderComponent
void kbStaticModelComponent::Constructor() {
	m_model = nullptr;
}

/// ~kbStaticModelComponent
kbStaticModelComponent::~kbStaticModelComponent() {
}

/// kbStaticModelComponent::EditorChange
void kbStaticModelComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (IsEnabled() && (propertyName == "Model" || propertyName == "ShaderOverride")) {
		enable_internal(false);
		enable_internal(true);
	}
}

/// kbStaticModelComponent::enable_internal
void kbStaticModelComponent::enable_internal(const bool isEnabled) {

	Super::enable_internal(isEnabled);

	if (m_model == nullptr) {
		return;
	}

	m_render_object.m_pComponent = this;

	if (isEnabled) {

		m_render_object.m_casts_shadow = this->GetCastsShadow();
		m_render_object.m_bIsSkinnedModel = false;
		m_render_object.m_EntityId = GetOwner()->GetEntityId();
		m_render_object.m_Orientation = GetOwner()->GetOrientation();
		m_render_object.m_model = m_model;
		m_render_object.m_Position = GetOwner()->GetPosition();
		m_render_object.m_render_pass = m_render_pass;
		m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		m_render_object.m_render_order_bias = m_render_order_bias;

		refresh_materials(false);

		g_pRenderer->AddRenderObject(m_render_object);

		if (g_renderer) {
			g_renderer->add_render_component(this);
		}
	} else {
		if (g_pRenderer) {
			g_pRenderer->RemoveRenderObject(m_render_object);
		}

		if (g_renderer) {
			g_renderer->remove_render_component(this);
		}
	}
}

/// kbStaticModelComponent::update_internal
void kbStaticModelComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (m_model != nullptr && GetOwner()->IsDirty()) {
		m_render_object.m_Position = GetOwner()->GetPosition();
		m_render_object.m_Orientation = GetOwner()->GetOrientation();
		m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		m_render_object.m_model = m_model;

		g_pRenderer->UpdateRenderObject(m_render_object);
	}

	// m_model->DrawDebugTBN( GetOwner()->GetPosition(), GetOwner()->GetOrientation(), GetOwner()->GetScale() );
}

KB_DEFINE_COMPONENT(kbSkeletalRenderComponent)

/// kbAnimComponent::Constructor
void kbAnimComponent::Constructor() {
	m_animation = nullptr;
	m_time_scale = 1.0f;
	m_is_looping = false;
	m_current_animation_time = -1.0f;
}

/// kbSkeletalRenderComponent::Constructor
void kbSkeletalRenderComponent::Constructor() {
	m_model = nullptr;
	m_render_object.m_bIsSkinnedModel = true;

	m_CurrentAnimation = -1;
	m_NextAnimation = -1;

	m_pSyncParent = nullptr;

	m_BlendStartTime = 0.0f;
	m_BlendLength = 1.0f;

	m_DebugAnimIdx = -1;
	m_DebugAnimTime = 0.0f;
}

/// kbSkeletalRenderComponent::~kbSkeletalRenderComponent
kbSkeletalRenderComponent::~kbSkeletalRenderComponent() {}

/// kbSkeletalRenderComponent::EditorChange
void kbSkeletalRenderComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "Model" || propertyName == "ShaderOverride") {
		refresh_materials(true);
	}
}

/// kbSkeletalRenderComponent::enable_internal
void kbSkeletalRenderComponent::enable_internal(const bool isEnabled) {
	if (m_model == nullptr || g_pRenderer == nullptr) {
		return;
	}

	m_render_object.m_pComponent = this;
	if (isEnabled) {
		m_render_object.m_casts_shadow = this->GetCastsShadow();
		m_render_object.m_EntityId = GetOwner()->GetEntityId();
		m_render_object.m_Orientation = GetOwner()->GetOrientation();
		m_render_object.m_model = m_model;
		m_render_object.m_Position = GetOwner()->GetPosition();
		m_render_object.m_render_pass = m_render_pass;
		m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		refresh_materials(false);

		g_pRenderer->AddRenderObject(m_render_object);

		m_AnimationTimeScaleMultipliers.resize(m_Animations.size());
		for (int i = 0; i < m_AnimationTimeScaleMultipliers.size(); i++) {
			m_AnimationTimeScaleMultipliers[i] = 1.0f;
		}
	} else {
		g_pRenderer->RemoveRenderObject(m_render_object);
	}
}

/// kbSkeletalRenderComponent::update_internal
void kbSkeletalRenderComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (m_model != nullptr && m_pSyncParent == nullptr) {
		if (m_BindToLocalSpaceMatrices.size() != m_model->NumBones()) {
			m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
		}

		// Debug Animation
		if (m_DebugAnimIdx >= 0 && m_DebugAnimIdx < m_Animations.size() && m_Animations[m_DebugAnimIdx].m_animation != nullptr) {
			if (m_model != nullptr) {

				static bool pause = false;
				if (pause == false) {
					const float AnimTimeScale = m_Animations[m_DebugAnimIdx].m_time_scale;
					m_DebugAnimTime += DeltaTime * AnimTimeScale;

					if (m_Animations[m_DebugAnimIdx].m_is_looping == false) {
						m_DebugAnimTime = kbClamp(m_DebugAnimTime, 0.0f, m_Animations[m_DebugAnimIdx].m_animation->GetLengthInSeconds());
					}
				}

				if (m_BindToLocalSpaceMatrices.size() == 0) {
					m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
				}
				m_model->Animate(m_BindToLocalSpaceMatrices, m_DebugAnimTime, m_Animations[m_DebugAnimIdx].m_animation, m_Animations[m_DebugAnimIdx].m_is_looping);
			}
		} else {
			for (int i = 0; i < m_model->NumBones(); i++) {
				m_BindToLocalSpaceMatrices[i].SetIdentity();
			}
		}

		if (m_CurrentAnimation != -1) {
#if DEBUG_ANIMS
			bool bOutput = true;
			if (m_Animations[m_CurrentAnimation].animation_name().stl_str().find("Shoot") != std::string::npos) {
				bOutput = true;
			}
			if (bOutput) blk::log("Updating current anim %s with idx %d", m_Animations[m_CurrentAnimation].animation_name().c_str(), m_CurrentAnimation);
#endif
			// Check if the blend is finished
			if (m_NextAnimation != -1) {
				const float blendTime = (g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime) / m_BlendLength;

#if DEBUG_ANIMS
				if (bOutput) blk::log("	Checking if blend is finished.  Blend time is %f", blendTime);
#endif
				if (blendTime >= 1.0f) {
					m_CurrentAnimation = m_NextAnimation;
					m_NextAnimation = -1;

#if DEBUG_ANIMS
					if (bOutput) blk::log("	%s Transition to Next Animation", GetOwner()->GetName().c_str());
#endif
				}
			}

			kbAnimComponent& CurAnim = m_Animations[m_CurrentAnimation];

			bool bAnimIsFinished = false;
			const float curAnimLenSec = CurAnim.m_animation->GetLengthInSeconds();
			const float prevAnimTime = CurAnim.m_current_animation_time;

			bool bOutput = false;
			if (CurAnim.m_is_looping && CurAnim.m_anim_events.size() > 0) {
				bOutput = true;
			}

			if (CurAnim.m_is_looping == false) {
				if (CurAnim.m_current_animation_time >= curAnimLenSec) {

#if DEBUG_ANIMS
					if (bOutput) blk::log("	Cur anim is finished!");
#endif
					//CurAnim.m_current_animation_time = curAnimLenSec;
					bAnimIsFinished = true;
				}
			}

			if (m_NextAnimation == -1) {

				CurAnim.m_current_animation_time += DeltaTime * CurAnim.m_time_scale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];

				if (CurAnim.m_is_looping) {
					CurAnim.m_current_animation_time = fmod(CurAnim.m_current_animation_time, curAnimLenSec);
				}

				const float curAnimTime = CurAnim.m_current_animation_time;
#if DEBUG_ANIMS
				if (bOutput) { blk::log("		prevAnimTime = %f - Cur anim time = %f.  DeltaT and all that was %f", prevAnimTime, CurAnim.m_current_animation_time); }
#endif

				for (int iAnimEvent = 0; iAnimEvent < CurAnim.m_anim_events.size(); iAnimEvent++) {
					auto& curEvent = CurAnim.m_anim_events[iAnimEvent];
					const float animEventTime = curEvent.GetEventTime();

					if ((animEventTime > prevAnimTime && animEventTime <= curAnimTime) ||
						(prevAnimTime > curAnimTime && animEventTime < curAnimTime)) {

						if ((prevAnimTime > curAnimTime && animEventTime < curAnimTime)) {
							//	blk::log( "	pat = %f curAnimTime = %f animEventTime = %f --> %s. DT = %f", prevAnimTime, curAnimTime, animEventTime, curEvent.GetEventName().c_str(), DeltaTIme );
						}
						const kbAnimEventInfo_t animEventInfo(curEvent, this);
						for (int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++) {
							IAnimEventListener* const pCurListener = m_AnimEventListeners[iListener];
							m_AnimEventListeners[iListener]->OnAnimEvent(animEventInfo);
						}
					}
				}

#if DEBUG_ANIMS
				if (bOutput) blk::log("	Not blending anim %s. anim time = %f", CurAnim.m_animation_name.c_str(), CurAnim.m_current_animation_time);
#endif

				m_model->Animate(m_BindToLocalSpaceMatrices, CurAnim.m_current_animation_time, CurAnim.m_animation, CurAnim.m_is_looping);

				if (bAnimIsFinished && CurAnim.m_desired_next_animation.IsEmptyString() == false) {

#if DEBUG_ANIMS
					if (bOutput) blk::log("	Cur Animation Done, going to %s - %f", CurAnim.m_desired_next_animation.c_str(), CurAnim.m_desired_next_anim_blend_length);
#endif

					PlayAnimation(CurAnim.m_desired_next_animation, CurAnim.m_desired_next_anim_blend_length, true);
				}
			} else {

				if (bAnimIsFinished == false) {
					const float prevAnimTime = fmod(CurAnim.m_current_animation_time, curAnimLenSec);

					CurAnim.m_current_animation_time += DeltaTime * CurAnim.m_time_scale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];
					if (CurAnim.m_is_looping) {
						CurAnim.m_current_animation_time = fmod(CurAnim.m_current_animation_time, curAnimLenSec);
					}

					for (int iAnimEvent = 0; iAnimEvent < CurAnim.m_anim_events.size(); iAnimEvent++) {
						auto& curEvent = CurAnim.m_anim_events[iAnimEvent];
						const float animEventTime = curEvent.GetEventTime() * CurAnim.m_time_scale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];

						if ((animEventTime > prevAnimTime && animEventTime <= CurAnim.m_current_animation_time) ||
							(prevAnimTime > CurAnim.m_current_animation_time && animEventTime < CurAnim.m_current_animation_time)) {

							const kbAnimEventInfo_t animEventInfo(curEvent, this);
							for (int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++) {
								IAnimEventListener* const pCurListener = m_AnimEventListeners[iListener];
								m_AnimEventListeners[iListener]->OnAnimEvent(animEventInfo);
							}
						}
					}
				}

				kbAnimComponent& NextAnim = m_Animations[m_NextAnimation];
				const float nextAnimLenSec = NextAnim.m_animation->GetLengthInSeconds();
				const float prevNextAnimTime = NextAnim.m_current_animation_time;

#if DEBUG_ANIMS
				if (bOutput) { blk::log("		Cur anim is %s.  time = %f.  DeltaT was %f.  Next anim is %s.  Next anim time is %f", CurAnim.m_animation_name.c_str(), CurAnim.m_current_animation_time, DeltaTime * CurAnim.m_time_scale, NextAnim.m_animation_name.c_str(), NextAnim.m_current_animation_time); }
#endif

				if (CurAnim.m_is_looping && NextAnim.m_is_looping) {
					// Sync the anims if they're both looping
					NextAnim.m_current_animation_time = CurAnim.m_current_animation_time;
				} else {
					NextAnim.m_current_animation_time += DeltaTime * NextAnim.m_time_scale * m_AnimationTimeScaleMultipliers[m_NextAnimation];
				}

				if (NextAnim.m_is_looping) {
					NextAnim.m_current_animation_time = fmod(NextAnim.m_current_animation_time, nextAnimLenSec);
				}

				for (int iAnimEvent = 0; iAnimEvent < NextAnim.m_anim_events.size(); iAnimEvent++) {
					auto& curEvent = NextAnim.m_anim_events[iAnimEvent];

					const float animEventTime = curEvent.GetEventTime() * m_AnimationTimeScaleMultipliers[m_NextAnimation];

					if ((animEventTime > prevNextAnimTime && animEventTime <= NextAnim.m_current_animation_time) ||
						(prevNextAnimTime > NextAnim.m_current_animation_time && animEventTime < NextAnim.m_current_animation_time)) {

						const kbAnimEventInfo_t animEventInfo(curEvent, this);
						for (int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++) {
							IAnimEventListener* const pCurListener = m_AnimEventListeners[iListener];
							m_AnimEventListeners[iListener]->OnAnimEvent(animEventInfo);
						}
					}
				}

				const float blendTime = kbClamp((g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime) / m_BlendLength, 0.0f, 1.0f);
				m_model->BlendAnimations(m_BindToLocalSpaceMatrices, CurAnim.m_animation, CurAnim.m_current_animation_time, CurAnim.m_is_looping, NextAnim.m_animation, NextAnim.m_current_animation_time, NextAnim.m_is_looping, blendTime);

#if DEBUG_ANIMS
				if (bOutput) blk::log("	Blending anims %f.  %s cur time = %f. %s cur time is %f", blendTime, CurAnim.animation_name().c_str(), CurAnim.m_current_animation_time, NextAnim.animation_name().c_str(), NextAnim.m_current_animation_time);
#endif
			}
		}
	}

	m_render_object.m_pComponent = this;
	m_render_object.m_Position = GetOwner()->GetPosition();
	m_render_object.m_Orientation = GetOwner()->GetOrientation();
	m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
	m_render_object.m_model = m_model;
	m_render_object.m_render_pass = m_render_pass;
	g_pRenderer->UpdateRenderObject(m_render_object);

	for (int i = 0; i < m_SyncedSkelModels.size(); i++) {
		m_SyncedSkelModels[i]->m_BindToLocalSpaceMatrices = m_BindToLocalSpaceMatrices;
	}
}

/// kbSkeletalRenderComponent::GetBoneIndex
int kbSkeletalRenderComponent::GetBoneIndex(const kbString& boneName) {
	if (m_model == nullptr) {
		return -1;
	}
	return m_model->GetBoneIndex(boneName);
}

/// kbSkeletalRenderComponent::GetBoneRefMatrix
kbBoneMatrix_t kbSkeletalRenderComponent::GetBoneRefMatrix(int index) {
	return m_model->GetRefBoneMatrix(index);
}

/// kbSkeletalRenderComponent::GetBoneWorldPosition
bool kbSkeletalRenderComponent::GetBoneWorldPosition(const kbString& boneName, Vec3& outWorldPosition) {
	const int boneIdx = GetBoneIndex(boneName);
	if (boneIdx == -1 || boneIdx >= m_BindToLocalSpaceMatrices.size()) {
		return false;
	}

	Mat4 worldMatrix;
	GetOwner()->CalculateWorldMatrix(worldMatrix);

	const Vec3 localPos = m_model->GetRefBoneMatrix(boneIdx).GetOrigin() * m_BindToLocalSpaceMatrices[boneIdx];
	outWorldPosition = worldMatrix.transform_point(localPos);
	return true;
}

/// kbSkeletalRenderComponent::GetBoneWorldMatrix
bool kbSkeletalRenderComponent::GetBoneWorldMatrix(const kbString& boneName, kbBoneMatrix_t& boneMatrix) {
	const int boneIdx = GetBoneIndex(boneName);
	if (boneIdx == -1 || boneIdx >= m_BindToLocalSpaceMatrices.size()) {
		return false;
	}

	Mat4 WeaponMatrix;
	GetOwner()->CalculateWorldMatrix(WeaponMatrix);

	boneMatrix = m_model->GetRefBoneMatrix(boneIdx) * m_BindToLocalSpaceMatrices[boneIdx];
	boneMatrix *= WeaponMatrix;
	return true;
}

/// kbSkeletalRenderComponent::SetAnimationTimeScaleMultiplier
void kbSkeletalRenderComponent::SetAnimationTimeScaleMultiplier(const kbString& animName, const float factor) {
	for (int i = 0; i < m_Animations.size(); i++) {
		const kbAnimComponent& anim = m_Animations[i];
		if (anim.m_animation_name == animName) {
			m_AnimationTimeScaleMultipliers[i] = factor;
			return;
		}
	}
}

/// kbSkeletalRenderComponent::PlayAnimation
void kbSkeletalRenderComponent::PlayAnimation(const kbString& AnimationName, const float BlendLength, const bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength) {
#if DEBUG_ANIMS
	bool bOutput = true;
	if (bOutput) blk::log("Attempting to play Animation %s ===================================================================", AnimationName.c_str());
#endif

	if (bRestartIfAlreadyPlaying == false && IsPlaying(AnimationName)) {
		if (m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_animation_name != AnimationName) {
			m_Animations[m_NextAnimation].m_current_animation_time = -1;
			m_NextAnimation = -1;
		}
#if DEBUG_ANIMS
		if (bOutput) blk::log("		Returning as it's already playing");
#endif
		return;
	}

	const std::string& animName = AnimationName.stl_str();
	for (int i = 0; i < m_Animations.size(); i++) {
		if (m_Animations[i].m_animation_name != AnimationName) {
			continue;
		}

#if DEBUG_ANIMS
		if (bOutput) blk::log("		Found desired animation");
#endif

		if (BlendLength <= 0.0f || m_CurrentAnimation == -1) {

#if DEBUG_ANIMS
			if (bOutput) blk::log("		Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation);
#endif

			// Stop previous animation
			if (m_CurrentAnimation != -1 && m_CurrentAnimation != i) {
#if DEBUG_ANIMS
				if (bOutput) blk::log("		Stopping Animation %s", m_Animations[m_CurrentAnimation].animation_name().c_str());
#endif

				m_Animations[m_CurrentAnimation].m_current_animation_time = -1;
			}

			if (m_NextAnimation != -1 && m_NextAnimation != i) {

#if DEBUG_ANIMS
				if (bOutput)blk::log("		Canceling next animation %s", m_Animations[m_NextAnimation].animation_name().c_str());
#endif

				m_Animations[m_NextAnimation].m_current_animation_time = -1;
			}
			m_NextAnimation = -1;

			// Start current animation
			if (m_CurrentAnimation != i) {
				m_Animations[i].m_current_animation_time = 0.0f;
			}
			m_CurrentAnimation = i;

#if DEBUG_ANIMS
			if (bOutput)blk::log("		Anim all set up.  Next anim = %s.  Desired blend length = %f.  Starting animation time is %f", desiredNextAnimation.c_str(), desiredNextAnimationBlendLength, m_Animations[i].m_current_animation_time);
#endif

			m_Animations[m_CurrentAnimation].m_desired_next_animation = desiredNextAnimation;
			m_Animations[m_CurrentAnimation].m_desired_next_anim_blend_length = desiredNextAnimationBlendLength;
		} else {


#if DEBUG_ANIMS
			if (bOutput) {
				if (m_CurrentAnimation != -1) {
					blk::log("		Cur anim is %s.  Anim time is %f", m_Animations[m_CurrentAnimation].animation_name().c_str(), m_Animations[m_CurrentAnimation].m_current_animation_time);
				}

			}// blk::log( "	Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation );
#endif

			m_BlendStartTime = g_GlobalTimer.TimeElapsedSeconds();
			m_BlendLength = BlendLength;
			m_NextAnimation = i;

			m_Animations[m_NextAnimation].m_desired_next_animation = desiredNextAnimation;
			m_Animations[m_NextAnimation].m_desired_next_anim_blend_length = desiredNextAnimationBlendLength;
			m_Animations[m_NextAnimation].m_current_animation_time = 0.0f;


#if DEBUG_ANIMS
			if (bOutput) blk::log("		Blend Len = %f.  Next Anim Idx = %d.  Next Anim Name = %s.  Next Anim Len = %f. ", m_BlendLength, m_NextAnimation, desiredNextAnimation.c_str(), desiredNextAnimationBlendLength);
#endif
		}

		break;
	}
}

/// kbSkeletalRenderComponent::PlayAnimation
bool kbSkeletalRenderComponent::IsPlaying(const kbString& AnimationName) const {
	if (m_Animations.size() == 0) {
		return false;
	}

	if (m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_animation_name == AnimationName) {
		return true;
	}

	if (m_CurrentAnimation != -1 && m_Animations[m_CurrentAnimation].m_animation_name == AnimationName) {

		const kbAnimComponent& anim = m_Animations[m_CurrentAnimation];
		if (anim.m_is_looping == true || anim.m_current_animation_time <= anim.m_animation->GetLengthInSeconds()) {
			return true;
		}
	}

	return false;
}

/// kbSkeletalRenderComponent::SetModel
void kbSkeletalRenderComponent::set_model(kbModel* const pModel) {
	m_BindToLocalSpaceMatrices.clear();
	m_render_object.m_model = pModel;
	g_pRenderer->UpdateRenderObject(m_render_object);
}

/// kbSkeletalRenderComponent::GetCurAnimationName
const kbString* kbSkeletalRenderComponent::GetCurAnimationName() const {
	if (m_CurrentAnimation >= 0 && m_CurrentAnimation < m_Animations.size()) {
		return &m_Animations[m_CurrentAnimation].animation_name();
	}

	return nullptr;
}

/// kbSkeletalRenderComponent::GetNextAnimationName
const kbString* kbSkeletalRenderComponent::GetNextAnimationName() const {
	if (m_NextAnimation >= 0 && m_NextAnimation < m_Animations.size()) {
		return &m_Animations[m_NextAnimation].animation_name();
	}

	return nullptr;
}

/// kbSkeletalRenderComponent::RegisterAnimEventListener
void kbSkeletalRenderComponent::RegisterAnimEventListener(IAnimEventListener* const pListener) {
	blk::error_check(blk::std_contains(m_AnimEventListeners, pListener) == false, "RegisterAnimEventListener() - Duplicate entries");
	m_AnimEventListeners.push_back(pListener);
}

/// kbSkeletalRenderComponent::UnregisterAnimEventListener
void kbSkeletalRenderComponent::UnregisterAnimEventListener(IAnimEventListener* const pListener) {
	blk::error_check(blk::std_contains(m_AnimEventListeners, pListener) == true, "UnregisterAnimEventListener() - Listener not previously registered");
	blk::std_remove_swap(m_AnimEventListeners, pListener);
}

/// kbSkeletalRenderComponent::RegisterSyncSkelModel
void kbSkeletalRenderComponent::RegisterSyncSkelModel(kbSkeletalRenderComponent* const pSkelModel) {
	if (blk::std_find(m_SyncedSkelModels, pSkelModel) != m_SyncedSkelModels.end()) {
		return;
	}
	m_SyncedSkelModels.push_back(pSkelModel);
	pSkelModel->m_pSyncParent = this;
}

/// kbSkeletalRenderComponent::UnregisterSyncSkelModel
void kbSkeletalRenderComponent::UnregisterSyncSkelModel(kbSkeletalRenderComponent* const pSkelModel) {
	blk::std_remove_swap(m_SyncedSkelModels, pSkelModel);
	pSkelModel->m_pSyncParent = nullptr;
}

/// kbFlingPhysicsComponent::Constructor
void kbFlingPhysicsComponent::Constructor() {
	// Editor
	m_MinLinearVelocity.set(-0.015f, 0.015f, 0.03f);
	m_MaxLinearVelocity.set(0.015f, 0.025f, 0.035f);
	m_MinAngularSpeed = 10.0f;
	m_MaxAngularSpeed = 15.0f;
	m_Gravity.set(0.0f, -20.0f, 0.0f);

	// Run time
	m_OwnerStartPos.set(0.0f, 0.0f, 0.0f);
	m_OwnerStartRotation.set(0.0f, 0.0f, 0.0f, 1.0f);

	m_Velocity.set(0.0f, 0.0f, 0.0f);
	m_RotationAxis.set(1.0f, 0.0f, 0.0f);

	m_CurRotationAngle = 0.0f;
	m_RotationSpeed = 1.0f;

	m_FlingStartTime = 0.0f;

	m_bOwnerStartSet = false;
}

/// kbFlingPhysicsComponent::enable_internal
void kbFlingPhysicsComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (bEnable) {
		m_OwnerStartPos = owner_position();
		m_OwnerStartRotation = owner_rotation();
		m_bOwnerStartSet = true;

		m_Velocity = Vec3Rand(m_MinLinearVelocity, m_MaxLinearVelocity);

		Mat4 worldMatrix;
		GetOwner()->CalculateWorldMatrix(worldMatrix);
		const XMMATRIX inverseMat = XMMatrixInverse(nullptr, XMMATRIXFromMat4(worldMatrix));
		worldMatrix = Mat4FromXMMATRIX(inverseMat);
		worldMatrix.transpose_self();
		m_Velocity = m_Velocity * worldMatrix;

		m_RotationAxis = Vec3(kbfrand(), kbfrand(), kbfrand());
		if (m_RotationAxis.length_sqr() < 0.01f) {
			m_RotationAxis.set(1.0f, 0.0f, 0.0f);
		} else {
			m_RotationAxis.normalize_self();
		}
		m_RotationSpeed = kbfrand(m_MinAngularSpeed, m_MaxAngularSpeed);
		m_CurRotationAngle = 0;

		m_FlingStartTime = g_GlobalTimer.TimeElapsedSeconds();
	} else {
		if (g_UseEditor) {
			ResetToStartPos();
		}
	}
}

/// kbFlingPhysicsComponent::update_internal
void kbFlingPhysicsComponent::update_internal(const float dt) {
	Super::update_internal(dt);

	const float curTime = g_GlobalTimer.TimeElapsedSeconds();
	const float elapsedDeathTime = curTime - m_FlingStartTime;

	Vec3 newPos = owner_position();
	newPos.x += m_Velocity.x * dt;
	newPos.y = m_OwnerStartPos.y + m_Velocity.y * elapsedDeathTime - (0.5f * -m_Gravity.y * elapsedDeathTime * elapsedDeathTime);
	newPos.z += m_Velocity.z * dt;
	SetOwnerPosition(newPos);

	m_CurRotationAngle += m_RotationSpeed * dt;
	Quat4 rot;
	rot.from_axis_angle(m_RotationAxis, m_CurRotationAngle);
	rot = m_OwnerStartRotation * rot;
	SetOwnerRotation(rot);
}