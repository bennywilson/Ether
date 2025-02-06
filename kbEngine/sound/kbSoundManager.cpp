/// kbSoundComponent.cpp
///
/// 2017-2025 blk 1.0

#include "kbCore.h"
#include "kbSoundManager.h"
#include "kbGameEntityHeader.h"
#include "kbLevelComponent.h"

/// kbWaveFile::kbWaveFile
kbWaveFile::kbWaveFile() :
	m_pWaveFormat(nullptr),
	m_hMMio(nullptr),
	m_dwSize(0),
	m_pWaveDataBuffer(nullptr),
	m_cbWaveSize(0) {
}

/// kbWaveFile::~kbWaveFile
kbWaveFile::~kbWaveFile() {}

/// kbWaveFile::Load_Internal
bool kbWaveFile::Load_Internal() {
	HRESULT hr;

	const LPSTR pFileName = (LPSTR)GetFullFileName().c_str();
	m_hMMio = mmioOpen(pFileName, nullptr, MMIO_ALLOCBUF | MMIO_READ);

	hr = ReadMMIO();
	blk::error_check(SUCCEEDED(hr), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str());

	hr = ResetFile();
	blk::error_check(SUCCEEDED(hr), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str());

	// After the reset, the size of the wav file is m_ck.cksize so store it now
	m_dwSize = m_ck.cksize;

	// Read the sample data into memory
	m_cbWaveSize = m_dwSize;
	m_pWaveDataBuffer = new BYTE[m_cbWaveSize];

	hr = Read(m_pWaveDataBuffer, m_cbWaveSize, &m_cbWaveSize);
	blk::error_check(SUCCEEDED(hr), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str());

	return true;
}

/// kbWaveFile::Release_Internal
void kbWaveFile::Release_Internal() {
	if (m_hMMio != nullptr) {
		mmioClose(m_hMMio, 0);
		m_hMMio = nullptr;
	}

	delete[] m_pWaveDataBuffer;
	m_pWaveDataBuffer = nullptr;
}

/// kbWaveFile::ReadMMIO
HRESULT	kbWaveFile::ReadMMIO() {
	MMCKINFO ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT pcmWaveFormat;  // Temp PCM structure to load in.

	memset(&ckIn, 0, sizeof(ckIn));

	m_pWaveFormat = nullptr;

	MMRESULT MR = mmioDescend(m_hMMio, &m_ckRiff, NULL, 0);

	blk::error_check(MR == 0, "kbWaveFile::ReadMMIO() - Error");
	blk::error_check(m_ckRiff.ckid == FOURCC_RIFF && m_ckRiff.fccType == mmioFOURCC('W', 'A', 'V', 'E'), "kbWaveFile::ReadMMIO() - Error");

	// Search the input file for for the 'fmt ' chunk.
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');

	MR = mmioDescend(m_hMMio, &ckIn, &m_ckRiff, MMIO_FINDCHUNK);
	blk::error_check(MR == 0, "kbWaveFile::ReadMMIO() - Error");

	// Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
	// if there are extra parameters at the end, we'll ignore them
	blk::error_check(ckIn.cksize >= (LONG)sizeof(PCMWAVEFORMAT), "kbWaveFile::ReadMMIO() - Error");

	LONG amtRead = mmioRead(m_hMMio, (HPSTR)&pcmWaveFormat, sizeof(pcmWaveFormat));
	blk::error_check(amtRead == sizeof(pcmWaveFormat), "kbWaveFile::ReadMMIO() - Error");

	// Allocate the waveformatex, but if its not pcm format, read the next
	// word, and thats how many extra bytes to allocate.
	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM) {
		m_pWaveFormat = reinterpret_cast<WAVEFORMATEX*>(new CHAR[sizeof(WAVEFORMATEX)]);
		blk::error_check(m_pWaveFormat != nullptr, "kbWaveFile::ReadMMIO() - Error");

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy(m_pWaveFormat, &pcmWaveFormat, sizeof(pcmWaveFormat));
		m_pWaveFormat->cbSize = 0;
	}
	else
	{
		// Read in length of extra bytes.
		WORD cbExtraBytes = 0L;
		amtRead = mmioRead(m_hMMio, (CHAR*)&cbExtraBytes, sizeof(WORD));
		blk::error_check(amtRead == sizeof(WORD), "kbWaveFile::ReadMMIO() - Error");

		m_pWaveFormat = reinterpret_cast<WAVEFORMATEX*>(new CHAR[sizeof(WAVEFORMATEX) + cbExtraBytes]);
		blk::error_check(m_pWaveFormat != nullptr, "kbWaveFile::ReadMMIO() - Error");

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy(m_pWaveFormat, &pcmWaveFormat, sizeof(pcmWaveFormat));
		m_pWaveFormat->cbSize = cbExtraBytes;

		// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
		amtRead = mmioRead(m_hMMio, (CHAR*)(((BYTE*)&(m_pWaveFormat->cbSize)) + sizeof(WORD)), cbExtraBytes);
		blk::error_check(amtRead == cbExtraBytes, "kbWaveFile::ReadMMIO() - Error");
	}

	MR = mmioAscend(m_hMMio, &ckIn, 0);
	blk::error_check(MR == 0, "kbWaveFile::ReadMMIO() - Error");

	return S_OK;
}

