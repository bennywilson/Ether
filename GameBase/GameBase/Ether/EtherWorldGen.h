//===================================================================================================
// EtherWorldGen.h
//
// Manages terrain streaming/generation + environmental fx
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERWORLDGEN_H_
#define _ETHERWORLDGEN_H_


/**
 *	EtherCoverObject
 */
class EtherCoverObject {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherCoverObject() { }
												EtherCoverObject( const kbBounds & inBounds, const float inHealth );

	kbVec3										GetPosition() const { return m_Position; }
	float										GetHealth() const { return m_Health; }
	const kbBounds &							GetBounds() const { return m_Bounds; }

private:
	kbVec3										m_Position;
	float										m_Health;
	kbBounds									m_Bounds;
};

/**
 *	EtherTerrainChunkGenJob
 */
class EtherTerrainChunkGenJob : public kbJob {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherTerrainChunkGenJob();

	virtual void								Run();
	void										Reset();

	// Inputs
	int											m_TrisPerChunkSide;
	float										m_ChunkWorldLength;
	float										m_TerrainGenNoiseScale;
	float										m_MaxTerrainHeight;
	float										m_MaxTerrainCellMidPointHeight;
	kbVec3										m_Position;
	const class EtherEnviroComponent *			m_EnviroComponent;

	// Outputs
	vertexLayout *								m_VertexBufferOutput;
	unsigned long *								m_IndexBufferOutput;
	std::vector<kbVec3> *						m_CollisionMeshOutput;
	std::vector<kbVec3> *						m_DynamicCollisionMeshOutput;
	std::vector<EtherCoverObject> *				m_pCoverObjects;

	int											m_NumDynamicVertices;
	int											m_NumDynamicIndices;
};

/**
 *	EtherTerrainChunk
 */
class EtherTerrainChunk {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												EtherTerrainChunk( const int numTrisPerSide );
												~EtherTerrainChunk();

	void										MarkAsAvailable();

	/** Provides the EtherTerrainChunk with a position and a kbComponent to pass to the renderer */
	class kbGameEntity *						m_pTerrainEntity;

	/** Procedurally generated terrain model */
	class kbModel *								m_pTerrainModel;

	/** Procedurally generated collision meshes */
	std::vector<kbVec3>							m_StaticCollisionMesh;
	std::vector<kbVec3>							m_DynamicCollisionMesh;

	/** Procedurally generated cover objects */
	std::vector<EtherCoverObject>				m_CoverObjects;

	enum ChunkState_t {
		Available,
		LoadingResources,
		StreamingIn,
		Visible
	};
	ChunkState_t								m_ChunkState;

	/** Job associated with this chunk.  Will generate the render model, collision geometry, and cover objects */
	EtherTerrainChunkGenJob						m_TerrainJob;

	/** The resources that the terrain job needs to generate this chunk */
	std::vector<kbString>						m_NeededResources;
};

/**
 *	EtherEnviroInfo
 */
class EtherEnviroInfo : public kbGameComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( EtherEnviroInfo, kbGameComponent );

	/** Points to a kbGameEntity with an EtherEnviroComponent on it */
	kbGameEntityPtr								m_EnvironmentData;
};

/**
 *	EtherWorldGenCollision_t
 */
struct EtherWorldGenCollision_t {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
												EtherWorldGenCollision_t() : m_bHitFound( false ) { }

	bool										m_bHitFound;
	kbVec3										m_HitLocation;
	float										m_DistFromOrigin;
};

/**
 *	EtherWorldGenComponent
 */
class EtherWorldGenComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherWorldGenComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual										~EtherWorldGenComponent();

	virtual void								RenderSync() override;

	// Collision functions
	/** Returns true if an intersection is detected */
	bool										TraceAgainstWorld( EtherWorldGenCollision_t & OutHitInfo, const kbVec3 & StartPt, const kbVec3 & EndPt, const bool bTraceAgainstDynamicCollision ) const;
	void										MoveActorAlongGround( class EtherActorComponent *const pActor, const kbVec3 & StartPt, const kbVec3 & EndPt ) const;

	/** Returns the number of cover objects within the radius */
	bool										CoverObjectsPointTest( const EtherCoverObject *& OutCoverObject, const kbVec3 & startPt ) const;
	int											GetCoverObjectsInRadius( std::vector<EtherCoverObject> & OutCoverObjects, const kbVec3 & startPt, const float Radius );

	void										SetTerrainWarp( const kbVec3 & location, const float amplitude, const float range, const float timeScale );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	void										InitializeWorld();
	void										TearDownWorld();

	void										RenderSyncStreaming();

	bool										TraceAgainstTerrain_Recurse( EtherWorldGenCollision_t & OutHitInfo, const kbVec3 & StartPt, const kbVec3 & EndPt, const float TraceLen, const bool bTraceAgainstDynamicCollision, const kbVec2 & LowerLeftPos, const float CurNodeLength ) const;

	void										UpdateTimeOfDayFX();
	void										UpdateDebug();

	kbVec3										GetTerrainAlignedPos( const kbVec3 & worldPosition ) const;

	int											m_ChunksPerTerrainSide;
	int											m_TrisPerChunkSide;
	float										m_ChunkWorldLength;
	float										m_HalfChunkWorldLength;
	float										m_TerrainGenNoiseScale;
	float										m_MaxTerrainHeight;
	float										m_MaxTerrainCellMidPointHeight;

	float										m_SecondsInADay;
	float										m_DebugHour;
	float										m_SecondsSinceSpawn;

	kbGameEntityPtr								m_SunEntity;
	kbGameEntityPtr								m_FogEntity;
	kbGameEntityPtr								m_SkyEntity;
	kbGameEntityPtr								m_LightShaftEntity;

	std::vector<EtherEnviroInfo>				m_EnviroInfo;

	std::vector<EtherTerrainChunk*>				m_TerrainChunksPool;
	std::vector<EtherTerrainChunk *>			m_StreamingInChunks;

	/** 2D array which contains the visible chunks.  As the player move, the visible chunks will wrap around the array */
	typedef std::vector<EtherTerrainChunk *> EtherChunkArray;
	struct VisibleTerrainMap_t {

		void										Initialize( const kbVec3 & centerPos, const float TerrainChunkWidth, const int TerrainDimensions );
		bool										IsInitialized() const { return m_LowerLeftCornerIdx != -1; }
		void										Shutdown() { m_LowerLeftCornerIdx = -1; }

		void										Update( const kbVec3 & centerPos );

		void										AddChunk( EtherTerrainChunk *const pChunkToAdd );
		void										RemoveChunk( EtherTerrainChunk *const pChunkToRemove );

		int											GetChunkIndexFromPosition( const kbVec3 & position ) const;

		int											GetLowerLeftIdx() const { return m_LowerLeftCornerIdx; }
		kbVec3										GetLowerLeftPos() const { return m_LowerLeftCornerPosition; }

		int											Get2DMapDimensions() const { return m_2DMapDimensions; }
		EtherChunkArray &							Get2DMap() { return m_TerrainChunk2DMap; }
		const EtherChunkArray						Get2DMap() const { return m_TerrainChunk2DMap; }

private:
		int											m_LowerLeftCornerIdx;
		kbVec3										m_LowerLeftCornerPosition;

		float										m_ChunkWorldLength;
		int											m_2DMapDimensions;
		EtherChunkArray								m_TerrainChunk2DMap;
	};

	VisibleTerrainMap_t								m_VisibleTerrainMap;

	kbShader *										m_pTerrainShader;
};


//===================================================================================================
// Environment objects classes
//===================================================================================================

/**
 *	EtherEnviroMaterial
 */
class EtherEnviroMaterial : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherEnviroMaterial, kbGameComponent );

	friend class EtherEnviroComponent;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	const kbVec4 &								GetColor() const { return m_Color; }
	const kbTexture *							GetTexture() const { return m_Texture; }

protected:

	kbVec4										m_Color;
	kbTexture *									m_Texture;
};

/**
 *	EtherEnviroObject
 */
class EtherEnviroObject : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherEnviroObject, kbGameComponent );

	friend class EtherWorldGenComponent;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:


	const kbModel *								GetModel() const { return m_pModel; }

	const kbVec3 &								GetMinScale() const { return m_MinScale; }
	const kbVec3 &								GetMaxScale() const { return m_MaxScale; }

private:
	class kbModel *								m_pModel;
	kbVec3										m_MinScale;
	kbVec3										m_MaxScale;
	float										m_MinHealth;
	float										m_MaxHealth;
};

/**
 *	EtherTimeOfDayModifier
 *  Todo: Verify that the array of modifiers is in ascending order
 */
class EtherTimeOfDayModifier : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherTimeOfDayModifier, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	float										GetHour() const { return m_Hour; }
	const kbColor &								GetSunColor() const { return m_SunColor; }
	const kbColor &								GetFogColor() const { return m_FogColor; }
	const kbColor &								GetSkyColor() const { return m_SkyColor; }
	const kbColor &								GetLightShaftColor() const { return m_LightShaftColor; }

private:
	/** Takes values between 0 and 23 */
	float										m_Hour;
	kbColor										m_SunColor;
	kbColor										m_FogColor;
	kbColor										m_SkyColor;
	kbColor										m_LightShaftColor;
};

/**
 *	EtherEnviroComponent
 */
class EtherEnviroComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherEnviroComponent, kbGameComponent );

	friend class EtherWorldGenComponent;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	const std::vector<EtherEnviroObject> &		GetEnviroObjects() const { return m_EnviroObjects; }
	const std::vector<EtherEnviroObject> &		GetCoverObjects() const { return m_CoverObjects; }
	const std::vector<EtherTimeOfDayModifier> & GetTimeOfDayModifiers() const { return m_TimeOfDayModifiers; }

private:
	std::vector<EtherEnviroMaterial>			m_EnviroMaterials;
	std::vector<EtherEnviroObject>				m_CoverObjects;
	std::vector<EtherEnviroObject>				m_EnviroObjects;
	std::vector<EtherTimeOfDayModifier>			m_TimeOfDayModifiers;
};

extern const int g_MaxDynamicVertices;

#endif