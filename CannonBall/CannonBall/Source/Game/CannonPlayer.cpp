/// CannonPlayer.cpp
///
// 2019-2025 blk 1.0

#include <windows.h>
#include <math.h>
#include "blk_core.h"
#include "kbGameEntityHeader.h"
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/// CannonActorComponent::Constructor
void CannonActorComponent::Constructor() {
	m_MaxRunSpeed = 3.0f;
	m_MaxRotateSpeed = 15.0f;
	m_health = 100.0f;

	m_TargetFacingDirection.set(0.0f, 0.0f, -1.0f);

	m_AnimSmearDuration = 0.1f;
	m_AnimSmearVec.set(0.0f, 0.0f, 0.0f, 0.0f);
	m_AnimSmearStartTime = -1.0f;

	m_LastVOTime = 0.0f;
	m_OverridenFXMaskParams.set(-1.0f, -1.0f, -1.0f, -1.0f);

	m_bIsPlayer = false;
}

/// CannonActorComponent::enable_internal
void CannonActorComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	m_OverridenFXMaskParams.set(-1.0f, -1.0f, -1.0f, -1.0f);

	if (bEnable) {
		m_SkelModelsList.clear();
		const int NumComponents = (int)GetOwner()->NumComponents();
		for (int i = 0; i < NumComponents; i++) {
			kbComponent* const pComponent = GetOwner()->GetComponent(i);
			if (pComponent->IsA(SkeletalModelComponent::GetType()) == false) {
				continue;
			}
			m_SkelModelsList.push_back((SkeletalModelComponent*)pComponent);
		}

		if (m_SkelModelsList.size() > 0) {
			for (int i = 0; i < m_SkelModelsList.size(); i++) {
				m_SkelModelsList[i]->RegisterAnimEventListener(this);
			}

			m_SkelModelsList[0]->RegisterSyncSkelModel(m_SkelModelsList[1]);
			const static kbString smearParam("smearParams");
			m_SkelModelsList[1]->set_material_param_vec4(0, smearParam.stl_str(), Vec4::zero);
		}
	} else {
		for (int i = 0; i < m_SkelModelsList.size(); i++) {
			m_SkelModelsList[i]->UnregisterAnimEventListener(this);
		}

		if (m_SkelModelsList.size() > 1) {
			m_SkelModelsList[0]->UnregisterSyncSkelModel(m_SkelModelsList[1]);
		}
		m_SkelModelsList.clear();
	}
}

/// CannonActorComponent::update_internal
void CannonActorComponent::update_internal(const float DT) {
	Super::update_internal(DT);

	const Quat4 curRot = owner_rotation();

	Mat4 facingMat;
	facingMat.look_at(owner_position(), owner_position() + m_TargetFacingDirection, Vec3::up);

	const Quat4 targetRot = Quat4::from_mat4(facingMat);
	GetOwner()->SetOrientation(curRot.slerp(curRot, targetRot, DT * m_MaxRotateSpeed));

	// Anim Smear
	if (m_AnimSmearStartTime > 0.0f) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_AnimSmearStartTime;
		if (elapsedTime > m_AnimSmearDuration) {
			m_AnimSmearStartTime = -1.0f;
			const static kbString smearParam("smearParams");
			m_SkelModelsList[1]->set_material_param_vec4(0, smearParam.stl_str(), Vec4::zero);
		} else {
			const float strength = 1.0f - kbClamp(elapsedTime / m_AnimSmearDuration, 0.0f, 1.0f);
			const static kbString smearParam("smearParams");
			const Vec4 smearVec = strength * m_AnimSmearVec;
			m_SkelModelsList[1]->set_material_param_vec4(0, smearParam.stl_str(), smearVec);
		}
	}
}

