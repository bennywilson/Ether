/// kbDebugComponents.h
///
/// 2018-2025 blk 1.0

#pragma once

class kbModel;

/// kbDebugSphereCollision
class kbDebugSphereCollision : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbDebugSphereCollision, kbGameComponent);

private:
	virtual void SetEnable_Internal(const bool bEnable) override;
	virtual void Update_Internal(const float DeltaTime) override;

	kbModel* m_pCollisionModel;
	kbRenderObject m_RenderObject;
};
