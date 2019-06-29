//==============================================================================
// kbSoundManager.cpp
//
//
// 2017-2019 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbSoundManager.h"


/**
 *	kbWave::kbWave
 */
kbWaveFile::kbWaveFile() :
	m_pWaveFormat( nullptr ),
	m_hMMio( nullptr ),
	m_dwSize( 0 ),
	m_pWaveDataBuffer( nullptr ),
	m_cbWaveSize( 0 ) {
}

/**
 *	kbWaveFile::~kbWaveFile
 */
kbWaveFile::~kbWaveFile() {
}

/**
 *	kbWaveFile::Load_Internal
 */
bool kbWaveFile::Load_Internal() {
	HRESULT hr;

	const LPSTR pFileName = (LPSTR)GetFullFileName().c_str();
	m_hMMio = mmioOpen( pFileName, nullptr, MMIO_ALLOCBUF | MMIO_READ );
	
	hr = ReadMMIO();
	kbErrorCheck( SUCCEEDED( hr ), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str() ); 

	hr = ResetFile();
	kbErrorCheck( SUCCEEDED( hr ), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str() ); 
	
	// After the reset, the size of the wav file is m_ck.cksize so store it now
	m_dwSize = m_ck.cksize;
	    
	// Read the sample data into memory
	m_cbWaveSize = m_dwSize;
    m_pWaveDataBuffer = new BYTE[m_cbWaveSize] ;

	hr = Read( m_pWaveDataBuffer, m_cbWaveSize, &m_cbWaveSize );
	kbErrorCheck( SUCCEEDED( hr ), "kbWaveFile::Load_Internal() - Failed to load wave %s", GetFullFileName().c_str() ); 

	return true;
}

/**
 *	kbWaveFile::Release_Internal
 */
void kbWaveFile::Release_Internal() {
	if ( m_hMMio != nullptr ) {
		mmioClose( m_hMMio, 0 );
		m_hMMio = nullptr;
	}

	delete[] m_pWaveDataBuffer;
	m_pWaveDataBuffer = nullptr;
}

/**
 *	kbWaveFile::ReadMMIO
 */
HRESULT	kbWaveFile::ReadMMIO() {
    MMCKINFO ckIn;           // chunk info. for general use.
    PCMWAVEFORMAT pcmWaveFormat;  // Temp PCM structure to load in.

    memset( &ckIn, 0, sizeof( ckIn ) );

    m_pWaveFormat = nullptr;

	MMRESULT MR = mmioDescend( m_hMMio, &m_ckRiff, NULL, 0 );

	kbErrorCheck( MR == 0, "kbWaveFile::ReadMMIO() - Error" );
	kbErrorCheck( m_ckRiff.ckid == FOURCC_RIFF && m_ckRiff.fccType  == mmioFOURCC( 'W', 'A', 'V', 'E' ), "kbWaveFile::ReadMMIO() - Error" );

    // Search the input file for for the 'fmt ' chunk.
    ckIn.ckid = mmioFOURCC( 'f', 'm', 't', ' ' );

	MR = mmioDescend( m_hMMio, &ckIn, &m_ckRiff, MMIO_FINDCHUNK );
	kbErrorCheck( MR == 0, "kbWaveFile::ReadMMIO() - Error" );

    // Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    // if there are extra parameters at the end, we'll ignore them
	kbErrorCheck( ckIn.cksize >= ( LONG )sizeof( PCMWAVEFORMAT ), "kbWaveFile::ReadMMIO() - Error" );

	LONG amtRead = mmioRead( m_hMMio, ( HPSTR )&pcmWaveFormat, sizeof( pcmWaveFormat ) );
	kbErrorCheck( amtRead == sizeof( pcmWaveFormat ), "kbWaveFile::ReadMMIO() - Error" );

    // Allocate the waveformatex, but if its not pcm format, read the next
    // word, and thats how many extra bytes to allocate.
    if( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM ) {
        m_pWaveFormat = reinterpret_cast< WAVEFORMATEX* >( new CHAR[ sizeof( WAVEFORMATEX ) ] );
		kbErrorCheck( m_pWaveFormat != nullptr, "kbWaveFile::ReadMMIO() - Error" );

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( m_pWaveFormat, &pcmWaveFormat, sizeof( pcmWaveFormat ) );
        m_pWaveFormat->cbSize = 0;
    }
    else
    {
        // Read in length of extra bytes.
        WORD cbExtraBytes = 0L;
		amtRead = mmioRead( m_hMMio, ( CHAR* )&cbExtraBytes, sizeof( WORD ) );
		kbErrorCheck( amtRead == sizeof( WORD ), "kbWaveFile::ReadMMIO() - Error" );

        m_pWaveFormat = reinterpret_cast< WAVEFORMATEX* >(new CHAR[ sizeof( WAVEFORMATEX ) + cbExtraBytes ] );
		kbErrorCheck( m_pWaveFormat != nullptr, "kbWaveFile::ReadMMIO() - Error" );

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( m_pWaveFormat, &pcmWaveFormat, sizeof( pcmWaveFormat ) );
        m_pWaveFormat->cbSize = cbExtraBytes;

        // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
		amtRead = mmioRead( m_hMMio, ( CHAR* )( ( ( BYTE* )&( m_pWaveFormat->cbSize ) ) + sizeof( WORD ) ), cbExtraBytes );
		kbErrorCheck( amtRead == cbExtraBytes, "kbWaveFile::ReadMMIO() - Error" );
    }

	MR = mmioAscend( m_hMMio, &ckIn, 0 );
	kbErrorCheck( MR == 0, "kbWaveFile::ReadMMIO() - Error" );

    return S_OK;
}

