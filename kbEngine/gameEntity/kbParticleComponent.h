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
	BT_AxialBillboard
};

struct kbParticle_t {
	kbVec3 m_Position;
	kbVec2 m_StartSize;
	kbVec2 m_EndSize;
	float m_LifeLeft;
	float m_TotalLife;
	kbVec3 m_StartVelocity;
	kbVec3 m_EndVelocity;
	float m_Randoms[3];
};

/*
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
	kbTexture *													m_pParticleTexture;
	kbShader *													m_pParticleShader;
	float														m_TotalDuration;
	float														m_MinParticleSpawnRate;				// Particles per second
	float														m_MaxParticleSpawnRate;				// Particles per second
	kbVec3														m_MinParticleStartVelocity;
	kbVec3														m_MaxParticleStartVelocity;
	kbVec3														m_MinParticleEndVelocity;
	kbVec3														m_MaxParticleEndVelocity;
	kbVec3														m_MinParticleStartSize;
	kbVec3														m_MaxParticleStartSize;
	kbVec3														m_MinParticleEndSize;
	kbVec3														m_MaxParticleEndSize;
	float														m_ParticleMinDuration;
	float														m_ParticleMaxDuration;
	kbVec4														m_ParticleStartColor;
	kbVec4														m_ParticleEndColor;
	kbVec3														m_Gravity;
	int															m_MinBurstCount;
	int															m_MaxBurstCount;
	EBillboardType												m_ParticleBillboardType;
	float														m_TranslucencySortBias;
	bool														m_bLockVelocity;

	// Non-editable
	float														m_LeftOverTime;
	float														m_TimeAlive;
	int															m_BurstCount;

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
