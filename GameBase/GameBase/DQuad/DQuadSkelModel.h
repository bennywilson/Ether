//===================================================================================================
// DQuadSkelModel.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _DQUADSKELMODEL_H_
#define _DQUADSKELMODEL_H_

#include "kbSkeletalModelComponent.h"

/**
 *	kbDQuadAnimComponent
 */
class kbDQuadAnimComponent : public kbComponent {
public:
	friend class kbDQuadSkelModelComponent;

	KB_DECLARE_COMPONENT( kbDQuadAnimComponent, kbComponent );

private:
	kbString								m_AnimationName;
	kbAnimation *							m_pAnimation;
	float									m_TimeScale;
	bool									m_bIsLooping;

	float									m_CurrentAnimationTime;
};

/**
 *	kbDQuadSkelModelComponent
 */
class kbDQuadSkelModelComponent : public kbSkeletalModelComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadSkelModelComponent, kbSkeletalModelComponent );

	virtual void							Update( const float DeltaTime );

	void									PlayAnimation( const kbString & AnimationName, const bool bStopAllOtherAnimations );
	bool									IsPlaying( const kbString & AnimationName ) const;

	void									SetModel( kbModel *const pModel, bool bIsFirstPersonModel );

	bool									IsFirstPersonModel() const { return m_bFirstPersonModel; }

protected:

	virtual void							SetEnable_Internal( const bool isEnabled );

	std::vector<kbDQuadAnimComponent>		m_Animations;
	int										m_DebugAnimIdx;

	std::vector<int>						m_CurrentlyPlayingAnimations;
	bool									m_bFirstPersonModel;
};

#endif