/// CannonActorComponent::PlayAnimation
void CannonActorComponent::PlayAnimation(const kbString animName, const float animBlendInLen, const bool bRestartIfAlreadyPlaying, const kbString nextAnimName, const float nextAnimBlendInLen) {

	if (m_SkelModelsList.size() > 0) {
		m_SkelModelsList[0]->PlayAnimation(animName, animBlendInLen, bRestartIfAlreadyPlaying, nextAnimName, nextAnimBlendInLen);
	}
}

/// CannonActorComponent::HasFinishedAnim
bool CannonActorComponent::HasFinishedAnim(const kbString animName) const {

	if (m_SkelModelsList.size() == 0) {
		blk::warn("KungFuSheepComponent::HasFinishedAnim() - Called with empty m_SkelModels list");
		return true;
	}

	if (animName != kbString::EmptyString) {
		const kbString* pCurAnim = m_SkelModelsList[0]->GetCurAnimationName();
		const kbString* pNextAnim = m_SkelModelsList[0]->GetNextAnimationName();

		if (pCurAnim != nullptr && *pCurAnim == animName) {
			return m_SkelModelsList[0]->HasFinishedAnimation();
		}

		if (pNextAnim != nullptr && *pNextAnim == animName) {
			return false;
		}
	}

	return m_SkelModelsList[0]->HasFinishedAnimation();
}

/// CannonActorComponent::SetAnimationTimeScaleMultiplier
void CannonActorComponent::SetAnimationTimeScaleMultiplier(const kbString animName, const float multiplier) {
	if (m_SkelModelsList.size() < 2) {
		blk::warn("KungFuSheepComponent::SetAnimationTimeMultiplier() - Needs at least 2 skeletal models");
		blk::warn("KungFuSheepComponent::SetAnimationTimeMultiplier() - Needs at least 2 skeletal models");
		return;
	}

	m_SkelModelsList[0]->SetAnimationTimeScaleMultiplier(animName, multiplier);
	m_SkelModelsList[1]->SetAnimationTimeScaleMultiplier(animName, multiplier);
}

/// CannonActorComponent::ApplyAnimSmear
void CannonActorComponent::ApplyAnimSmear(const Vec3 smearVec, const float durationSec) {
	m_AnimSmearStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_AnimSmearVec = smearVec;
	m_AnimSmearDuration = durationSec;
}

/// CannonActorComponent::SetOverrideFXMaskParameters
void CannonActorComponent::SetOverrideFXMaskParameters(const Vec4& fxParams) {
	m_OverridenFXMaskParams = fxParams;
}

/// CannonActorComponent::IsPlayingAnim
bool CannonActorComponent::IsPlayingAnim(const kbString animName) const {
	if (m_SkelModelsList.size() == 0) {
		return false;
	}

	return m_SkelModelsList[0]->IsPlaying(animName);
}

/// CannonActorComponent::PlayAttackVO
void CannonActorComponent::PlayAttackVO(const int pref) {

	if (m_AttackVO.size() == 0) {
		return;
	}

	const float curTime = g_GlobalTimer.TimeElapsedSeconds();
	if (curTime < m_LastVOTime + 2.0f) {
		return;
	}
	m_LastVOTime = curTime;

	m_AttackVO[rand() % m_AttackVO.size()].PlaySoundAtPosition(owner_position());
}

/// CannonActorComponent::Constructor
void CannonCameraComponent::Constructor() {
	// Editor
	m_NearPlane = 1.0f;
	m_FarPlane = 20000.0f;		// TODO - NEAR/FAR PLANE - Tie into renderer properly
	m_positionOffset.set(0.0f, 0.0f, 0.0f);
	m_LookAtOffset.set(0.0f, 0.0f, 0.0f);

	m_MoveMode = MoveMode_Follow;
	m_pTarget = nullptr;

	// Game
	m_SwitchTargetBlendSpeed = 1.0f;
	m_SwitchTargetCurT = 1.0f;
	m_SwitchTargetStartPos.set(0.0f, 0.0f, 0.0f);

	m_SwitchPosOffsetBlendSpeed = 1.0f;
	m_SwitchPosOffsetCurT = 1.0f;
	m_PosOffsetTarget.set(0.0f, 0.0f, 0.0f);

	m_SwitchLookAtOffsetBlendSpeed = 1.0f;
	m_SwitchLookAtOffsetCurT = 1.0f;
	m_LookAtOffsetTarget.set(0.0f, 0.0f, 0.0f);

	m_CameraShakeStartTime = -1.0f;
	m_CameraShakeStartingOffset.set(0.0f, 0.0f);
	m_CameraShakeDuration = 0.0f;
	m_CameraShakeAmplitude.set(0.0f, 0.0f);
	m_CameraShakeFrequency.set(0.0f, 0.0f);
}

