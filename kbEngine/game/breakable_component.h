/// EtherSkelModel.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "kbComponent.h"
#include "model_component.h"
#include "kbGameEntity.h"

/// AnimationComponent
class AnimationComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(AnimationComponent, kbGameComponent);

	friend class EtherSkelModelComponent;

public:
	const kbString& animation_name() const { return m_animation_name; }

private:
	kbString m_animation_name;
	kbAnimation* m_animation;
	f32 m_time_scale;
	bool m_is_looping;
	std::vector<kbAnimEvent> m_anim_events;

	f32 m_current_time;
	kbString m_next_anim;
	f32	m_next_anim_blend_duration;
};

enum EBreakableBehavior {
	PushFromImpactPoint,
	UserVelocity,
};

/// BreakableComponent
class BreakableComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(BreakableComponent, kbGameComponent);

public:
	virtual void editor_change(const std::string& propertyName) override;

	void take_damage(const float damageAmt, const Vec3& damagePosition, const float damageRadius);

	bool is_simulating() const { return m_is_simulating; }

	struct DestructibleBone_t {
		Vec3 m_position;
		Vec3 m_acceleration;
		Vec3 m_velocity;

		Vec3 m_rotation_axis;
		f32	m_rotation_speed;
		f32	m_cur_rotation_angle;
	};
	const std::vector<DestructibleBone_t>& get_bones() const { return m_bones; }

private:
	void enable_internal(const bool bEnable) override;
	void update_internal(const float deltaTime) override;

	// Editor
	EBreakableBehavior m_destructible_type;
	f32 m_life_duration;
	Vec3 m_gravity;
	Vec3 m_min_linear_vel;
	Vec3 m_max_linear_vel;
	f32 m_min_angular_vel;
	f32 m_max_angular_vel;
	f32 m_starting_health;

	kbGameEntityPtr	m_complete_destruction_fx;
	Vec3 m_fx_local_offset;

	bool m_bDebugResetSim;

	// Run time
	std::vector<DestructibleBone_t>	m_bones;

	f32 m_health;
	const SkeletalModelComponent* m_skel_model;
	f32	m_sim_start_time;
	Vec3 m_last_hit_location;
	bool m_is_simulating;
};
