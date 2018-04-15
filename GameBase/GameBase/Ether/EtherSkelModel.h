//===================================================================================================
// EtherSkelModel.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERSKELMODEL_H_
#define _ETHERSKELMODEL_H_

#include "kbSkeletalModelComponent.h"

/**
 *	EtherAnimComponent
 */
class EtherAnimComponent : public kbGameComponent {
public:
	friend class EtherSkelModelComponent;

	KB_DECLARE_COMPONENT( EtherAnimComponent, kbGameComponent );

	const kbString &						GetAnimationName() const { return m_AnimationName; }

private:
	kbString								m_AnimationName;
	kbAnimation *							m_pAnimation;
	float									m_TimeScale;
	bool									m_bIsLooping;

	float									m_CurrentAnimationTime;
	bool									m_bReturnToIdleWhenDone;
};

/**
 *	EtherSkelModelComponent
 */
class EtherSkelModelComponent : public kbSkeletalModelComponent {
public:
	KB_DECLARE_COMPONENT( EtherSkelModelComponent, kbSkeletalModelComponent );

	void									PlayAnimation( const kbString & AnimationName, const float BlendLength, const bool ReturnToIdleWhenDone );
	bool									IsPlaying( const kbString & AnimationName ) const;

	void									SetModel( kbModel *const pModel, bool bIsFirstPersonModel );

	bool									IsFirstPersonModel() const { return m_bFirstPersonModel; }

	bool									HasFinishedAnimation() const { return IsTransitioningAnimations() == false && ( m_CurrentAnimation == -1 || m_Animations[m_CurrentAnimation].m_pAnimation == NULL || m_Animations[m_CurrentAnimation].m_CurrentAnimationTime >= m_Animations[m_CurrentAnimation].m_pAnimation->GetLengthInSeconds() ); }
	bool									IsTransitioningAnimations() const { return m_CurrentAnimation != -1 && m_NextAnimation != -1; }

	float									GetCurAnimTimeSeconds() const { if ( m_CurrentAnimation == -1 ) return -1.0f; return m_Animations[m_CurrentAnimation].m_CurrentAnimationTime; }
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

#endif