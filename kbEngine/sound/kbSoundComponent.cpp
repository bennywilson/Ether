//==============================================================================
// kbSoundComponent.cpp
//
//
// 2017 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"
#include "kbSoundComponent.h"

/**
 *	kbSoundData::Constructor
 */
void kbSoundData::Constructor() {
	m_pWaveFile = nullptr;
	m_Radius = -1.0f;
	m_Volume = 1.0f;
}

/**
 *	kbSoundData::PlaySoundAtPosition
 */
void kbSoundData::PlaySoundAtPosition( const kbVec3 & soundPosition ) const {

	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );

	const float distToCamera = ( currentCameraPosition - soundPosition ).Length();	
	float atten = 1.0f;
	if ( m_Radius > 0.0f ) {
		if ( distToCamera > m_Radius ) {
			return;
		} else {
			atten = 1.0f - ( distToCamera / m_Radius );
		}
	}

	g_pGame->GetSoundManager().PlayWave( m_pWaveFile, atten * m_Volume );
}