//==============================================================================
// kbSkeletalModelComponent.h
//
// 2016-2019 kbEngine 2.0
//==============================================================================
#ifndef _KBSKELETALMODELCOMPONENT_H_
#define _KBSKELETALMODELCOMPONENT_H_

#include "kbModel.h"

/**
 *	kbAnimComponent
 */
class kbAnimComponent : public kbGameComponent {

	friend class kbSkeletalModelComponent;

	KB_DECLARE_COMPONENT( kbAnimComponent, kbGameComponent );

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
 *	kbSkeletalModelComponent
 */
class kbSkeletalModelComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbSkeletalModelComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbSkeletalModelComponent();

	void										SetModel( class kbModel *const pModel );
	const kbModel *								GetModel() const { return m_pModel; }

	virtual void								EditorChange( const std::string & propertyName );

	int											GetBoneIndex( const kbString & boneName );
	kbBoneMatrix_t								GetBoneRefMatrix( const int index );

	bool										GetBoneWorldPosition( const kbString & boneName, kbVec3 & outWorldPosition );
	bool										GetBoneWorldMatrix( const kbString & boneName, kbBoneMatrix_t & boneMatrix );
	std::vector<kbBoneMatrix_t> &				GetFinalBoneMatrices() { return m_BindToLocalSpaceMatrices; }
	const std::vector<kbBoneMatrix_t> &			GetFinalBoneMatrices() const { return m_BindToLocalSpaceMatrices; }

	// Animation
	void										PlayAnimation( const kbString & AnimationName, const float BlendLength, bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation = kbString::EmptyString, const float desiredNextAnimBlendLength = 0.0f );
	bool										IsPlaying( const kbString & AnimationName ) const;

	bool										HasFinishedAnimation() const { return IsTransitioningAnimations() == false && ( m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL || m_Animations[m_CurrentAnimation].m_CurrentAnimationTime >= m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds() ); }
	bool										IsTransitioningAnimations() const { return m_CurrentAnimation != -1 && m_NextAnimation != -1; }

	float										GetCurAnimTimeSeconds() const { if ( m_CurrentAnimation == -1 ) return -1.0f; return m_Animations[m_CurrentAnimation].m_CurrentAnimationTime; }
	float										GetNormalizedAnimTime() const { return GetCurAnimTimeSeconds() / GetCurAnimLengthSeconds(); }
	float										GetCurAnimLengthSeconds() const { if ( m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL ) return -1.0f; return m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds(); }

	const kbString *							GetCurAnimationName() const;
	const kbString *							GetNextAnimationName() const;

	void										RegisterAnimEventListener( IAnimEventListener *const pListener );
	void										UnregisterAnimEventListener( IAnimEventListener *const pListener );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	std::vector<IAnimEventListener*>			m_AnimEventListeners;

	// Editor
	class kbModel *								m_pModel;
	std::vector<kbAnimComponent>				m_Animations;

	// Game
	std::vector<kbBoneMatrix_t>					m_BindToLocalSpaceMatrices;

	int											m_CurrentAnimation;
	int											m_NextAnimation;
	float										m_BlendStartTime;
	float										m_BlendLength;

	// Debug
	int											m_DebugAnimIdx;
	float										m_DebugAnimTime;
};

/**
 *	kbFlingPhysicsComponent
 */
class kbFlingPhysicsComponent: public kbGameComponent {

	KB_DECLARE_COMPONENT( kbFlingPhysicsComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Editor
	kbVec3										m_MinLinearVelocity;
	kbVec3										m_MaxLinearVelocity;
	float										m_MinAngularSpeed;
	float										m_MaxAngularSpeed;
	kbVec3										m_Gravity;

	// Run time
	kbVec3										m_OwnerStartPos;
	kbQuat										m_OwnerStartRotation;

	kbVec3										m_Velocity;
	kbVec3										m_RotationAxis;

	float										m_CurRotationAngle;
	float										m_RotationSpeed;

	float										m_FlingStartTime;

};

#endif