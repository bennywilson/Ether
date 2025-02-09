/// kbSoundComponent.h
///
/// 2017-2025 blk 1.0

#pragma once

/// kbSoundData
class kbSoundData : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbSoundData, kbGameComponent);

public:
	virtual	~kbSoundData();

	void PlaySoundAtPosition(const Vec3& soundPosition);
	void StopSound();

	virtual void editor_change(const std::string& propertyName) override;

private:
	// Data
	class kbWaveFile* m_pWaveFile;
	float m_Radius;
	float m_Volume;

	bool m_bLooping;
	bool m_bDebugPlaySound;

	// Runtime
	int m_SoundId;
};

/// kbPlaySoundComponent
class kbPlaySoundComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbPlaySoundComponent, kbGameComponent);

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

private:
	// Data
	float m_MinStartDelay;
	float m_MaxStartDelay;
	std::vector<kbSoundData> m_SoundData;

	// Runtime
	float m_TimeToPlay;
};

/// PlayRandomSound
inline void PlayRandomSound(std::vector<kbSoundData>& soundData, const Vec3& pos = Vec3::zero) {
	if (soundData.empty()) {
		return;
	}

	soundData[rand() % soundData.size()].PlaySoundAtPosition(pos);
}