/// kbWaveFile::Read
HRESULT kbWaveFile::Read(BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead) {

	MMIOINFO mmioinfoIn; // current status of m_hMMio

	if (m_hMMio == nullptr) {
		return CO_E_NOTINITIALIZED;
	}

	blk::error_check(pBuffer != nullptr && pdwSizeRead != nullptr, "kbWaveFile::Read() - Error");

	*pdwSizeRead = 0;

	MMRESULT MR = mmioGetInfo(m_hMMio, &mmioinfoIn, 0);
	blk::error_check(MR == 0, "kbWaveFile::Read() - Error");


	UINT cbDataIn = dwSizeToRead;
	if (cbDataIn > m_ck.cksize) {
		cbDataIn = m_ck.cksize;
	}

	m_ck.cksize -= cbDataIn;

	for (DWORD cT = 0; cT < cbDataIn; cT++) {
		// Copy the bytes from the io to the buffer.
		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
		{
			MR = mmioAdvance(m_hMMio, &mmioinfoIn, MMIO_READ);
			blk::error_check(MR == 0, "kbWaveFile::Read() - Error");
			blk::error_check(mmioinfoIn.pchNext != mmioinfoIn.pchEndRead, "kbWaveFile::Read() - Error");
		}

		// Actual copy.
		*((BYTE*)pBuffer + cT) = *((BYTE*)mmioinfoIn.pchNext);
		mmioinfoIn.pchNext++;
	}

	MR = mmioSetInfo(m_hMMio, &mmioinfoIn, 0);
	blk::error_check(MR == 0, "kbWaveFile::Read() - Error");

	*pdwSizeRead = cbDataIn;

	return S_OK;

}

/// kbWaveFile::ResetFile
HRESULT kbWaveFile::ResetFile() {

	if (m_hMMio == nullptr) {
		return CO_E_NOTINITIALIZED;
	}

	// Seek to the data
	LONG MR = mmioSeek(m_hMMio, m_ckRiff.dwDataOffset + sizeof(FOURCC), SEEK_SET);
	blk::error_check(MR != -1, "kbWaveFile::ResetFile()");

	// Search the input file for the 'data' chunk.
	m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
	MR = mmioDescend(m_hMMio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK);
	blk::error_check(MR == 0, "kbWaveFile::ResetFile()");

	return S_OK;
}

/// kbSoundManager::kbSoundManager
kbSoundManager::kbSoundManager() {
	blk::log("Creating Audio Engine");

	m_bInitialized = false;

	HRESULT hr = XAudio2Create(&m_pXAudioEngine);
	if (FAILED(hr)) {
		blk::warning("kbSoundManager::kbSoundManager() - Failed to create XAudio2");
		return;
	}

	hr = m_pXAudioEngine->CreateMasteringVoice(&m_pMasteringVoice);

	if (FAILED(hr)) {
		blk::warning("kbSoundManager::kbSoundManager() - Failed to create a mastering voice");
		return;
	}

	m_FrequencyRatio = 1.0f;
	m_bInitialized = true;

	m_MasterVolume = 1.0f;
}

/// kbSoundManager::~kbSoundManager
kbSoundManager::~kbSoundManager() {
	if (m_bInitialized == false) {
		return;
	}

	for (int iVoice = 0; iVoice < MAX_VOICES; iVoice++) {
		if (m_Voices[iVoice].m_bInUse == false) {
			continue;
		}

		m_Voices[iVoice].m_pVoice->DestroyVoice();
		m_Voices[iVoice].m_pVoice = nullptr;
		m_Voices[iVoice].m_bInUse = false;
	}

	m_pMasteringVoice->DestroyVoice();
	m_pMasteringVoice = nullptr;
	m_pXAudioEngine->Release();
	CoUninitialize();

	blk::log("Audio Engine destroyed");
}

