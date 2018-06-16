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

private:
	int                                         m_Dummy;
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

private:

    kbTexture *									m_pDiffuseMap;
	kbTexture *									m_pNormalMap;
	kbTexture *									m_pSpecMap;
	float										m_SpecFactor;
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

	kbTexture *									m_pHeightMap;
	float										m_HeightScale;
	float										m_TerrainWidth;
	int											m_TerrainDimensions;
	std::vector<kbTerrainMatComponent>          m_TerrainMaterials;
	kbShader *                                  m_pTerrainShader;
	kbTexture *                                 m_pSplatMap;
    std::vector<kbGrass>                        m_Grass;

	kbModel										m_TerrainModel;
	kbShaderParamOverrides_t					m_ShaderParamOverride;
    std::vector<kbShader *>                     m_ShaderOverrideList;

    kbModel                                     m_GrassModel;

	bool										m_bRegenerateTerrain;

private:
	
	void										GenerateTerrain();
};
#endif