/**
 *	kbWaveFile::Read
 */
HRESULT kbWaveFile::Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead ) {

	MMIOINFO mmioinfoIn; // current status of m_hMMio
	
	if( m_hMMio == nullptr ) {
	    return CO_E_NOTINITIALIZED;
	}

	kbErrorCheck( pBuffer != nullptr && pdwSizeRead != nullptr, "kbWaveFile::Read() - Error" );

	*pdwSizeRead = 0;
	
	MMRESULT MR = mmioGetInfo( m_hMMio, &mmioinfoIn, 0 );
	kbErrorCheck( MR == 0, "kbWaveFile::Read() - Error" );

	
	UINT cbDataIn = dwSizeToRead;
	if( cbDataIn > m_ck.cksize ) {
	    cbDataIn = m_ck.cksize;
	}

	m_ck.cksize -= cbDataIn;
	
	for( DWORD cT = 0; cT < cbDataIn; cT++ ) {
	    // Copy the bytes from the io to the buffer.
	    if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
	    {
			MR = mmioAdvance( m_hMMio, &mmioinfoIn, MMIO_READ );
			kbErrorCheck( MR == 0, "kbWaveFile::Read() - Error" );
			kbErrorCheck( mmioinfoIn.pchNext != mmioinfoIn.pchEndRead, "kbWaveFile::Read() - Error" );
	    }
	
	    // Actual copy.
	    *( ( BYTE* )pBuffer + cT ) = *( ( BYTE* )mmioinfoIn.pchNext );
	    mmioinfoIn.pchNext++;
	}

	MR = mmioSetInfo( m_hMMio, &mmioinfoIn, 0 );
	kbErrorCheck( MR == 0, "kbWaveFile::Read() - Error" );
	
	*pdwSizeRead = cbDataIn;
	
	return S_OK;
	
}

/**
 *	kbWaveFile::ResetFile
 */
HRESULT kbWaveFile::ResetFile() {

	if( m_hMMio == nullptr ) {
		return CO_E_NOTINITIALIZED;
	}

    // Seek to the data
	LONG MR = mmioSeek( m_hMMio, m_ckRiff.dwDataOffset + sizeof( FOURCC ), SEEK_SET );
	kbErrorCheck( MR != -1, "kbWaveFile::ResetFile()" );

    // Search the input file for the 'data' chunk.
    m_ck.ckid = mmioFOURCC( 'd', 'a', 't', 'a' );
	MR = mmioDescend( m_hMMio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK );
	kbErrorCheck( MR == 0, "kbWaveFile::ResetFile()" );

    return S_OK;
}

/**
 *	kbSoundManager::kbSoundManager
 */
