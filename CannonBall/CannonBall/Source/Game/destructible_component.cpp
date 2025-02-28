/// destructible_component.cpp
///  
/// 2025 blk 1.0

#include "blk_core.h"
#include "blk_containers.h"
#include "Matrix.h"
#include "kbRenderer_defs.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"
#include "kbRenderer.h"
#include "destructible_component.h"
#include "kbRenderer.h"
#include "DX11/kbRenderer_DX11.h"			// HACK


KB_DEFINE_COMPONENT(EtherAnimComponent)
KB_DEFINE_COMPONENT(EtherSkelModelComponent)

#define DEBUG_ANIMS 0

// TODO: HACK!
static XMMATRIX& XMMATRIXFromMat4(Mat4& matrix) { return (*(XMMATRIX*)&matrix); }
static Mat4& Mat4FromXMMATRIX(FXMMATRIX& matrix) { return (*(Mat4*)&matrix); }

/**
 *	EtherAnimComponent::Constructor()
 */
void EtherAnimComponent::Constructor() {
	m_pAnimation = nullptr;
	m_TimeScale = 1.0f;
	m_bIsLooping = false;
	m_CurrentAnimationTime = -1.0f;
}

/**
 *	EtherSkelModelComponent::Constructor(
 */
void EtherSkelModelComponent::Constructor() {
	m_DebugAnimIdx = -1;
	m_bFirstPersonModel = false;
	m_CurrentAnimation = -1;
	m_NextAnimation = -1;
}

/**
 *	EtherSkelModelComponent::PlayAnimation
 */
