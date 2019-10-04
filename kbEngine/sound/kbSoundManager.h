//==============================================================================
// kbSoundManager.h
//
//
// 2017-2019 kbEngine 2.0
//==============================================================================
#ifndef _KBSOUNDMANAGER_H_
#define _KBSOUNDMANAGER_H_

#include <mmsystem.h>
#include <mmreg.h>
#include <XAudio2.h>

/**
 *	kbWaveFile
 */
class kbWaveFile : public kbResource {

//---------------------------------------------------------------------------------------------------
public:
												kbWaveFile();
												~kbWaveFile();

	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_SOUNDWAVE; }

	BYTE *										GetWaveData() const { return m_pWaveDataBuffer; }
	DWORD										GetWaveSize() const { return m_cbWaveSize; }

	WAVEFORMATEX *								GetFormat() { return m_pWaveFormat; }

private:

	virtual bool								Load_Internal();
	virtual void								Release_Internal();

	HRESULT										ReadMMIO();

    HRESULT										Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead );
    HRESULT										ResetFile();

    WAVEFORMATEX *								m_pWaveFormat;
    HMMIO										m_hMMio;
    MMCKINFO									m_ck;
    MMCKINFO									m_ckRiff;
    DWORD										m_dwSize;
	BYTE*										m_pWaveDataBuffer;
	DWORD										m_cbWaveSize;
};

/**
 *	kbSoundManager
 */
class kbSoundManager {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbSoundManager();
												~kbSoundManager();

	void										PlayWave( kbWaveFile *const pWaveFile, const float volume );

	void										Update();

	void										SetFrequencyRatio( const float frequencyRatio );

	static float								GetMasterVolume() { return s_MasterVolume; }
	static void									SetMasterVolume( const float newVolume ) { s_MasterVolume = newVolume; }

private:

	IXAudio2 *									m_pXAudioEngine;
	IXAudio2MasteringVoice *					m_pMasteringVoice;

	float										m_FrequencyRatio;

	static const int							MAX_VOICES = 32;
	struct kbVoiceData_t {
												kbVoiceData_t() : m_pVoice( nullptr ), m_bInUse( false ) { }

		IXAudio2SourceVoice *					m_pVoice;
		bool									m_bInUse;
	}											m_Voices[MAX_VOICES];

	bool										m_bInitialized;

	inline static float							s_MasterVolume = 1.0f;
};


#endif