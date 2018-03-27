//===================================================================================================
// kbWorldGen.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBWORLDGEN_H_
#define _KBWORLDGEN_H_

/**
 *	kbTerrainCellGenJob
 */
class kbTerrainCellGenJob : public kbJob {
public:
											kbTerrainCellGenJob();

	virtual void							Run();

	// Inputs
	int										m_NumTerrainChunks;
	int										m_ChunkDimensions;
	float									m_ChunkWorldLength;
	float									m_TerrainGenerationNoiseScale;
	float									m_MaxTerrainHeight;
	float									m_MaxTerrainCellMidPointHeight;
	kbVec3									m_Position;
	const class kbDQuadEnviroComponent *	m_EnviroComponent;

	// Outputs
	vertexLayout *							m_VertexBufferOutput;
	unsigned long *							m_IndexBufferOutput;
	kbVec3 *								m_CollisionMeshOutput;
	kbVec3 *								m_DynamicCollisionMeshOutput;

	int										m_NumDynamicObjects;
	int										m_NumDynamicVertices;
	int										m_NumDynamicIndices;
};

/**
 *	terrainChunk_t
 */
struct terrainChunk_t {

	terrainChunk_t();

	~terrainChunk_t() {
		delete m_pTerrainEntity;
		delete m_pTerrainModel;
		delete m_pCollisionMesh;
		delete m_pDynamicCollisionMesh;
	}

	void Initialize( const int dimensions );

	class kbGameEntity *		m_pTerrainEntity;
	class kbComponent			m_HackDummyComponent;

	class kbModel *				m_pTerrainModel;
	kbVec3 *					m_pCollisionMesh;
	kbVec3 *					m_pDynamicCollisionMesh;

	enum ChunkState_t {
		Available,
		WaitingOnEnviroStreaming,
		StreamingIn,
		StreamedIn,
		Visible
	} m_State;

	kbTerrainCellGenJob			m_TerrainJob;

	std::vector<kbString>		m_NeededResources;
};

/**
 *	kbDQuadEnviroInfo
 */
class kbDQuadEnviroInfo : public kbComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadEnviroInfo, kbComponent );

	kbGameEntityPtr		m_EnvironmentData;
};

/**
 *	kbDQuadWorldGenComponent
 */
struct kbWorldGenCollision_t {
	kbWorldGenCollision_t() :
		m_bHitFound( false ) {

	}

	bool	m_bHitFound;
	kbVec3	m_HitLocation;
};

class kbDQuadWorldGenComponent : public kbComponent {

public:
	KB_DECLARE_COMPONENT( kbDQuadWorldGenComponent, kbComponent );

	virtual							~kbDQuadWorldGenComponent();

	virtual void					RenderSync();
	virtual void					Update( const float DeltaTime );

	bool							TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbWorldGenCollision_t & HitInfo, const bool bTraceAgainstDynamicCollision ) const;

protected:
		
	virtual void					SetEnable_Internal( const bool isEnabled );

private:

	void							InitializeWorld();
	void							TearDownWorld();
	void							UpdateWorldStreaming();
	
	int								m_TerrainDimensions;
	int								m_NumTerrainChunks;
	int								m_ChunkPoolSize;
	int								m_ChunkDimensions;
	float							m_ChunkWorldLength;
	float							m_HalfChunkWorldLength;
	float							m_TerrainGenerationNoiseScale;
	float							m_MaxTerrainHeight;
	float							m_MaxTerrainCellMidPointHeight;
	float							m_SecondsInADay;
	float							m_DebugHour;
	float							m_SecondsSinceSpawn;

	kbGameEntityPtr					m_SunEntity;
	kbGameEntityPtr					m_FogEntity;
	kbGameEntityPtr					m_SkyEntity;
	kbGameEntityPtr					m_LightShaftEntity;

	std::vector<kbDQuadEnviroInfo>	m_EnviroInfo;

	terrainChunk_t *				m_TerrainChunks;

	std::vector<terrainChunk_t *>	m_StreamingChunks;
};


//===================================================================================================
// Environment related classes
//===================================================================================================

/**
 *	kbDQuadEnviroMaterial
 */
class kbDQuadEnviroMaterial : public kbComponent {
public:
	friend class kbDQuadEnviroComponent;

	KB_DECLARE_COMPONENT( kbDQuadEnviroMaterial, kbComponent );
	kbVec4						m_Color;
	kbTexture *					m_Texture;
};

/**
 *	kbDQuadEnviroObject
 */
class kbDQuadEnviroObject : public kbComponent {
public:
	friend class kbDQuadWorldGenComponent;
	friend class kbDQuadEnviroComponent;

	KB_DECLARE_COMPONENT( kbDQuadEnviroObject, kbComponent );

	const kbModel *				GetModel() const { return m_pModel; }

	const kbVec3 &				GetMinScale() const { return m_MinScale; }
	const kbVec3 &				GetMaxScale() const { return m_MaxScale; }

private:
	class kbModel *				m_pModel;
	kbVec3						m_MinScale;
	kbVec3						m_MaxScale;
	float						m_MinHealth;
	float						m_MaxHealth;
};

/**
 *	kbDQuadTimeOfDayModifier
 */
class kbDQuadTimeOfDayModifier : public kbComponent {
public:

	KB_DECLARE_COMPONENT( kbDQuadTimeOfDayModifier, kbComponent );

	float								GetHour() const { return m_Hour; }
	const kbColor &						GetSunColor() const { return m_SunColor; }
	const kbColor &						GetFogColor() const { return m_FogColor; }
	const kbColor &						GetSkyColor() const { return m_SkyColor; }
	const kbColor &						GetLightShaftColor() const { return m_LightShaftColor; }

private:
	float								m_Hour;			// 0-24
	kbColor								m_SunColor;
	kbColor								m_FogColor;
	kbColor								m_SkyColor;
	kbColor								m_LightShaftColor;
};

/**
 *	kbDQuadEnviroComponent
 */
class kbDQuadEnviroComponent : public kbComponent {
public:
	friend class kbDQuadWorldGenComponent;

	KB_DECLARE_COMPONENT( kbDQuadEnviroComponent, kbComponent );

	const std::vector<kbDQuadEnviroObject> & GetEnviroObjects() const { return m_EnviroObjects; }
	const std::vector<kbDQuadEnviroObject> & GetCoverObjects() const { return m_CoverObjects; }
	const std::vector<kbDQuadTimeOfDayModifier> & GetTimeOfDayModifiers() const { return m_TimeOfDayModifiers; }

private:
	std::vector<kbDQuadEnviroMaterial>		m_EnviroMaterials;
	std::vector<kbDQuadEnviroObject>		m_CoverObjects;
	std::vector<kbDQuadEnviroObject>		m_EnviroObjects;
	std::vector<kbDQuadTimeOfDayModifier>	m_TimeOfDayModifiers;
};

extern const int g_MaxDynamicVertices;

#endif