kbSoundManager::kbSoundManager() {

	kbLog( "Creating Audio Engine" );

	m_bInitialized = false;

	HRESULT hr = XAudio2Create( &m_pXAudioEngine );
	if ( FAILED( hr ) ) {
		kbWarning( "kbSoundManager::kbSoundManager() - Failed to create XAudio2" );
		return;
	}

	hr = m_pXAudioEngine->CreateMasteringVoice( &m_pMasteringVoice );
	if ( FAILED( hr ) ) {
		kbWarning( "kbSoundManager::kbSoundManager() - Failed to create a mastering voice" );
		return;
	}

	m_FrequencyRatio = 1.0f;
	m_bInitialized = true;

	//memset( m_Voices, NULL, sizeof( voices ) );
//	memset( voicesInUse, false, sizeof( voicesInUse ) );
}

/**
 *	kbSoundManager::~kbSoundManager
 */						
kbSoundManager::~kbSoundManager() {

	for ( int iVoice = 0; iVoice < MAX_VOICES; iVoice++ ) {
		if ( m_Voices[iVoice].m_bInUse == false ) {
			continue;
		}

		m_Voices[iVoice].m_pVoice->DestroyVoice();
		m_Voices[iVoice].m_pVoice = nullptr;
		m_Voices[iVoice].m_bInUse = false;
	}

	m_pMasteringVoice->DestroyVoice();
	m_pXAudioEngine->Release();
	CoUninitialize();

	kbLog( "Audio Engine destroyed");
}

/**
 *	kbSoundManager::PlayWave
 */
void kbSoundManager::PlayWave( kbWaveFile *const pWaveFile, const float inVolume ) {

	if ( m_bInitialized == false ) {
		return;
	}

	kbVoiceData_t * pVoice = nullptr;

	for ( int iVoice = 0; iVoice < MAX_VOICES; iVoice++ ) {
		if ( m_Voices[iVoice].m_bInUse == true ) {
			continue;
		}
		kbErrorCheck( m_Voices[iVoice].m_pVoice == nullptr, "kbSoundManager::PlayWave() - Non null voice is in use." );

		pVoice = &m_Voices[iVoice];
		pVoice->m_bInUse = true;

		// Create the source voice
		WAVEFORMATEX *const pwfx = pWaveFile->GetFormat();
		HRESULT hr = m_pXAudioEngine->CreateSourceVoice( &pVoice->m_pVoice, pwfx );
		kbErrorCheck( SUCCEEDED( hr ), "kbSoundManager::PlayWave() - Failed to create a voice" );

		break;
	}

	if ( pVoice == nullptr ) {
		return;
	}

	// Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = pWaveFile->GetWaveData();//files[index].m_pWaveDataBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = pWaveFile->GetWaveSize();//files[index].m_cbWaveSize;

	HRESULT hr = pVoice->m_pVoice->SubmitSourceBuffer( &buffer );
	kbErrorCheck( SUCCEEDED( hr ), "kbSoundManager::PlayWave() - Failed to submit audio buffer" );

	pVoice->m_pVoice->SetVolume( inVolume );

	pVoice->m_pVoice->SetFrequencyRatio( m_FrequencyRatio );
	hr = pVoice->m_pVoice->Start( 0 );
	kbErrorCheck( SUCCEEDED( hr ), "kbSoundManager::PlayWave() - Failed to submit start voice" );
}

/**
 *	kbSoundManager::Update
 */
void kbSoundManager::Update() {

	if ( m_bInitialized == false ) {
		return;
	}

	for ( int iVoice = 0; iVoice < MAX_VOICES; iVoice++ ) {
		if ( m_Voices[iVoice].m_bInUse == false ) {
			continue;
		}

		XAUDIO2_VOICE_STATE state;
		m_Voices[iVoice].m_pVoice->GetState( &state );

		if ( state.BuffersQueued <= 0 ) {
			m_Voices[iVoice].m_pVoice->Stop();
			m_Voices[iVoice].m_bInUse = false;
			m_Voices[iVoice].m_pVoice->DestroyVoice();
			m_Voices[iVoice].m_pVoice = nullptr;
		}
	}
}

/**
 *	kbSoundManager::SetFrequencyRatio
 */
void kbSoundManager::SetFrequencyRatio( const float frequencyRatio ) {

	if ( m_bInitialized == false ) {
		return;
	}

	m_FrequencyRatio = frequencyRatio;

	for ( int iVoice = 0; iVoice < MAX_VOICES; iVoice++ ) {
		if ( m_Voices[iVoice].m_bInUse == false ) {
			continue;
		}
		m_Voices[iVoice].m_pVoice->SetFrequencyRatio( m_FrequencyRatio );
	}
}