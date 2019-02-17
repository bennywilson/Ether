//===================================================================================================
// EtherSkelModel.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERSKELMODEL_H_
#define _ETHERSKELMODEL_H_

#include "kbSkeletalModelComponent.h"


/**
 *	EtherAnimComponent
 */
class EtherAnimComponent : public kbGameComponent {

	friend class EtherSkelModelComponent;

	KB_DECLARE_COMPONENT( EtherAnimComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:


	const kbString &						GetAnimationName() const { return m_AnimationName; }

private:
	kbString								m_AnimationName;
	kbAnimation *							m_pAnimation;
	float									m_TimeScale;
	bool									m_bIsLooping;
	std::vector<kbAnimEvent>				m_AnimEvents;

	float									m_CurrentAnimationTime;
	kbString								m_DesiredNextAnimation;
	float									m_DesiredNextAnimBlendLength;
};

/**
 *	EtherSkelModelComponent
 */
class EtherSkelModelComponent : public kbSkeletalModelComponent {

	KB_DECLARE_COMPONENT( EtherSkelModelComponent, kbSkeletalModelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	void									PlayAnimation( const kbString & AnimationName, const float BlendLength, bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation = kbString::EmptyString, const float desiredNextAnimBlendLength = 0.0f );
	bool									IsPlaying( const kbString & AnimationName ) const;

	void									SetModel( kbModel *const pModel, bool bIsFirstPersonModel );

	bool									IsFirstPersonModel() const { return m_bFirstPersonModel; }

	bool									HasFinishedAnimation() const { return IsTransitioningAnimations() == false && ( m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL || m_Animations[m_CurrentAnimation].m_CurrentAnimationTime >= m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds() ); }
	bool									IsTransitioningAnimations() const { return m_CurrentAnimation != -1 && m_NextAnimation != -1; }

	float									GetCurAnimTimeSeconds() const { if ( m_CurrentAnimation == -1 ) return -1.0f; return m_Animations[m_CurrentAnimation].m_CurrentAnimationTime; }
	float									GetNormalizedAnimTime() const { return GetCurAnimTimeSeconds() / GetCurAnimLengthSeconds(); }
	float									GetCurAnimLengthSeconds() const { if ( m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL ) return -1.0f; return m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds(); }

	const kbString *						GetCurAnimationName() const;

protected:

	virtual void							SetEnable_Internal( const bool isEnabled ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	std::vector<EtherAnimComponent>			m_Animations;
	int										m_DebugAnimIdx;

	int										m_CurrentAnimation;
	int										m_NextAnimation;
	float									m_BlendStartTime;
	float									m_BlendLength;

	bool									m_bFirstPersonModel;
};


/**
 *	EtherDestructibleComponent
 */
class EtherDestructibleComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherDestructibleComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void							EditorChange( const std::string & propertyName ) override;

	void									TakeDamage( const float damageAmt, const kbVec3 & damagePosition, const float damageRadius );

	bool									IsSimulating() const { return m_bIsSimulating; }

	struct destructibleBone_t {
		kbVec3								m_Position;
		kbVec3								m_Acceleration;
		kbVec3								m_Velocity;

		kbVec3								m_RotationAxis;
		float								m_RotationSpeed;
		float								m_CurRotationAngle;
	};
	const std::vector<destructibleBone_t> &	GetBonesList() const { return m_BonesList; }

private:

	void									SetEnable_Internal( const bool bEnable ) override;
	void									Update_Internal( const float deltaTime ) override;

	// Editor
	float									m_MaxLifeTime;
	kbVec3									m_Gravity;
	float									m_MinLinearVelocity;
	float									m_MaxLinearVelocity;
	float									m_MinAngularVelocity;
	float									m_MaxAngularVelocity;
	float									m_StartingHealth;

	bool									m_bDebugResetSim;

	// Run time
	std::vector<destructibleBone_t>				m_BonesList;

	float									m_Health;
	const EtherSkelModelComponent *			m_pSkelModel;
	float									m_SimStartTime;
	kbVec3									m_LastHitLocation;
	bool									m_bIsSimulating;
};
#endif