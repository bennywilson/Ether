//===================================================================================================
// EtherTV.h
//
//
// 2020 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERTV_H_
#define _ETHERTV_H_


/**
 *	EtherTVComponent
 */
class EtherTVComponent : public EtherActorComponent {

	KB_DECLARE_COMPONENT( EtherTVComponent, EtherActorComponent );

//---------------------------------------------------------------------------------------------------

protected:

	void										SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	// Data
	std::vector<kbTexture*>						m_SheepAndFoxClip;
	float										m_MovieLen;
	float										m_DelayBeforeAd;
	float										m_AdLength;
	float										m_GlitchDuration;
	kbTexture*									m_pELPAd;
	std::vector<kbSoundData>					m_Music;
	std::vector<kbSoundData>					m_VO;

	// Runtime
	kbStaticModelComponent*						m_pStaticModel;
	float										m_StartTime;
	float										m_State;
};

#endif