//==============================================================================
// kbClothComponent.h
//
//
// 2016-2019 kbEngine 2.0
//==============================================================================
#ifndef _KBCLOTHCOMPONENT_H_
#define _KBCLOTHCOMPONENT_H_

/**
 *	EClothType
 */
enum EClothType {
	CT_None,
	CT_Square,
};

/**
 *	kbClothSpring_t
 */
struct kbClothSpring_t {
	int											m_MassIndices[2];
	float										m_Length;
};

/**
 *	kbClothMass_t
 */
struct kbClothMass_t {
	kbClothMass_t() : m_LastPosition( kbVec3::zero ), m_FrameForces( kbVec3::zero ), m_bAnchored( false ) { }

	const kbVec3 &								GetPosition() const { return m_Matrix.GetOrigin(); }
	const kbVec3 &								GetAxis( const int index ) const { return m_Matrix.GetAxis( index ); }
	
	void										SetPosition( const kbVec3 newOrigin ) { m_Matrix.SetAxis( 3, newOrigin ); }
	void										SetAxis( const int index, const kbVec3 & axis ) { m_Matrix.SetAxis( index, axis ); }

	kbBoneMatrix_t								m_Matrix;
	kbVec3										m_LastPosition;
	kbVec3										m_FrameForces;
	bool										m_bAnchored;
};

/**
 *	kbClothBone
 */
class kbClothBone : public kbGameComponent {
public:
	friend class kbClothComponent;

	KB_DECLARE_COMPONENT( kbClothBone, kbGameComponent );

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
private:
	kbString									m_BoneName;
	std::vector<kbString>						m_NeighborBones;
	bool										m_bIsAnchored;
};

/**
 *	kbClothComponent
 */
class kbClothComponent : public kbGameComponent {
public:
	KB_DECLARE_COMPONENT( kbClothComponent, kbGameComponent );

	virtual										~kbClothComponent();

	const std::vector<class kbClothBone> &		GetBoneInfo() const { return m_BoneInfo; }
	const std::vector<class kbClothBone> &		GetAdditionalBoneInfo() const { return m_AdditionalBoneInfo; }
	const std::vector<kbClothMass_t> &			GetMasses() const { return m_Masses; }
	const std::vector<kbClothSpring_t> &		GetSprings() const { return m_Springs; }

	void										AddForceToMass( const int massIdx, const kbVec3 & force ) { m_Masses[massIdx].m_FrameForces += force; }

protected:

	virtual void								RunSimulation( const float DeltaTime );

private:

	virtual void								Update_Internal( const float DeltaTime ) override;

	void										SetupCloth();

	int											m_Width;
	int											m_Height;
	int											m_CurrentTickFrame;
	EClothType									m_ClothType;
	std::vector<class kbClothBone>				m_BoneInfo;
	std::vector<class kbClothBone>				m_AdditionalBoneInfo;
	std::vector<class kbBoneCollisionSphere>	m_CollisionSpheres;
	int											m_NumConstrainIterations;

	kbVec3										m_Gravity;
	// Wind Data
	kbVec3										m_MaxWindVelocity;
	kbVec3										m_MinWindVelocity;
	float										m_MaxWindGustDuration;
	float										m_MinWindGustDuration;
	bool										m_bAddFakeOscillation;

	// Run-time
	kbVec3										m_CurWindVelocity;
	kbVec3										m_NextWindVelocity;
	float										m_NextWindChangeTime;

	const kbModel *								m_pSkeletalModel;

	std::vector<int>							m_BoneIndices;
	std::vector<kbClothMass_t>					m_Masses;	
	std::vector<kbClothSpring_t>				m_Springs;
};


#endif