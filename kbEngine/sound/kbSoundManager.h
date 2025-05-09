/// kbSoundManager.h
///
//// 2017-2025 blk 1.0

#pragma once

#include <array>
#include <mmsystem.h>
#include <mmreg.h>
#include <XAudio2.h>
#include "kbResourceManager.h"

/// kbWaveFile
class kbWaveFile : public kbResource {
public:
	kbWaveFile();
	~kbWaveFile();

	virtual kbTypeInfoType_t GetType() const { return KBTYPEINFO_SOUNDWAVE; }

	BYTE* GetWaveData() const { return m_pWaveDataBuffer; }
	DWORD GetWaveSize() const { return m_cbWaveSize; }

	WAVEFORMATEX* GetFormat() { return m_pWaveFormat; }

private:
	virtual bool Load_Internal();
	virtual void Release_Internal();

	HRESULT	ReadMMIO();

	HRESULT	Read(BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead);
	HRESULT	ResetFile();

	WAVEFORMATEX* m_pWaveFormat;
	HMMIO m_hMMio;
	MMCKINFO m_ck;
	MMCKINFO m_ckRiff;
	DWORD m_dwSize;
	DWORD m_cbWaveSize;
	BYTE* m_pWaveDataBuffer;
};

/// kbSoundManager
class kbSoundManager {
public:
	kbSoundManager();
	~kbSoundManager();

	int	PlayWave(kbWaveFile* const pWaveFile, const float volume, const bool bLoop = false);
	void StopWave(const int id);

	void Update();

	void SetFrequencyRatio(const float frequencyRatio);

	float GetMasterVolume() const { return m_MasterVolume; }
	void SetMasterVolume(const float newVolume);

private:
	IXAudio2* m_pXAudioEngine;
	IXAudio2MasteringVoice* m_pMasteringVoice;

	float m_FrequencyRatio;

	static const int MAX_VOICES = 32;
	struct kbVoiceData_t {
		kbVoiceData_t() :
			m_pVoice(nullptr),
			m_bInUse(false) {}

		IXAudio2SourceVoice* m_pVoice;
		bool m_bInUse;
	};
	std::array<kbVoiceData_t, MAX_VOICES> m_Voices;

	float m_MasterVolume;
	bool m_bInitialized;
};
