/// kbParticleComponent.h
///
/// 2016-2025 blk 1.0
#pragma once
#include "kbModel.h"

enum EBillboardType {
	BT_FaceCamera,
	BT_AxialBillboard,
	BT_AlignAlongVelocity
};

struct kbParticle_t {
	kbParticle_t();
	kbParticle_t(const kbParticle_t&) = default;
	~kbParticle_t();

	kbParticle_t& operator=(const kbParticle_t&) = default;

	void Shutdown();

	Vec3 m_position;
	float m_Rotation;
	Vec3 m_StartSize;
	Vec3 m_EndSize;
	float m_LifeLeft;
	float m_TotalLife;
	Vec3 m_StartVelocity;
	Vec3 m_EndVelocity;
	float m_StartRotation;
	float m_EndRotation;
	float m_Randoms[3];
	Vec3 m_rotation_axis;

	class kbModelEmitter* m_pSrcModelEmitter;

	int m_CurrentModelIndex;
	kbRenderObject m_render_object;		// For model emitters
};

/// kbModelEmitter
class kbModelEmitter : public kbGameComponent {

	KB_DECLARE_COMPONENT(kbModelEmitter, kbGameComponent);

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	const kbModel* model() const { return m_model; }
	const std::vector<kbShaderParamOverrides_t>					GetShaderParamOverrides() const { return m_ShaderParams; }

	void														Init();

private:
	kbModel* m_model;
	std::vector<kbMaterialComponent>							m_materials;

	std::vector<kbShaderParamOverrides_t>						m_ShaderParams;
};


/// kbParticleComponent
class kbParticleComponent : public RenderComponent {
	KB_DECLARE_COMPONENT(kbParticleComponent, RenderComponent);

public:
	virtual	~kbParticleComponent();

	virtual void editor_change(const std::string& propertyName);

	virtual void RenderSync();

	void StopParticleSystem();

	void EnableNewSpawns(const bool bEnable);

	// Hack wasn't picking up from the package file
	void SetBillboardType(const EBillboardType inBBType) { m_ParticleBillboardType = inBBType; }

	bool IsModelEmitter() const {return m_ModelEmitter.size() > 0 && m_ModelEmitter[0].model() != nullptr; }

	const kbModel* get_model() const {
		if (m_buffer_to_render != -1) {
			return &m_models[m_buffer_to_render];
		} else {
			return nullptr;
		}
	}

protected:
	virtual void enable_internal(const bool isEnabled) override;
	virtual void update_internal(const float DeltaTime) override;

private:
	// Editable
	std::vector<kbMaterialComponent> m_materials;
	f32 m_TotalDuration;
	int	m_MaxParticlesToEmit;
	f32 m_StartDelay;
	f32 m_MinParticleSpawnRate;				// Particles per second
	f32 m_MaxParticleSpawnRate;				// Particles per second
	Vec3 m_MinParticleStartVelocity;
	Vec3 m_MaxParticleStartVelocity;
	std::vector<kbAnimEvent> m_velocityOverLifeTimeCurve;
	Vec3 m_MinParticleEndVelocity;
	Vec3 m_MaxParticleEndVelocity;
	f32 m_MinStartRotationRate;
	f32 m_MaxStartRotationRate;
	f32 m_MinEndRotationRate;
	f32 m_MaxEndRotationRate;
	Vec3 m_MinStart3DRotation;
	Vec3 m_MaxStart3DRotation;
	Vec3 m_MinStart3DOffset;
	Vec3 m_MaxStart3DOffset;
	Vec3 m_MinParticleStartSize;
	Vec3 m_MaxParticleStartSize;
	Vec3 m_MinParticleEndSize;
	Vec3 m_MaxParticleEndSize;
	f32 m_ParticleMinDuration;
	f32 m_ParticleMaxDuration;
	Vec4 m_ParticleStartColor;
	Vec4 m_ParticleEndColor;
	std::vector<kbVectorAnimEvent> m_SizeOverLifeTimeCurve;
	std::vector<kbVectorAnimEvent> m_RotationOverLifeTimeCurve;
	std::vector<kbVectorAnimEvent> m_ColorOverLifeTimeCurve;
	std::vector<kbAnimEvent> m_AlphaOverLifeTimeCurve;
	Vec3 m_gravity;
	int	m_MinBurstCount;
	int	m_MaxBurstCount;
	EBillboardType m_ParticleBillboardType;
	std::vector<kbModelEmitter>	m_ModelEmitter;
	f32	m_render_order_bias;
	bool m_DebugPlayEntity;

	// Non-editable
	f32 m_LeftOverTime;
	f32	m_TimeAlive;
	int	m_BurstCount;
	f32	m_StartDelayRemaining;
	int	m_NumEmittedParticles;

	kbRenderObject m_render_object;
	std::vector<kbParticle_t> m_Particles;

	// Dx12
	static const int NumParticleBuffers = 3;
	kbModel m_models[NumParticleBuffers];
	ParticleVertex* m_vertex_buffer;
	u16* m_index_buffer;
	//

	u32 m_buffer_to_fill;
	u32 m_buffer_to_render;

	friend class kbParticleManager;
	const kbParticleComponent* m_ParticleTemplate;
	bool m_bIsPooled;
	bool m_bIsSpawning;
};