/// CannonActorComponent::enable_internal
void CannonCameraComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	m_pTarget = nullptr;
	g_pRenderer->SetNearFarPlane(nullptr, m_NearPlane, m_FarPlane);
}

/// CannonActorComponent::SetTarget
void CannonCameraComponent::SetTarget(const kbGameEntity* const pTarget, const float blendRate) {
	m_SwitchTargetBlendSpeed = blendRate;

	if (m_SwitchTargetBlendSpeed > 0) {
		if (m_pTarget != nullptr) {
			m_SwitchTargetStartPos = m_pTarget->GetPosition();
			m_SwitchTargetCurT = 0.0f;

		} else {
			m_SwitchTargetBlendSpeed = -1.0f;
		}
	}

	m_pTarget = pTarget;
}

/// CannonActorComponent::SetPositionOffset
void CannonCameraComponent::SetPositionOffset(const Vec3& posOffset, const float blendRate) {

	if (blendRate < 0.0f) {
		m_SwitchPosOffsetCurT = 1.0f;
		m_positionOffset = posOffset;
	} else {
		m_SwitchPosOffsetCurT = 0.0f;
		m_SwitchPosOffsetBlendSpeed = blendRate;
		m_PosOffsetTarget = posOffset;
	}
}

/// CannonActorComponent::SetLookAtOffset
void CannonCameraComponent::SetLookAtOffset(const Vec3& lookAtOffset, const float blendRate) {

	if (blendRate < 0.0f) {
		m_SwitchLookAtOffsetCurT = 1.0f;
		m_LookAtOffset = lookAtOffset;
	} else {
		m_SwitchLookAtOffsetBlendSpeed = blendRate;
		m_SwitchLookAtOffsetCurT = 0.0f;
		m_LookAtOffsetTarget = lookAtOffset;
	}
}

/// CannonActorComponent::StartCameraShake
void CannonCameraComponent::StartCameraShake(const CannonCameraShakeComponent* const pCameraShakeComponent) {

	m_CameraShakeStartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_CameraShakeStartingOffset = Vec2Rand(-m_CameraShakeAmplitude, m_CameraShakeAmplitude);
	m_CameraShakeDuration = pCameraShakeComponent->GetDuration();
	m_CameraShakeAmplitude = pCameraShakeComponent->GetAmplitude();
	m_CameraShakeFrequency = pCameraShakeComponent->GetFrequency();
}

