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

	virtual void								EditorChange( const std::string & propertyName );

private:

	bool										NeedsMaterialUpdate() const { return m_bNeedsMaterialUpdate; }
	void										ClearMaterialUpdate() { m_bNeedsMaterialUpdate = false; }

	float										m_MinBladeWidth;
	float										m_MaxBladeWidth;

	float										m_MinBladeHeight;
	float										m_MaxBladeHeight;

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
	FLOAT										m_SpecPowerMultiplier;

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


protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

    void                                        SetMaterialParams();

	// Editor properties
	kbTexture *									m_pHeightMap;
	float										m_HeightScale;
	float										m_TerrainWidth;
	int											m_TerrainDimensions;

	std::vector<kbTerrainMatComponent>          m_TerrainMaterials;
	kbShader *                                  m_pTerrainShader;
	kbTexture *                                 m_pSplatMap;
	kbTexture *									m_pGrassMap;
    std::vector<kbGrass>                        m_Grass;

	// Non-editor
	kbModel										m_TerrainModel;
	kbShaderParamOverrides_t					m_TerrainShaderOverrides;
    std::vector<kbShader *>                     m_ShaderOverrideList;

    kbModel                                     m_GrassModel;
	kbShaderParamOverrides_t					m_GrassShaderOverrides;

	bool										m_bRegenerateTerrain;

private:
	
	void										GenerateTerrain();
};
#endif
