//===================================================================================================
// kbParticleComponent.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBPARTICLECOMPONENT_H_
#define _KBPARTICLECOMPONENT_H_

#include "kbModel.h"

enum EBillboardType {
	BT_FaceCamera,
	BT_AxialBillboard,
	BT_AlignAlongVelocity
};

struct kbParticle_t {
	kbVec3 m_Position;
	float m_Rotation;
	kbVec2 m_StartSize;
	kbVec2 m_EndSize;
	float m_LifeLeft;
	float m_TotalLife;
	kbVec3 m_StartVelocity;
	kbVec3 m_EndVelocity;
	float m_StartRotation;
	float m_EndRotation;
	float m_Randoms[3];
};

/**
 *	kbParticleComponent
 */
class kbParticleComponent : public kbTransformComponent {

	KB_DECLARE_COMPONENT( kbParticleComponent, kbTransformComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual														~kbParticleComponent();

	virtual void												EditorChange( const std::string & propertyName );

	virtual void												RenderSync();

	void														StopParticleSystem();
	void														StopNewSpawns() { m_bIsSpawning = false; }

	// Hack wasn't picking up from the package file
	void														SetBillboardType( const EBillboardType inBBType ) { m_ParticleBillboardType = inBBType; }

protected:

	virtual void												SetEnable_Internal( const bool isEnabled ) override;
	virtual void												Update_Internal( const float DeltaTime ) override;

private:

	// Editable
	std::vector<kbMaterialComponent>							m_MaterialList;
	float														m_TotalDuration;
	float														m_StartDelay;
	float														m_MinParticleSpawnRate;				// Particles per second
	float														m_MaxParticleSpawnRate;				// Particles per second
	kbVec3														m_MinParticleStartVelocity;
	kbVec3														m_MaxParticleStartVelocity;
	std::vector<kbAnimEvent>									m_VelocityOverLifeTimeCurve;
	kbVec3														m_MinParticleEndVelocity;
	kbVec3														m_MaxParticleEndVelocity;
	float														m_MinStartRotationRate;
	float														m_MaxStartRotationRate;
	float														m_MinEndRotationRate;
	float														m_MaxEndRotationRate;
	kbVec3														m_MinParticleStartSize;
	kbVec3														m_MaxParticleStartSize;
	kbVec3														m_MinParticleEndSize;
	kbVec3														m_MaxParticleEndSize;
	float														m_ParticleMinDuration;
	float														m_ParticleMaxDuration;
	kbVec4														m_ParticleStartColor;
	kbVec4														m_ParticleEndColor;
	std::vector<kbVectorAnimEvent>								m_ColorOverLifeTimeCurve;
	std::vector<kbAnimEvent>									m_AlphaOverLifeTimeCurve;
	kbVec3														m_Gravity;
	int															m_MinBurstCount;
	int															m_MaxBurstCount;
	EBillboardType												m_ParticleBillboardType;
	float														m_TranslucencySortBias;


	// Non-editable
	float														m_LeftOverTime;
	float														m_TimeAlive;
	int															m_BurstCount;
	float														m_StartDelayRemaining;

	kbRenderObject												m_RenderObject;

	static const int											NumParticleBuffers = 3;
	std::vector<kbParticle_t>									m_Particles;
	kbModel														m_ParticleBuffer[NumParticleBuffers];
	kbParticleVertex *											m_pVertexBuffer;
	ushort *													m_pIndexBuffer;

	unsigned int												m_NumIndicesInCurrentBuffer;
	byte														m_CurrentParticleBuffer;

	friend class kbParticleManager;
	const kbParticleComponent *									m_ParticleTemplate;
	bool														m_bIsPooled;
	bool														m_bIsSpawning;
};

#endif
