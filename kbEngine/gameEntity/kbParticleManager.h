//===================================================================================================
// kbParticleManager.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBPARTICLEMANAGER_H_
#define _KBPARTICLEMANAGER_H_

#include <vector>
#include "kbGameEntityHeader.h"
#include "kbParticleComponent.h"

/**
 *	kbParticleManager
 */
class kbParticleManager {
public:
											kbParticleManager();
											~kbParticleManager();

	void									SetCustomParticleTextureAtlas( const std::string & atlasFileName );

	void									PoolParticleComponent( const kbParticleComponent *const pParticle, const int PoolSize );

	kbParticleComponent *					GetParticleComponent( const kbParticleComponent *const pParticle );
	void									ReturnParticleComponent( kbParticleComponent *const pParticle );

	void									RenderSync();

	struct CustomParticleInfo_t {
		EBillboardType						m_Type;
		kbVec3								m_Position;
		kbVec3								m_Direction;
		kbVec3								m_Color;
		float								m_Width;
		float								m_Height;
		kbVec2								m_UVs[2];
	};
	void									AddQuad( const CustomParticleInfo_t & CustomParticleInfo );

private:

	std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>	m_ParticlePools;

	static const int						NumCustomParticleBuffers = 3;
	std::vector<CustomParticleInfo_t>		m_Particles;
	kbModel									m_CustomParticleBuffer[NumCustomParticleBuffers];
	uint									m_NumIndicesInCurrentBuffer;
	byte									m_CurrentParticleBuffer;

	kbParticleVertex *						m_pVertexBuffer;
	unsigned long *							m_pIndexBuffer;

	kbTexture *								m_pParticleTexture;
};

#endif