/// kbSoundManager::PlayWave
int kbSoundManager::PlayWave(kbWaveFile* const pWaveFile, const float inVolume, const bool bLoop) {
	if (m_bInitialized == false) {
		return -1;
	}

	kbVoiceData_t* pVoice = nullptr;

	const float finalVolume = inVolume * kbLevelComponent::GetGlobalVolumeScale();
	int iVoice = 0;
	for (iVoice = 0; iVoice < MAX_VOICES; iVoice++) {
		if (m_Voices[iVoice].m_bInUse == true) {
			continue;
		}
		blk::error_check(m_Voices[iVoice].m_pVoice == nullptr, "kbSoundManager::PlayWave() - Non null voice is in use.");

		pVoice = &m_Voices[iVoice];
		pVoice->m_bInUse = true;

		// Create the source voice
		WAVEFORMATEX* const pwfx = pWaveFile->GetFormat();
		HRESULT hr = m_pXAudioEngine->CreateSourceVoice(&pVoice->m_pVoice, pwfx);
		blk::error_check(SUCCEEDED(hr), "kbSoundManager::PlayWave() - Failed to create a voice");

		break;
	}

	if (pVoice == nullptr) {
		return -1;
	}

	// Submit the wave sample data using an XAUDIO2_BUFFER structure
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.pAudioData = pWaveFile->GetWaveData();//files[index].m_pWaveDataBuffer;
	buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
	buffer.AudioBytes = pWaveFile->GetWaveSize();//files[index].m_cbWaveSize;

	if (bLoop) {
		buffer.LoopBegin = 0;
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	HRESULT hr = pVoice->m_pVoice->SubmitSourceBuffer(&buffer);
	blk::error_check(SUCCEEDED(hr), "kbSoundManager::PlayWave() - Failed to submit audio buffer");

	pVoice->m_pVoice->SetVolume(finalVolume);
	pVoice->m_pVoice->SetFrequencyRatio(m_FrequencyRatio);
	hr = pVoice->m_pVoice->Start(0);
	blk::error_check(SUCCEEDED(hr), "kbSoundManager::PlayWave() - Failed to submit start voice");

	return iVoice;
}

/// kbSoundManager::StopWave
void kbSoundManager::StopWave(const int id) {

	blk::error_check(id >= 0 && id < MAX_VOICES, "kbSoundManager::StopWave() - Called with invalid wave id");

	kbVoiceData_t* const pVoice = &m_Voices[id];
	pVoice->m_pVoice->Stop();
	pVoice->m_bInUse = false;
	pVoice->m_pVoice->DestroyVoice();
	pVoice->m_pVoice = nullptr;
}

/// kbSoundManager::Update
void kbSoundManager::Update() {
	if (m_bInitialized == false) {
		return;
	}

	for (int iVoice = 0; iVoice < MAX_VOICES; iVoice++) {
		if (m_Voices[iVoice].m_bInUse == false) {
			continue;
		}

		XAUDIO2_VOICE_STATE state;
		m_Voices[iVoice].m_pVoice->GetState(&state);

		if (state.BuffersQueued <= 0) {
			m_Voices[iVoice].m_pVoice->Stop();
			m_Voices[iVoice].m_bInUse = false;
			m_Voices[iVoice].m_pVoice->DestroyVoice();
			m_Voices[iVoice].m_pVoice = nullptr;
		}
	}
}

/// kbSoundManager::SetFrequencyRatio
void kbSoundManager::SetFrequencyRatio(const float frequencyRatio) {

	if (m_bInitialized == false) {
		return;
	}

	m_FrequencyRatio = frequencyRatio;

	for (int iVoice = 0; iVoice < MAX_VOICES; iVoice++) {
		if (m_Voices[iVoice].m_bInUse == false) {
			continue;
		}
		m_Voices[iVoice].m_pVoice->SetFrequencyRatio(m_FrequencyRatio);
	}
}

/// kbSoundManager::SetMasterVolume
void kbSoundManager::SetMasterVolume(const float newVolume) {
	m_MasterVolume = newVolume;
	m_pMasteringVoice->SetVolume(newVolume);
}