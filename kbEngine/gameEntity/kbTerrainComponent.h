//===================================================================================================
// kbTerrainComponent.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBTERRAINCOMPONENT_H_
#define _KBTERRAINCOMPONENT_H_

#include "kbRenderer_defs.h"
#include "kbRenderBuffer.h"
#include "kbModel.h"

/**
 *	kbGrass
 */
class kbGrass : public kbGameComponent {
	friend class kbTerrainComponent;

	KB_DECLARE_COMPONENT( kbGrass, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

												~kbGrass();

	virtual void								EditorChange( const std::string & propertyName ) override;
	virtual void								RenderSync() override;

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;

private:

	void										SetOwningTerrainComponent( kbTerrainComponent *const pTerrain ) { m_pOwningTerrainComponent = pTerrain; m_bNeedsMaterialUpdate = true; }

	void										UpdateMaterial();

	int											m_GrassCellsPerTerrainSide;

	kbTexture *									m_pGrassMap;
    kbTexture *                                 m_pNoiseMap;

	float										m_PatchStartCullDistance;
	float										m_PatchEndCullDistance;

	int											m_PatchesPerCellSide;

	float										m_BladeMinWidth;
	float										m_BladeMaxWidth;

	float										m_BladeMinHeight;
	float										m_BladeMaxHeight;

	float										m_MaxPatchJitterOffset;
	float										m_MaxBladeJitterOffset;

    kbTexture *									m_pDiffuseMap;

    kbVec3                                      m_TestWind;

    float                                       m_FakeAODarkness;
    float                                       m_FakeAOPower;

private:

	float										m_GrassCellLength;

	struct grassRenderObject_t {
												grassRenderObject_t() : m_pModel( nullptr ), m_pComponent( nullptr ) { }

		void									Initialize( const kbVec3 & ownerPosition );
		void									Shutdown();

		kbModel *								m_pModel;
		kbComponent *							m_pComponent;
		kbRenderObject							m_RenderObject;
	};
	std::vector<grassRenderObject_t>			m_GrassRenderObjects;

	kbShaderParamOverrides_t					m_GrassShaderOverrides;

	kbTerrainComponent *						m_pOwningTerrainComponent;
	bool										m_bNeedsMaterialUpdate;
};

/**
 *  kbTerrainMatComponent
 */
class kbTerrainMatComponent : public kbComponent {

    friend class kbTerrainComponent;

    KB_DECLARE_COMPONENT( kbTerrainMatComponent, kbComponent );

//---------------------------------------------------------------------------------------------------
public:

	kbTexture *									GetDiffuseMap() const { return m_pDiffuseMap; }
	kbTexture *									GetNormalMap() const { return m_pNormalMap; }
	kbTexture *									GetSpecMap() const { return m_pSpecMap; }

	const kbVec3 &								GetUVScale() const { return m_UVScale; }
	float										GetSpecFactor() const { return m_SpecFactor; }
	float										GetSpecPowerMultiplier() const { return m_SpecPowerMultiplier; }

private:

    kbTexture *									m_pDiffuseMap;
	kbTexture *									m_pNormalMap;
	kbTexture *									m_pSpecMap;
	float										m_SpecFactor;
	float										m_SpecPowerMultiplier;

	kbVec3										m_UVScale;
};

/**
 * kbTerrainComponent
 */
class kbTerrainComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbTerrainComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:
												~kbTerrainComponent();

	virtual void								PostLoad() override;

	void										SetHeightMap( kbTexture * pTexture ) { m_pHeightMap = pTexture; }

	virtual void								EditorChange( const std::string & propertyName ) override;

	virtual void								RenderSync() override;

	kbTexture *									GetHeightMap() const { return m_pHeightMap; }
	float										GetHeightScale() const { return m_HeightScale; }
	float										GetTerrainWidth() const { return m_TerrainWidth; }

	void										SetCollisionMap( const kbRenderTexture *const pTexture );
	void										SetGrassTexture( const std::string & textureName, const kbTexture *const pTexture );

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

    void                                        UpdateTerrainMaterial();

	// Editor properties
	kbTexture *									m_pHeightMap;
	float										m_HeightScale;
	float										m_TerrainWidth;
	int											m_TerrainDimensions;

	std::vector<kbTerrainMatComponent>          m_TerrainMaterials;
	kbShader *                                  m_pTerrainShader;
	kbTexture *                                 m_pSplatMap;
    std::vector<kbGrass>                        m_Grass;

	// Non-editor
	kbModel										m_TerrainModel;
	kbRenderObject								m_TerrainRenderObject;

	kbShaderParamOverrides_t					m_TerrainShaderOverrides;
    std::vector<kbShader *>                     m_ShaderOverrideList;

	bool										m_bRegenerateTerrain;

private:
	
	void										GenerateTerrain();
};
#endif
