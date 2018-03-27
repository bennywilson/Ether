//===================================================================================================
// kbTerrainComponent.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _KBTERRAINCOMPONENT_H_
#define _KBTERRAINCOMPONENT_H_

#include "kbRenderer_defs.h"
#include "kbRenderBuffer.h"
#include "kbModel.h"

class kbTexture;

/**
 * kbTerrainComponent
 */
class kbTerrainComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbTerrainComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:

												~kbTerrainComponent();

	virtual void								PostLoad();

	void										SetHeightMap( kbTexture * pTexture ) { m_pHeightMap = pTexture; }

	virtual void								EditorChange( const std::string & propertyName );

protected:

	virtual void								Update_Internal( const float DeltaTime ) override;

	kbTexture *									m_pHeightMap;
	float										m_HeightScale;
	float										m_TerrainWidth;
	float										m_TerrainLength;
	kbModel										m_TerrainModel;

private:
	
	void										GenerateTerrain();
};
#endif
