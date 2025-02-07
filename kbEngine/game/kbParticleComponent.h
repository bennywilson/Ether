//===================================================================================================
// kbParticleComponent.h
//
//
// 2016 blk 1.0
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
																kbParticle_t();
																~kbParticle_t();

	void														Shutdown();

	Vec3														m_Position;
	float														m_Rotation;
	Vec3														m_StartSize;
	Vec3														m_EndSize;
	float														m_LifeLeft;
	float														m_TotalLife;
	Vec3														m_StartVelocity;
	Vec3														m_EndVelocity;
	float														m_StartRotation;
	float														m_EndRotation;
	float														m_Randoms[3];
	Vec3														m_RotationAxis;

	class kbModelEmitter *										m_pSrcModelEmitter;

	int															m_CurrentModelIndex;
	kbRenderObject												m_RenderObject;		// For model emitters
};

/**
 *	kbModelEmitter
 */
class kbModelEmitter : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbModelEmitter, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	const kbModel *												GetModel() const { return m_pModel; }
	const std::vector<kbShaderParamOverrides_t>					GetShaderParamOverrides() const { return m_ShaderParams; }

	void														Init();

private:
	kbModel *													m_pModel;
	std::vector<kbMaterialComponent>							m_MaterialList;

	std::vector<kbShaderParamOverrides_t>						m_ShaderParams;
};


/**
 *	kbParticleComponent
 */
class kbParticleComponent : public kbTransformComponent {

	KB_DECLARE_COMPONENT( kbParticleComponent, kbTransformComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual														~kbParticleComponent();

	virtual void												EditorChange( const std::string& propertyName );

	virtual void												RenderSync();

	void														StopParticleSystem();

	void														EnableNewSpawns( const bool bEnable );

	// Hack wasn't picking up from the package file
	void														SetBillboardType( const EBillboardType inBBType ) { m_ParticleBillboardType = inBBType; }

	bool														IsModelEmitter() const { return m_ModelEmitter.size() > 0 && m_ModelEmitter[0].GetModel() != nullptr; }


protected:

	virtual void												SetEnable_Internal( const bool isEnabled ) override;
	virtual void												Update_Internal( const float DeltaTime ) override;

private:

	// Editable
	std::vector<kbMaterialComponent>							m_MaterialList;
	float														m_TotalDuration;
	int															m_MaxParticlesToEmit;
	float														m_StartDelay;
	float														m_MinParticleSpawnRate;				// Particles per second
	float														m_MaxParticleSpawnRate;				// Particles per second
	Vec3														m_MinParticleStartVelocity;
	Vec3														m_MaxParticleStartVelocity;
	std::vector<kbAnimEvent>									m_VelocityOverLifeTimeCurve;
	Vec3														m_MinParticleEndVelocity;
	Vec3														m_MaxParticleEndVelocity;
	float														m_MinStartRotationRate;
	float														m_MaxStartRotationRate;
	float														m_MinEndRotationRate;
	float														m_MaxEndRotationRate;
	Vec3														m_MinStart3DRotation;
	Vec3														m_MaxStart3DRotation;
	Vec3														m_MinStart3DOffset;
	Vec3														m_MaxStart3DOffset;
	Vec3														m_MinParticleStartSize;
	Vec3														m_MaxParticleStartSize;
	Vec3														m_MinParticleEndSize;
	Vec3														m_MaxParticleEndSize;
	float														m_ParticleMinDuration;
	float														m_ParticleMaxDuration;
	Vec4														m_ParticleStartColor;
	Vec4														m_ParticleEndColor;
	std::vector<kbVectorAnimEvent>								m_SizeOverLifeTimeCurve;
	std::vector<kbVectorAnimEvent>								m_RotationOverLifeTimeCurve;
	std::vector<kbVectorAnimEvent>								m_ColorOverLifeTimeCurve;
	std::vector<kbAnimEvent>									m_AlphaOverLifeTimeCurve;
	Vec3														m_Gravity;
	int															m_MinBurstCount;
	int															m_MaxBurstCount;
	EBillboardType												m_ParticleBillboardType;
	std::vector<kbModelEmitter>									m_ModelEmitter;
	float														m_RenderOrderBias;
	bool														m_DebugPlayEntity;

	// Non-editable
	float														m_LeftOverTime;
	float														m_TimeAlive;
	int															m_BurstCount;
	float														m_StartDelayRemaining;
	int															m_NumEmittedParticles;

	kbRenderObject												m_RenderObject;

	static const int											NumParticleBuffers = 3;
	std::vector<kbParticle_t>									m_Particles;
	kbModel														m_ParticleBuffer[NumParticleBuffers];
	kbParticleVertex *											m_pVertexBuffer;
	ushort *													m_pIndexBuffer;

	byte														m_CurrentParticleBuffer;

	friend class kbParticleManager;
	const kbParticleComponent *									m_ParticleTemplate;
	bool														m_bIsPooled;
	bool														m_bIsSpawning;
};

#endif