void EtherSkelModelComponent::PlayAnimation(const kbString& AnimationName, const float BlendLength, const bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength) {

#if DEBUG_ANIMS
	kbLog("Attempting to play Animation %s ===================================================================", AnimationName.c_str());
#endif

	if (bRestartIfAlreadyPlaying == false && IsPlaying(AnimationName)) {
		return;
	}

	const std::string& animName = AnimationName.stl_str();
	for (int i = 0; i < m_Animations.size(); i++) {
		const std::string& CurName = m_Animations[i].m_AnimationName.stl_str();
		if (m_Animations[i].m_AnimationName == AnimationName) {
#if DEBUG_ANIMS
			kbLog("	Found desired animation");
#endif

			if (BlendLength <= 0.0f || m_CurrentAnimation == -1) {

#if DEBUG_ANIMS
				kbLog("	Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation);
#endif

				// Stop previous animation
				if (m_CurrentAnimation != -1 && m_CurrentAnimation != i) {
#if DEBUG_ANIMS
					kbLog("	Stopping Animation %s", m_Animations[m_CurrentAnimation].GetAnimationName().c_str());
#endif

					m_Animations[m_CurrentAnimation].m_CurrentAnimationTime = -1;
				}

				if (m_NextAnimation != -1 && m_NextAnimation != i) {

#if DEBUG_ANIMS
					kbLog("	Canceling next animation %s", m_Animations[m_NextAnimation].GetAnimationName().c_str());
#endif

					m_Animations[m_NextAnimation].m_CurrentAnimationTime = -1;
				}
				m_NextAnimation = -1;

				// Start current animation
				if (m_CurrentAnimation != i) {
					m_Animations[i].m_CurrentAnimationTime = 0.0f;
				}
				m_CurrentAnimation = i;

#if DEBUG_ANIMS
				kbLog("	Anim all set up.  Next anim = %s.  Desired blend length = %f", desiredNextAnimation.c_str(), desiredNextAnimationBlendLength);
#endif

				m_Animations[m_CurrentAnimation].m_DesiredNextAnimation = desiredNextAnimation;
				m_Animations[m_CurrentAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
			} else {
				m_BlendStartTime = g_GlobalTimer.TimeElapsedSeconds();
				m_BlendLength = BlendLength;
				m_NextAnimation = i;

				m_Animations[m_NextAnimation].m_DesiredNextAnimation = desiredNextAnimation;
				m_Animations[m_NextAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
				m_Animations[m_NextAnimation].m_CurrentAnimationTime = 0.0f;


#if DEBUG_ANIMS
				kbLog("	Blending this animation in %f %d.  Desired next = %s, desired len = %f ", m_BlendLength, m_NextAnimation, desiredNextAnimation.c_str(), desiredNextAnimationBlendLength);
#endif
			}

			break;
		}
	}
}

/**
 *	EtherSkelModelComponent::IsPlaying
 */
bool EtherSkelModelComponent::IsPlaying(const kbString& AnimationName) const {
	if (m_Animations.size() == 0) {
		return false;
	}

	if (m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_AnimationName == AnimationName) {
		return true;
	}

	if (m_CurrentAnimation != -1 && m_Animations[m_CurrentAnimation].m_AnimationName == AnimationName) {
		return true;
	}

	return false;
}

/// EtherSkelModelComponent::enable_internal
void EtherSkelModelComponent::enable_internal(const bool isEnabled) {
	Super::enable_internal(isEnabled);
}

/// EtherSkelModelComponent::update_internal
void EtherSkelModelComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (m_model != nullptr) {
		if (m_BindToLocalSpaceMatrices.size() == 0) {
			m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
		}

		// Debug Animation
		if (m_DebugAnimIdx >= 0 && m_DebugAnimIdx < m_Animations.size() && m_Animations[m_DebugAnimIdx].m_pAnimation != nullptr) {
			if (m_model != nullptr) {
				static float time = 0.0f;
				static bool pause = false;

				if (pause == false) {
					const float AnimTimeScale = m_Animations[m_DebugAnimIdx].m_TimeScale;
					time += DeltaTime * AnimTimeScale;

					if (m_Animations[m_DebugAnimIdx].m_bIsLooping == false) {
						time = kbClamp(time, 0.0f, m_Animations[m_DebugAnimIdx].m_pAnimation->GetLengthInSeconds());
					}
				}

				if (m_BindToLocalSpaceMatrices.size() == 0) {
					m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
				}

				m_model->Animate(m_BindToLocalSpaceMatrices, time, m_Animations[m_DebugAnimIdx].m_pAnimation, m_Animations[m_DebugAnimIdx].m_bIsLooping);
			}
		} else {
			for (int i = 0; i < m_model->NumBones(); i++) {
				m_BindToLocalSpaceMatrices[i].SetIdentity();
			}
		}

		EtherDestructibleComponent* const pDestructible = (EtherDestructibleComponent*)GetOwner()->GetComponentByType(EtherDestructibleComponent::GetType());
		if (pDestructible != nullptr && pDestructible->IsSimulating()) {

			const std::vector<EtherDestructibleComponent::destructibleBone_t>& brokenBones = pDestructible->GetBonesList();
			const kbModel* const pModel = this->model();
			for (int i = 0; i < brokenBones.size(); i++) {
				const EtherDestructibleComponent::destructibleBone_t& destructibleBone = brokenBones[i];

				Quat4 rot;
				rot.from_axis_angle(destructibleBone.m_RotationAxis, destructibleBone.m_CurRotationAngle);
				Mat4 matRot = rot.to_mat4();
				m_BindToLocalSpaceMatrices[i].SetAxis(0, matRot[0].ToVec3());
				m_BindToLocalSpaceMatrices[i].SetAxis(1, matRot[1].ToVec3());
				m_BindToLocalSpaceMatrices[i].SetAxis(2, matRot[2].ToVec3());

				m_BindToLocalSpaceMatrices[i].SetAxis(3, destructibleBone.m_Position);

				m_BindToLocalSpaceMatrices[i] = pModel->GetInvRefBoneMatrix(i) * m_BindToLocalSpaceMatrices[i];

				//kbVec3 worldPos = destructibleBone.m_Position * GetOwner()->GetOrientation().ToMat4() + GetOwner()->GetPosition();
				//g_pRenderer->DrawBox( kbBounds( worldPos - kbVec3::one * 0.1f, worldPos + kbVec3::one * 0.1f ), kbColor::red );
			}
		}


		if (m_CurrentAnimation != -1) {
#if DEBUG_ANIMS
			kbLog("Updating current anim %s", m_Animations[m_CurrentAnimation].GetAnimationName().c_str());
#endif
			// Check if the blend is finished
			if (m_NextAnimation != -1) {
				const float blendTime = (g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime) / m_BlendLength;

#if DEBUG_ANIMS
				kbLog("	Checking if blend is finished.  Blend time is %f", blendTime);
#endif
				if (blendTime >= 1.0f) {
					m_CurrentAnimation = m_NextAnimation;
					m_NextAnimation = -1;

#if DEBUG_ANIMS
					kbLog("	%s Transition to Next Animation", GetOwner()->GetName().c_str());
#endif
				}
			}

			EtherAnimComponent& CurAnim = m_Animations[m_CurrentAnimation];

			bool bAnimIsFinished = false;

			if (CurAnim.m_bIsLooping == false) {
				if (CurAnim.m_CurrentAnimationTime >= CurAnim.m_pAnimation->GetLengthInSeconds()) {

#if DEBUG_ANIMS
					kbLog("	Cur anim is finished!");
#endif
					CurAnim.m_CurrentAnimationTime = CurAnim.m_pAnimation->GetLengthInSeconds();
					bAnimIsFinished = true;
				}
			}

			if (m_NextAnimation == -1) {
				const float prevAnimTime = CurAnim.m_CurrentAnimationTime;

				CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;

				for (int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++) {
					auto& curEvent = CurAnim.m_AnimEvents[iAnimEvent];
					if (curEvent.GetEventTime() > prevAnimTime && curEvent.GetEventTime() <= CurAnim.m_CurrentAnimationTime) {
						blk::log("AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime());
					}
				}

				if (m_BindToLocalSpaceMatrices.size() == 0) {
					m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
				}

#if DEBUG_ANIMS
				kbLog("	Not blending anim %s. anim time = %f", CurAnim.m_AnimationName.c_str(), CurAnim.m_CurrentAnimationTime);
#endif
				m_model->Animate(m_BindToLocalSpaceMatrices, CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, CurAnim.m_bIsLooping);

				if (bAnimIsFinished && CurAnim.m_DesiredNextAnimation.IsEmptyString() == false) {

#if DEBUG_ANIMS
					kbLog("	Cur Animation Done, going to %s - %f", CurAnim.m_DesiredNextAnimation.c_str(), CurAnim.m_DesiredNextAnimBlendLength);
#endif

					PlayAnimation(CurAnim.m_DesiredNextAnimation, CurAnim.m_DesiredNextAnimBlendLength, true);
				}
			} else {
				if (m_BindToLocalSpaceMatrices.size() == 0) {
					m_BindToLocalSpaceMatrices.resize(m_model->NumBones());
				}

				if (bAnimIsFinished == false) {
					const float prevAnimTime = CurAnim.m_CurrentAnimationTime;

					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;

					for (int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++) {
						auto& curEvent = CurAnim.m_AnimEvents[iAnimEvent];
						if (curEvent.GetEventTime() > prevAnimTime && curEvent.GetEventTime() <= CurAnim.m_CurrentAnimationTime) {
							blk::log("AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime());
						}
					}
					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;
				}

				EtherAnimComponent& NextAnim = m_Animations[m_NextAnimation];
				const float prevNextAnimTime = NextAnim.m_CurrentAnimationTime;
				if (CurAnim.m_bIsLooping && NextAnim.m_bIsLooping) {
					// Sync the anims if they're both looping
					NextAnim.m_CurrentAnimationTime = CurAnim.m_CurrentAnimationTime;
				} else {
					NextAnim.m_CurrentAnimationTime += DeltaTime * NextAnim.m_TimeScale;
				}

				for (int iAnimEvent = 0; iAnimEvent < NextAnim.m_AnimEvents.size(); iAnimEvent++) {
					auto& curEvent = NextAnim.m_AnimEvents[iAnimEvent];
					if (curEvent.GetEventTime() > prevNextAnimTime && curEvent.GetEventTime() <= NextAnim.m_CurrentAnimationTime) {
						blk::log("AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime());
					}
				}

				const float blendTime = kbClamp((g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime) / m_BlendLength, 0.0f, 1.0f);
//.				m_model->BlendAnimations(CurAnim.m_pAnimation, CurAnim.m_CurrentAnimationTime, CurAnim.m_bIsLooping, NextAnim.m_pAnimation, NextAnim.m_CurrentAnimationTime, NextAnim.m_bIsLooping, blendTime, m_BindToLocalSpaceMatrices);

#if DEBUG_ANIMS
				kbLog("	Blending anims %f.  %s cur time = %f. %s cur time is %f", blendTime, CurAnim.GetAnimationName().c_str(), CurAnim.m_CurrentAnimationTime, NextAnim.GetAnimationName().c_str(), NextAnim.m_CurrentAnimationTime);
#endif
			}
		}

		m_render_object.m_pComponent = this;
		m_render_object.m_Position = GetOwner()->GetPosition();
		m_render_object.m_Orientation = GetOwner()->GetOrientation();
		m_render_object.m_Scale = GetOwner()->GetScale();
		m_render_object.m_model = m_model;
		m_render_object.m_render_pass = m_render_pass;

		g_pRenderer->UpdateRenderObject(m_render_object);
	}

	// Update collision component
	EtherSkelModelComponent* const pSkelModel = this;
	kbCollisionComponent* const pCollisionComponent = (kbCollisionComponent*)GetOwner()->GetComponentByType(kbCollisionComponent::GetType());

	if (pCollisionComponent != nullptr && pSkelModel != nullptr) {

		const std::vector<kbBoneCollisionSphere>& BindPoseCollisionSpheres = pCollisionComponent->GetLocalSpaceCollisionSpheres();

		// Update collision space world spheres
		if (BindPoseCollisionSpheres.size() > 0) {

			for (int iCollision = 0; iCollision < BindPoseCollisionSpheres.size(); iCollision++) {
				kbBoneMatrix_t jointMatrix;
				if (pSkelModel->GetBoneWorldMatrix(BindPoseCollisionSpheres[iCollision].GetBoneName(), jointMatrix)) {
					Vec4 newBoneWorldPos = BindPoseCollisionSpheres[iCollision].GetSphere().ToVec3() * jointMatrix;
					newBoneWorldPos.a = BindPoseCollisionSpheres[iCollision].GetSphere().a;
					pCollisionComponent->SetWorldSpaceCollisionSphere(iCollision, newBoneWorldPos);
				} else {
					blk::error("EtherAIComponent::Update_Internal() - Failed to get a world matrix for bone %s on model %s", BindPoseCollisionSpheres[iCollision].GetBoneName().c_str(), pSkelModel->model()->GetName().c_str());
				}
			}
		}
	}
	// Temp: Search for "Additional Cloth Bones" and draw the hair locks for this AI using axial bill boards
	kbClothComponent* pClothComponent = nullptr;
	for (int i = 0; i < GetOwner()->NumComponents(); i++) {
		if (GetOwner()->GetComponent(i)->IsA(kbClothComponent::GetType())) {
			pClothComponent = static_cast<kbClothComponent*>(GetOwner()->GetComponent(i));
			break;
		}
	}

	if (pClothComponent != nullptr && pClothComponent->GetMasses().size() > 0) {

		const int startAdditionalBonedIdx = (int)pClothComponent->GetBoneInfo().size();
		const std::vector<kbClothMass_t>& ClothMasses = pClothComponent->GetMasses();

		for (int iMass = startAdditionalBonedIdx; iMass < ClothMasses.size() - 1; iMass++) {
			if (ClothMasses[iMass + 1].m_bAnchored) {
				continue;
			}

			const Vec3 midPt = (ClothMasses[iMass].GetPosition() + ClothMasses[iMass + 1].GetPosition()) * 0.5f;
			const float distance = (ClothMasses[iMass].GetPosition() - ClothMasses[iMass + 1].GetPosition()).length();
			const Vec3 direction = (ClothMasses[iMass + 1].GetPosition() - ClothMasses[iMass].GetPosition()) / distance;

			kbParticleManager::CustomParticleAtlasInfo_t ParticleInfo;
			ParticleInfo.m_Position = midPt;
			ParticleInfo.m_Direction = direction;
			ParticleInfo.m_Width = distance;
			ParticleInfo.m_Height = 4.0f;
			ParticleInfo.m_Color.set(1.0f, 1.0f, 1.0f, 1.0f);
			ParticleInfo.m_UVs[0].set(0.25f, 0.0f);
			ParticleInfo.m_UVs[1].set(0.25f + 0.125f, 0.125f);
			ParticleInfo.m_Type = BT_AxialBillboard;

//			g_pGame->GetParticleManager()->AddQuad(0, ParticleInfo);
		}
	}
}

/**
 *	EtherSkelModelComponent::GetCurAnimationName
 */
const kbString* EtherSkelModelComponent::GetCurAnimationName() const {
	if (m_CurrentAnimation >= 0 && m_CurrentAnimation < m_Animations.size()) {
		return &m_Animations[m_CurrentAnimation].GetAnimationName();
	}

	return nullptr;
}

/**
 *	EtherDestructibleComponent::Constructor
 */
void EtherDestructibleComponent::Constructor() {
	m_DestructibleType = EDestructibleBehavior::PushFromImpactPoint;
	m_MaxLifeTime = 4.0f;
	m_Gravity.set(0.0f, 22.0f, 0.0f);
	m_MinLinearVelocity.set(20.0f, 20.0f, 20.0f);
	m_MaxLinearVelocity.set(25.0f, 25.0f, 25.0f);
	m_MinAngularVelocity = 5.0f;
	m_MaxAngularVelocity = 10.0f;
	m_StartingHealth = 6.0f;
	m_DestructionFXLocalOffset.set(0.0f, 0.0f, 0.0f);

	m_bDebugResetSim = false;
	m_Health = 6.0f;
	m_pSkelModel = nullptr;
	m_bIsSimulating = false;
	m_SimStartTime = 0.0f;
}

/**
 *	EtherDestructibleComponent::editor_change
 */
void EtherDestructibleComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "ResetSim") {
		if (m_bIsSimulating) {
			m_bIsSimulating = false;
			m_Health = m_StartingHealth;
		} else {
			TakeDamage(9999999.0f, GetOwner()->GetPosition() + Vec3(kbfrand(), kbfrand(), kbfrand()) * 5.0f, 10000000.0f);
		}
	}
}

/**
 *	EtherDestructibleComponent::TakeDamage
 */
void EtherDestructibleComponent::TakeDamage(const float damageAmt, const Vec3& explosionPosition, const float explosionRadius) {
	m_Health -= damageAmt;
	if (m_Health > 0.0f || m_bIsSimulating) {
		return;
	}

	if (m_pSkelModel == nullptr) {
		m_pSkelModel = (EtherSkelModelComponent*)GetOwner()->GetComponentByType(EtherSkelModelComponent::GetType());
	}

	blk::error_check(m_pSkelModel != nullptr, "EtherDestructibleComponent::TakeDamage() - Missing skeletal model");

	kbCollisionComponent* const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType(kbCollisionComponent::GetType());
	if (pCollision != nullptr) {
		pCollision->Enable(false);
	}

	m_LastHitLocation = explosionPosition;

	Mat4 localMat;
	GetOwner()->CalculateWorldMatrix(localMat);
	const XMMATRIX inverseMat = XMMatrixInverse(nullptr, XMMATRIXFromMat4(localMat));
	localMat = Mat4FromXMMATRIX(inverseMat);

	const Vec3 localExplositionPos = localMat.transform_point(explosionPosition);
	const kbModel* const pModel = m_pSkelModel->model();

	Mat4 worldMat = localMat;
	worldMat.transpose_self();

	m_BonesList.resize(pModel->NumBones());
	for (int i = 0; i < pModel->NumBones(); i++) {
		m_BonesList[i].m_Position = pModel->GetRefBoneMatrix(i).GetOrigin();

		if (m_DestructibleType == EDestructibleBehavior::UserVelocity) {
			m_BonesList[i].m_Velocity = Vec3Rand(m_MinLinearVelocity, m_MaxLinearVelocity);


			m_BonesList[i].m_Velocity = m_BonesList[i].m_Velocity * worldMat;

		} else {
			m_BonesList[i].m_Velocity = (m_BonesList[i].m_Position - localExplositionPos).normalize_safe() * (kbfrand() * (m_MaxLinearVelocity.x - m_MinLinearVelocity.x) + m_MinLinearVelocity.x);
		}

		m_BonesList[i].m_Acceleration = Vec3::zero;
		m_BonesList[i].m_RotationAxis = Vec3(kbfrand(), kbfrand(), kbfrand());
		m_BonesList[i].m_RotationSpeed = kbfrand() * (m_MaxAngularVelocity - m_MinAngularVelocity) + m_MinAngularVelocity;
		m_BonesList[i].m_CurRotationAngle = 0.0f;
	}

	//	if ( m_CompleteDestructionFX.size() > 0 ) {
		//	const int iFX = rand() % m_CompleteDestructionFX.size();
	if (m_CompleteDestructionFX.GetEntity() != nullptr) {
		kbGameEntity* const pExplosionFX = g_pGame->CreateEntity(m_CompleteDestructionFX.GetEntity());

		Vec3 worldOffset = worldMat.transform_point(m_DestructionFXLocalOffset);
		//kbLog( "% f %f %f --- %f %f %f", m_DestructionFXLocalOffset.x, m_DestructionFXLocalOffset.y, m_DestructionFXLocalOffset.z, worldOffset.x, worldOffset.y, worldOffset.z );
		pExplosionFX->SetPosition(GetOwner()->GetPosition() + worldOffset);
		pExplosionFX->SetOrientation(GetOwner()->GetOrientation());
		pExplosionFX->DeleteWhenComponentsAreInactive(true);
	}
	//}

	m_bIsSimulating = true;
	m_SimStartTime = g_GlobalTimer.TimeElapsedSeconds();
}

/**
 *	EtherDestructibleComponent::enable_internal
 */
void EtherDestructibleComponent::enable_internal(const bool bEnable) {
	if (bEnable == false) {
		m_pSkelModel = nullptr;
	} else {
		m_pSkelModel = (EtherSkelModelComponent*)GetOwner()->GetComponentByType(EtherSkelModelComponent::GetType());
		if (m_pSkelModel == nullptr || m_pSkelModel->model() == nullptr) {
			blk::warn("EtherDestructibleComponent::SetEnable_Internal() - No skeletal model found on entity %", GetOwner()->GetName().c_str());
			this->Enable(false);
			return;
		}

		m_BonesList.resize(m_pSkelModel->model()->NumBones());

		m_Health = m_StartingHealth;
	}
}

extern kbConsoleVariable g_ShowCollision;

/// EtherDestructibleComponent::update_internal
void EtherDestructibleComponent::update_internal(const float deltaTime) {

	if (GetAsyncKeyState('G')) {
		m_bIsSimulating = false;
		m_Health = m_StartingHealth;

		kbCollisionComponent* const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType(kbCollisionComponent::GetType());
		if (pCollision != nullptr) {
			pCollision->Enable(true);
		}
	}

	if (m_bIsSimulating) {
		const float t = g_GlobalTimer.TimeElapsedSeconds() - m_SimStartTime;

		if (t > m_MaxLifeTime) {
			GetOwner()->DisableAllComponents();
			m_bIsSimulating = false;
		} else {
			const float tSqr = t * t;
			const kbModel* const pModel = m_pSkelModel->model();

			for (int i = 0; i < m_BonesList.size(); i++) {
				m_BonesList[i].m_Position.x += m_BonesList[i].m_Velocity.x * deltaTime;
				m_BonesList[i].m_Position.z += m_BonesList[i].m_Velocity.z * deltaTime;

				m_BonesList[i].m_Position.y = pModel->GetRefBoneMatrix(i).GetOrigin().y + (m_BonesList[i].m_Velocity.y * t - (0.5f * m_Gravity.y * tSqr));
				m_BonesList[i].m_CurRotationAngle += m_BonesList[i].m_RotationSpeed * deltaTime;
			}
		}

		if (g_ShowCollision.GetBool()) {
			g_pRenderer->DrawBox(kbBounds(m_LastHitLocation - Vec3::one, m_LastHitLocation + Vec3::one), kbColor::red);
		}
	}
}