///
/// kbTerrainComponent.h
///
/// 2016-2025 blk 1.0
#pragma once

#include "kbRenderBuffer.h"
#include "kbModel.h"

///
///	kbGrass
///
class kbGrass : public kbGameComponent {
	friend class kbTerrainComponent;
	KB_DECLARE_COMPONENT(kbGrass, kbGameComponent);

public:
	~kbGrass();

	virtual void EditorChange(const std::string& propertyName) override;
	virtual void RenderSync() override;

protected:
	virtual void SetEnable_Internal(const bool isEnabled) override;

private:
	void SetOwningTerrainComponent(kbTerrainComponent* const pTerrain) { m_pOwningTerrainComponent = pTerrain; m_bUpdateMaterial = true; m_bUpdatePointCloud = true; }

	void RefreshGrass();

	kbShader* m_pGrassShader;
	int	m_GrassCellsPerTerrainSide;

	std::vector<kbShaderParamComponent> m_ShaderParamList;

	float m_PatchStartCullDistance;
	float m_PatchEndCullDistance;

	int	m_PatchesPerCellSide;

	float m_BladeMinWidth;
	float m_BladeMaxWidth;

	float m_BladeMinHeight;
	float m_BladeMaxHeight;

	float m_MaxPatchJitterOffset;
	float m_MaxBladeJitterOffset;

	float m_FakeAODarkness;
	float m_FakeAOPower;
	float m_FakeAOClipPlaneFadeStartDist;

private:
	// Editor
	float m_GrassCellLength;

	struct grassRenderObject_t {
		grassRenderObject_t() : m_pModel(nullptr), m_pComponent(nullptr) { }

		void Initialize(const Vec3& ownerPosition);
		void Shutdown();

		kbModel* m_pModel;
		kbGameComponent* m_pComponent;
		kbRenderObject m_RenderObject;
	};
	std::vector<grassRenderObject_t> m_GrassRenderObjects;

	kbShaderParamOverrides_t m_GrassShaderOverrides;

	// Runtime
	kbTerrainComponent* m_pOwningTerrainComponent;

	bool m_bUpdatePointCloud;
	bool m_bUpdateMaterial;
};

/// kbGrassZone
class kbGrassZone : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbGrassZone, kbGameComponent);

public:
	Vec3 GetCenter() const { return m_Center; }
	Vec3 GetExtents() const { return m_Extents; }

private:
	Vec3 m_Center;
	Vec3 m_Extents;
};


/// kbTerrainComponent
class kbTerrainComponent : public RenderComponent {
	KB_DECLARE_COMPONENT(kbTerrainComponent, RenderComponent);

public:
	~kbTerrainComponent();

	virtual void PostLoad() override;

	void SetHeightMap(kbTexture* const pTexture) { m_pHeightMap = pTexture; }

	virtual void EditorChange(const std::string& propertyName) override;

	virtual void RenderSync() override;

	kbTexture* GetHeightMap() const { return m_pHeightMap; }
	float GetHeightScale() const { return m_HeightScale; }
	float GetTerrainWidth() const { return m_TerrainWidth; }

	void SetCollisionMap(const kbRenderTexture* const pTexture);

	static void	SetTerrainLOD(const float lod);

	void RegenerateTerrain() { m_bRegenerateTerrain = true; }

	const std::vector<kbGrassZone> GetGrassZones() const { return m_GrassZones; }

protected:
	virtual void SetEnable_Internal(const bool isEnabled) override;
	virtual void Update_Internal(const float DeltaTime) override;

	void RefreshMaterials();

private:
	void GenerateTerrain();

protected:
	// Editor properties
	kbTexture* m_pHeightMap;
	float m_HeightScale;
	float m_TerrainWidth;
	int m_TerrainDimensions;
	int	m_TerrainSmoothAmount;

	kbTexture* m_pSplatMap;
	std::vector<kbGrass> m_Grass;
	std::vector<kbGrassZone> m_GrassZones;

	bool m_bDebugForceRegenTerrain;

	// Non-editor
	kbModel	m_TerrainModel;
	float m_LastHeightMapLoadTime;

	bool m_bRegenerateTerrain;
};

extern bool g_bCullGrass;
