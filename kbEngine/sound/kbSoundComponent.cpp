/// kbSoundComponent.cpp
///
/// 2017-2025 blk 1.0

#include "blk_core.h"
#include "Quaternion.h"
#include "kbGame.h"
#include "kbSoundComponent.h"
#include "kbRenderer.h"

///	kbSoundData::Constructor
void kbSoundData::Constructor() {
	m_pWaveFile = nullptr;
	m_Radius = -1.0f;
	m_Volume = 1.0f;

	m_bDebugPlaySound = false;
	m_bLooping = false;

	m_SoundId = -1;
}

///	kbSoundData::~kbSoundData
kbSoundData::~kbSoundData() {
	StopSound();
}

/// kbSoundData::PlaySoundAtPosition
void kbSoundData::PlaySoundAtPosition(const Vec3& soundPosition) {
	//blk::error("Needs reimplementation");

	Vec3 currentCameraPosition;
	Quat4 currentCameraRotation;

	g_pRenderer->GetRenderViewTransform(nullptr, currentCameraPosition, currentCameraRotation);

	const float distToCamera = (currentCameraPosition - soundPosition).length();
	float atten = 1.0f;
	if (m_Radius > 0.0f) {
		if (distToCamera > m_Radius) {
			return;
		}
		else {
			atten = 1.0f - (distToCamera / m_Radius);
		}
	}

	const int waveId = g_pGame->GetSoundManager().PlayWave(m_pWaveFile, atten * m_Volume, m_bLooping);
	if (m_bLooping) {
		m_SoundId = waveId;
	}
}

/// kbSoundData::StopSound
void kbSoundData::StopSound() {
	if (m_SoundId != -1) {
		g_pGame->GetSoundManager().StopWave(m_SoundId);
	}
	m_SoundId = -1;
}

/// kbSoundData::EditorChange
void kbSoundData::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "TestPlaySoundNow") {
		m_bDebugPlaySound = false;
		g_pGame->GetSoundManager().PlayWave(m_pWaveFile, m_Volume * m_Volume);
	}
}

/// kbPlaySoundComponent::Constructor
void kbPlaySoundComponent::Constructor() {
	m_MinStartDelay = 0.0f;
	m_MaxStartDelay = 0.0f;
	m_TimeToPlay = 0.0f;
}

/// kbPlaySoundComponent::enable_internal
void kbPlaySoundComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (bEnable) {
		const float delay = kbfrand(m_MinStartDelay, m_MaxStartDelay);
		m_TimeToPlay = g_GlobalTimer.TimeElapsedSeconds() + delay;
	}
}

/// kbPlaySoundComponent::update_internal
void kbPlaySoundComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (g_UseEditor == true) {
		return;
	}

	if (g_GlobalTimer.TimeElapsedSeconds() >= m_TimeToPlay) {
		if (m_SoundData.size() > 0) {
			m_SoundData[rand() % m_SoundData.size()].PlaySoundAtPosition(owner_position());
		}

		Enable(false);
	}
}