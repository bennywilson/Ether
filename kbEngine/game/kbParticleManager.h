/// kbParticleManager.h
///
/// 2016-2025 blk 1.0

#pragma once

#include <vector>
#include "matrix.h"
#include "kbGameEntityHeader.h"
#include "kbParticleComponent.h"

class kbParticleComponent;

/// kbParticleManager
class kbParticleManager {
public:
	kbParticleManager();
	~kbParticleManager();

	void SetCustomAtlasTexture(const uint atlasIdx, const std::string& atlasFileName);
	void SetCustomAtlasShader(const uint atlasIdx, const std::string& atlasFileName);

	void PoolParticleComponent(const kbParticleComponent* const pParticle, const int PoolSize);

	kbParticleComponent* GetParticleComponent(const kbParticleComponent* const pParticle);
	void ReturnParticleComponent(kbParticleComponent* const pParticle);

	void RenderSync();

	struct CustomParticleAtlasInfo_t {
		EBillboardType m_Type;
		Vec3 m_position;
		Vec3 m_Direction;
		float m_Rotation;
		Vec4 m_Color;
		float m_Width;
		float m_Height;
		Vec2 m_UVs[2];
	};
	void AddQuad(const uint atlasIdx, const CustomParticleAtlasInfo_t& CustomParticleInfo);

	const kbGameComponent* GetComponentFromPool();
	void ReturnComponentToPool(const kbGameComponent* const);

	void ReserveScratchBufferSpace(kbParticleVertex*& outVertexBuffer, kbRenderObject& inOutRenderObj, const int numRequestedVerts);

private:
	std::map<const kbParticleComponent*, std::vector<kbParticleComponent*>> m_ParticlePools;

	static const int NumCustomParticleBuffers = 3;
	std::vector<CustomParticleAtlasInfo_t> m_Particles;

	struct CustomAtlasParticle_t {
		CustomAtlasParticle_t() :
			m_NumIndices(0),
			m_pVertexBuffer(nullptr),
			m_pIndexBuffer(nullptr),
			m_pAtlasTexture(nullptr),
			m_pAtlasShader(nullptr),
			m_iCurParticleModel(-1) { }

		kbRenderObject	m_render_object;

		kbModel	m_RenderModel[NumCustomParticleBuffers];
		uint m_NumIndices;

		kbParticleVertex* m_pVertexBuffer;
		ushort* m_pIndexBuffer;

		kbTexture* m_pAtlasTexture;
		kbShader* m_pAtlasShader;

		int m_iCurParticleModel;
	};
	std::vector<CustomAtlasParticle_t> m_CustomAtlases;

	struct ScratchBuffer_t {
		ScratchBuffer_t() :
			m_iVert(0),
			m_iCurModel(-1),
			m_pVertexBuffer(nullptr),
			m_pIndexBuffer(nullptr) { }

		kbModel	m_RenderModel[NumCustomParticleBuffers];
		uint m_iVert;
		int	m_iCurModel;

		kbParticleVertex* m_pVertexBuffer;
		ushort* m_pIndexBuffer;
	};

	std::vector<ScratchBuffer_t> m_ScratchParticleBuffers;

	std::vector<const kbGameComponent*>	m_ComponentPool;

private:
	void UpdateAtlas(CustomAtlasParticle_t& atlasInfo);
};
