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
#include "breakable_component.h"
#include "DX11/kbRenderer_DX11.h"			// HACK


KB_DEFINE_COMPONENT(AnimationComponent)
KB_DEFINE_COMPONENT(EtherSkelModelComponent)

#define DEBUG_ANIMS 0

// TODO: HACK!
static XMMATRIX& XMMATRIXFromMat4(Mat4& matrix) { return (*(XMMATRIX*)&matrix); }
static Mat4& Mat4FromXMMATRIX(FXMMATRIX& matrix) { return (*(Mat4*)&matrix); }

/// AnimationComponent::Constructor()
void AnimationComponent::Constructor() {
	m_animation = nullptr;
	m_time_scale = 1.0f;
	m_is_looping = false;
	m_current_time = -1.0f;
}

/// BreakableComponent::Constructor
void BreakableComponent::Constructor() {
	m_destructible_type = EBreakableBehavior::PushFromImpactPoint;
	m_life_duration = 4.0f;
	m_gravity.set(0.0f, 22.0f, 0.0f);
	m_min_linear_vel.set(20.0f, 20.0f, 20.0f);
	m_max_linear_vel.set(25.0f, 25.0f, 25.0f);
	m_min_angular_vel = 5.0f;
	m_max_angular_vel = 10.0f;
	m_starting_health = 6.0f;
	m_fx_local_offset.set(0.0f, 0.0f, 0.0f);

	m_bDebugResetSim = false;
	m_health = 6.0f;
	m_skel_model = nullptr;
	m_is_simulating = false;
	m_sim_start_time = 0.0f;
}

/// BreakableComponent::editor_change
void BreakableComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "ResetSim") {
		if (m_is_simulating) {
			m_is_simulating = false;
			m_health = m_starting_health;
			m_bones.clear();
		} else {
			take_damage(9999999.0f, GetOwner()->GetPosition() + Vec3(kbfrand(), kbfrand(), kbfrand()) * 5.0f, 10000000.0f);
		}
	}
}

/// BreakableComponent::take_damage
void BreakableComponent::take_damage(const f32 damageAmt, const Vec3& explosionPosition, const f32 explosionRadius) {
	m_health -= damageAmt;
	if (m_health > 0.0f || m_is_simulating) {
		return;
	}

	if (m_skel_model == nullptr) {
		m_skel_model = (SkeletalModelComponent*)GetOwner()->GetComponentByType(SkeletalModelComponent::GetType());
		if (m_skel_model == nullptr) {
			blk::error_check(m_skel_model != nullptr, "BreakableComponent::take_damage() - Missing skeletal model");
			return;
		}
	}

	kbCollisionComponent* const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType(kbCollisionComponent::GetType());
	if (pCollision != nullptr) {
		pCollision->Enable(false);
	}

	m_last_hit_location = explosionPosition;

	Mat4 local_mat;
	GetOwner()->CalculateWorldMatrix(local_mat);
	const XMMATRIX inverse_mat = XMMatrixInverse(nullptr, XMMATRIXFromMat4(local_mat));
	local_mat = Mat4FromXMMATRIX(inverse_mat);

	const Vec3 localExplositionPos = local_mat.transform_point(explosionPosition);
	const kbModel* const model = m_skel_model->model();

	Mat4 world_mat = local_mat;
	world_mat.transpose_self();

	m_bones.resize(model->NumBones());
	for (int i = 0; i < model->NumBones(); i++) {
		m_bones[i].m_position = model->GetRefBoneMatrix(i).GetOrigin();

		if (m_destructible_type == EBreakableBehavior::UserVelocity) {
			m_bones[i].m_velocity = Vec3Rand(m_min_linear_vel, m_max_linear_vel) * world_mat;

		} else {
			m_bones[i].m_velocity = (m_bones[i].m_position - localExplositionPos).normalize_safe() * (kbfrand() * (m_max_linear_vel.x - m_min_linear_vel.x) + m_min_linear_vel.x);
		}

		m_bones[i].m_acceleration = Vec3::zero;
		m_bones[i].m_rotation_axis = Vec3(kbfrand(), kbfrand(), kbfrand());
		m_bones[i].m_rotation_speed = kbfrand() * (m_max_angular_vel - m_min_angular_vel) + m_min_angular_vel;
		m_bones[i].m_cur_rotation_angle = 0.0f;
	}

	if (m_complete_destruction_fx.GetEntity() != nullptr) {
		kbGameEntity* const pExplosionFX = g_pGame->CreateEntity(m_complete_destruction_fx.GetEntity());

		Vec3 worldOffset = world_mat.transform_point(m_fx_local_offset);
		pExplosionFX->SetPosition(GetOwner()->GetPosition() + worldOffset);
		pExplosionFX->SetOrientation(GetOwner()->GetOrientation());
		pExplosionFX->DeleteWhenComponentsAreInactive(true);
	}

	m_is_simulating = true;
	m_sim_start_time = g_GlobalTimer.TimeElapsedSeconds();
}

/// BreakableComponent::enable_internal
void BreakableComponent::enable_internal(const bool enable) {
	if (!enable) {
		m_skel_model = nullptr;
	} else {
		m_skel_model = (SkeletalModelComponent*)GetOwner()->GetComponentByType(SkeletalModelComponent::GetType());
		if (m_skel_model == nullptr || m_skel_model->model() == nullptr) {
			blk::warn("BreakableComponent::SetEnable_Internal() - No skeletal model found on entity %", GetOwner()->GetName().c_str());
			this->Enable(false);
			return;
		}

		m_bones.resize(m_skel_model->model()->NumBones());
		m_health = m_starting_health;
	}
}

extern kbConsoleVariable g_ShowCollision;

/// BreakableComponent::update_internal
void BreakableComponent::update_internal(const f32 deltaTime) {
	if (m_is_simulating) {
		const f32 t = g_GlobalTimer.TimeElapsedSeconds() - m_sim_start_time;

		if (t > m_life_duration) {
			GetOwner()->DisableAllComponents();
			m_is_simulating = false;
		} else {
			const f32 tSqr = t * t;
			const kbModel* const pModel = m_skel_model->model();

			for (u32 i = 0; i < m_bones.size(); i++) {
				m_bones[i].m_position.x += m_bones[i].m_velocity.x * deltaTime;
				m_bones[i].m_position.z += m_bones[i].m_velocity.z * deltaTime;

				m_bones[i].m_position.y = pModel->GetRefBoneMatrix(i).GetOrigin().y + (m_bones[i].m_velocity.y * t - (0.5f * m_gravity.y * tSqr));
				m_bones[i].m_cur_rotation_angle += m_bones[i].m_rotation_speed * deltaTime;
			}
		}

		if (g_ShowCollision.GetBool()) {
			g_pRenderer->DrawBox(kbBounds(m_last_hit_location - Vec3::one, m_last_hit_location + Vec3::one), kbColor::red);
		}
	}
}