/// CannonActorComponent::update_internal
void CannonCameraComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	Vec2 camShakeOffset(0.0f, 0.0f);
	if (m_CameraShakeStartTime > 0.0f) {
		const float elapsedTime = g_GlobalTimer.TimeElapsedSeconds() - m_CameraShakeStartTime;
		if (elapsedTime > m_CameraShakeDuration) {
			m_CameraShakeStartTime = -1.0f;
		} else {
			const float fallOff = 1.0f - kbClamp((elapsedTime / m_CameraShakeDuration), 0.0f, 1.0f);
			camShakeOffset.x = sin(m_CameraShakeStartingOffset.x + (g_GlobalTimer.TimeElapsedSeconds() * m_CameraShakeFrequency.x)) * m_CameraShakeAmplitude.x * fallOff;
			camShakeOffset.y = sin(m_CameraShakeStartingOffset.y + (g_GlobalTimer.TimeElapsedSeconds() * m_CameraShakeFrequency.y)) * m_CameraShakeAmplitude.y * fallOff;
		}
	}

	switch (m_MoveMode) {
		case MoveMode_None: {
		}
						  break;

		case MoveMode_Follow: {
			if (m_pTarget != nullptr) {

				// Target blend to
				Vec3 targetPosition = m_pTarget->GetPosition();
				if (m_SwitchTargetCurT < 1.0f) {
					m_SwitchTargetCurT += m_SwitchTargetBlendSpeed * g_pGame->GetFrameDT();
					targetPosition = kbLerp(m_SwitchTargetStartPos, targetPosition, kbSaturate(m_SwitchTargetCurT));
				}

				// LookAt offset blend
				Vec3 lookAtOffset = m_LookAtOffset;
				if (m_SwitchLookAtOffsetCurT < 1.0f) {
					m_SwitchLookAtOffsetCurT += m_SwitchLookAtOffsetBlendSpeed * g_pGame->GetFrameDT();
					lookAtOffset = kbLerp(m_LookAtOffset, m_LookAtOffsetTarget, kbSaturate(m_SwitchLookAtOffsetCurT));
					if (m_SwitchLookAtOffsetCurT > 1.0f) {
						m_LookAtOffset = m_LookAtOffsetTarget;
					}
				}

				// Position offset blend
				Vec3 positionOffset = m_positionOffset;
				if (m_SwitchPosOffsetCurT < 1.0f) {
					m_SwitchPosOffsetCurT += m_SwitchPosOffsetBlendSpeed * g_pGame->GetFrameDT();
					positionOffset = kbLerp(m_positionOffset, m_PosOffsetTarget, kbSaturate(m_SwitchPosOffsetCurT));
					if (m_SwitchPosOffsetCurT >= 1.0f) {
						m_positionOffset = m_PosOffsetTarget;
					}
				}

				GetOwner()->SetPosition(targetPosition + positionOffset);

				Mat4 cameraDestRot;
				cameraDestRot.look_at(GetOwner()->GetPosition(), targetPosition + lookAtOffset, Vec3::up);
				cameraDestRot.inverse_fast();
				GetOwner()->SetOrientation(Quat4::from_mat4(cameraDestRot));

				const Vec3 cameraDestPos = targetPosition + positionOffset;
				GetOwner()->SetPosition(cameraDestPos + cameraDestRot[0].ToVec3() * camShakeOffset.x + cameraDestRot[1].ToVec3() * camShakeOffset.y);
				GetOwner()->SetPosition(cameraDestPos + cameraDestRot[0].ToVec3() * camShakeOffset.x + cameraDestRot[1].ToVec3() * camShakeOffset.y);
			}
		}
							break;
	}
}

/// CannonCameraShakeComponent::Constructor
void CannonCameraShakeComponent::Constructor() {
	m_Duration = 1.0f;
	m_AmplitudeX = 0.025f;
	m_AmplitudeY = 0.019f;

	m_FrequencyX = 15.0f;
	m_FrequencyY = 10.0f;

	m_ActivationDelaySeconds = 0.0f;
	m_bActivateOnEnable = false;

	m_ShakeStartTime = -1.0f;
}

/// CannonCameraShakeComponent::enable_internal
void CannonCameraShakeComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (bEnable) {
		m_ShakeStartTime = g_GlobalTimer.TimeElapsedSeconds() + m_ActivationDelaySeconds;
	}
}

/// CannonCameraShakeComponent::update_internal
void CannonCameraShakeComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (m_bActivateOnEnable && g_GlobalTimer.TimeElapsedSeconds() > m_ShakeStartTime) {
		// Disable so that this component doesn't prevent it's owning entity to linger past it's life time
		Enable(false);

		CannonCameraComponent* const pCam = (CannonCameraComponent*)g_pCannonGame->GetMainCamera();
		if (pCam != nullptr) {
			pCam->StartCameraShake(this);
		}
	}
}
