//===================================================================================================
// kbParticleManager.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBPARTICLEMANAGER_H_
#define _KBPARTICLEMANAGER_H_

#include <vector>
#include "kbGameEntityHeader.h"
#include "kbParticleComponent.h"

/**
 *	BufferedModel_t
 */
struct BufferedModel_t {

																BufferedModel_t();
																~BufferedModel_t();

	void														Release();
	kbModel *													m_pModels[3];

	int															m_CurrIdx;
};

/**
 *	kbParticleManager
 */
class kbParticleManager {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

public:
				
																kbParticleManager();
																~kbParticleManager();

	void														SetCustomAtlasTexture( const uint atlasIdx, const std::string & atlasFileName );
	void														SetCustomAtlasShader( const uint atlasIdx, const std::string & atlasFileName );

	void														PoolParticleComponent( const kbParticleComponent *const pParticle, const int PoolSize );

	kbParticleComponent *										GetParticleComponent( const kbParticleComponent *const pParticle );
	void														ReturnParticleComponent( kbParticleComponent *const pParticle );

	void														RenderSync();

	struct CustomParticleAtlasInfo_t {
		EBillboardType											m_Type;
		kbVec3													m_Position;
		kbVec3													m_Direction;
		float													m_Rotation;
		kbVec4													m_Color;
		float													m_Width;
		float													m_Height;
		kbVec2													m_UVs[2];
	};
	void														AddQuad( const uint atlasIdx, const CustomParticleAtlasInfo_t & CustomParticleInfo );

	BufferedModel_t *											GetModelEmitter();
	void														ReturnModelEmitter( BufferedModel_t *const pModelEmitter );

private:

	std::map<const kbParticleComponent *, std::vector< kbParticleComponent *>>	m_ParticlePools;

	static const int											NumCustomParticleBuffers = 3;
	std::vector<CustomParticleAtlasInfo_t>						m_Particles;

	struct CustomAtlasParticles_t {
																CustomAtlasParticles_t() :
																	m_NumIndices( 0 ),
																	m_pVertexBuffer( nullptr ),
																	m_pIndexBuffer( nullptr ),
																	m_pAtlasTexture( nullptr ),
																	m_pAtlasShader( nullptr ),
																	m_iCurParticleModel( -1 ) { }

		kbRenderObject											m_RenderObject;

		kbModel													m_RenderModel[NumCustomParticleBuffers];
		uint													m_NumIndices;
	
		kbParticleVertex *										m_pVertexBuffer;
		ushort *												m_pIndexBuffer;
		
		kbTexture *												m_pAtlasTexture;
		kbShader *												m_pAtlasShader;

		int														m_iCurParticleModel;
	};
	std::vector<CustomAtlasParticles_t>							m_CustomAtlases;

	std::vector<BufferedModel_t*>								m_ModelEmitterPool;

private:

	void														UpdateAtlas( CustomAtlasParticles_t & atlasInfo );
};

#endif
