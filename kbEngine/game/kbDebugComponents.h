/// kbDebugComponents.h
///
/// 2018-2025 blk 1.0

#pragma once

class kbModel;

/// kbDebugSphereCollision
class kbDebugSphereCollision : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbDebugSphereCollision, kbGameComponent);

private:
	virtual void enable_internal(const bool bEnable) override;
	virtual void update_internal(const float DeltaTime) override;

	kbModel* m_pCollisionModel;
	kbRenderObject m_render_object;
};
