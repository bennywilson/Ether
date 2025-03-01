/// kbClothComponent.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "kbComponent.h"
#include "kbRenderer_defs.h"

/// EClothType
enum EClothType {
	CT_None,
	CT_Square,
};

/// kbClothSpring_t
struct kbClothSpring_t {
	int	m_MassIndices[2];
	float m_Length;
};

/// kbClothMass_t
struct kbClothMass_t {
	kbClothMass_t() : m_LastPosition(Vec3::zero), m_FrameForces(Vec3::zero), m_bAnchored(false) { }

	const Vec3& GetPosition() const { return m_Matrix.GetOrigin(); }
	const Vec3& GetAxis(const int index) const { return m_Matrix.GetAxis(index); }

	void SetPosition(const Vec3 newOrigin) { m_Matrix.SetAxis(3, newOrigin); }
	void SetAxis(const int index, const Vec3& axis) { m_Matrix.SetAxis(index, axis); }

	kbBoneMatrix_t m_Matrix;
	Vec3 m_LastPosition;
	Vec3 m_FrameForces;
	bool m_bAnchored;
};

/// kbClothBone
class kbClothBone : public kbGameComponent {
public:
	friend class kbClothComponent;

	KB_DECLARE_COMPONENT(kbClothBone, kbGameComponent);

private:
	kbString									m_BoneName;
	std::vector<kbString>						m_NeighborBones;
	bool										m_bIsAnchored;
};

/// kbClothComponent
class kbClothComponent : public kbGameComponent {
public:
	KB_DECLARE_COMPONENT(kbClothComponent, kbGameComponent);

	virtual										~kbClothComponent();

	const std::vector<class kbClothBone>& GetBoneInfo() const { return m_BoneInfo; }
	const std::vector<class kbClothBone>& GetAdditionalBoneInfo() const { return m_AdditionalBoneInfo; }
	const std::vector<kbClothMass_t>& GetMasses() const { return m_Masses; }
	const std::vector<kbClothSpring_t>& GetSprings() const { return m_Springs; }

	void										AddForceToMass(const int massIdx, const Vec3& force) { m_Masses[massIdx].m_FrameForces += force; }

	void										SetClothCollisionSphere(const int idx, const Vec4& sphere);

protected:

	virtual void								RunSimulation(const float DeltaTime);

private:

	virtual void								update_internal(const float DeltaTime) override;

	void										SetupCloth();

	int											m_Width;
	int											m_Height;
	int											m_CurrentTickFrame;
	EClothType									m_ClothType;
	std::vector<class kbClothBone>				m_BoneInfo;
	std::vector<class kbClothBone>				m_AdditionalBoneInfo;
	std::vector<class kbBoneCollisionSphere>	m_CollisionSpheres;
	int											m_NumConstrainIterations;

	Vec3										m_gravity;

	// Wind Data
	Vec3										m_MaxWindVelocity;
	Vec3										m_MinWindVelocity;
	float										m_MaxWindGustDuration;
	float										m_MinWindGustDuration;
	bool										m_bAddFakeOscillation;

	// Run-time
	Vec3										m_CurWindVelocity;
	Vec3										m_NextWindVelocity;
	float										m_NextWindChangeTime;

	const kbModel* m_pSkeletalModel;

	std::vector<int>							m_BoneIndices;
	std::vector<kbClothMass_t>					m_Masses;
	std::vector<kbClothSpring_t>				m_Springs;
};
