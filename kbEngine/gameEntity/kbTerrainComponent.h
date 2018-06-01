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

class kbTexture;

/**
 *  kbTerrainMatComponent
 */
class kbTerrainMatComponent : public kbGameComponent {

    friend class kbTerrainComponent;

    KB_DECLARE_COMPONENT( kbTerrainMatComponent, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:
	kbTexture *									GetDiffuseTexture() const { return m_pDiffuseTexture; }

private:
    kbTexture *									m_pDiffuseTexture;
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

	kbTexture *									m_pHeightMap;
	float										m_HeightScale;
	float										m_TerrainWidth;
	int											m_TerrainDimensions;
	std::vector<kbTerrainMatComponent>          m_TerrainMaterials;
	kbShader *                                  m_pTerrainShader;
	kbTexture *                                 m_pSplatMap;

	kbModel										m_TerrainModel;
	kbShaderParamOverrides_t					m_ShaderParamOverride;
	bool										m_bRegenerateTerrain;

private:
	
	void										GenerateTerrain();
};
#endif
