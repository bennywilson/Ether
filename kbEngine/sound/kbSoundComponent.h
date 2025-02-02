//==============================================================================
// kbSoundComponent.h
//
//
// 2017-2019 kbEngine 2.0
//==============================================================================
#ifndef _KBSOUNDCOMPONENT_H_
#define _KBSOUNDCOMPONENT_H_

/**
 *	kbSoundData
 */
class kbSoundData : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbSoundData, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbSoundData();

	void										PlaySoundAtPosition( const kbVec3 & soundPosition );
	void										StopSound();

	virtual void								EditorChange( const std::string & propertyName ) override;

private:

	// Editor
	class kbWaveFile *							m_pWaveFile;
	float										m_Radius;
	float										m_Volume;

	bool										m_bLooping;
	bool										m_bDebugPlaySound;

	// Runtime
	int											m_SoundId;
};

/**
 *	kbPlaySoundComponent
 */
class kbPlaySoundComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbPlaySoundComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	// Data
	float										m_MinStartDelay;
	float										m_MaxStartDelay;
	std::vector<kbSoundData>					m_SoundData;

	// Run time
	float										m_TimeToPlay;
};

inline void PlayRandomSound( std::vector<kbSoundData> & soundData, const kbVec3 & pos = kbVec3::zero ) {
	
	if ( soundData.size() == 0 ) {
		return;
	}

	soundData[rand() % soundData.size()].PlaySoundAtPosition( pos );
}
#endif
