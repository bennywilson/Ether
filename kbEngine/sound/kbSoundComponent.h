//==============================================================================
// kbSoundComponent.h
//
//
// 2017 kbEngine 2.0
//==============================================================================
#ifndef _KBSOUNDCOMPONENT_H_
#define _KBSOUNDCOMPONENT_H_

/**
 *	kbSoundData
 */
class kbSoundData : public kbComponent {

//---------------------------------------------------------------------------------------------------
public:

	KB_DECLARE_COMPONENT( kbSoundData, kbComponent );

	void										PlaySoundAtPosition( const kbVec3 & soundPosition ) const;

private:

	class kbWaveFile *							m_pWaveFile;
	float										m_Radius;
	float										m_Volume;
};

#endif
