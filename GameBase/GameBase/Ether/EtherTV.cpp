//===================================================================================================
// EtherTV.cpp
//
//
// 2020 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "EtherGame.h"
#include "EtherTV.h"

/**
 *	EtherTVComponent::Constructor
 */
void EtherTVComponent::Constructor() {

	m_MovieLen = 10.0f;
	m_DelayBeforeAd = 5.0f;
	m_AdLength = 15;
	m_pELPAd = nullptr;
	m_GlitchDuration = 1.0f;

	m_pStaticModel = nullptr;
	m_StartTime = 0.0f;
	m_State = 0;
}

/**
 *	EtherTVComponent::SetEnable_Internal
 */
void EtherTVComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );
	
	
	for ( int i = 0; i < m_SheepAndFoxClip.size(); i++ ) {
		std::string name = "./assets/TV/Layer " + std::to_string( i + 1 ) + ".png";

		kbLog( "Loading %s", name.c_str() );
		m_SheepAndFoxClip[i] = (kbTexture*) g_ResourceManager.GetResource( name.c_str(), true, true );
	}
	
	m_pStaticModel = GetOwner()->GetComponent<kbStaticModelComponent>();

	m_StartTime = g_GlobalTimer.TimeElapsedSeconds();
	m_State = 0;
}

/**
 *	EtherTVComponent::Update_Internal
 */
void EtherTVComponent::Update_Internal( const float dt ) {

	Super::Update_Internal( dt );

	/*if ( m_SheepAndFoxClip[0] == nullptr ) {
		for ( int i = 0; i < m_SheepAndFoxClip.size(); i++ ) {
			std::string name = "./assets/TV/Layer " + std::to_string( i + 1 ) + ".jpg";

			kbLog( "Loading %s", name.c_str() );
			m_SheepAndFoxClip[i] = (kbTexture*) g_ResourceManager.GetResource( name.c_str(), true, true );
		}
	}*/

	/*
	const float duration = fmod( g_GlobalTimer.TimeElapsedSeconds() - m_StartTime, m_MovieLen );
	const int iTex = (int)( ( duration / m_MovieLen ) * m_SheepAndFoxClip.size() );
	static int l = 0;
	m_pStaticModel->SetMaterialParamTexture( 0, "screenColorTex", m_SheepAndFoxClip[iTex] );
	*/
	const float duration = g_GlobalTimer.TimeElapsedSeconds() - m_StartTime;
	const float curTime = g_GlobalTimer.TimeElapsedSeconds();

	if ( m_State == 0 ) {
		EtherLightAnimatorComponent::SetGlobalMultiplier( 1.0f );
		if ( duration > m_MovieLen ) {
			m_State = 1;
			m_StartTime = curTime;
		} else {
			const int iTex = (int)( ( duration / m_MovieLen ) * m_SheepAndFoxClip.size() );
			m_pStaticModel->SetMaterialParamTexture( 0, "screenColorTex", m_SheepAndFoxClip[iTex] );
		}
	} else if ( m_State == 1 ) {

		if ( duration > m_DelayBeforeAd ) {
			m_pStaticModel->SetMaterialParamVector( 0, "lineSpeedAndRadius", kbVec4( 55.0f, 43.0f, 1.0f, 1.0f ) );
			m_State = 2;
			m_StartTime = curTime;
		}
	} else if ( m_State == 2 ) {

		if ( duration > m_GlitchDuration ) {
			m_pStaticModel->SetMaterialParamTexture( 0, "screenColorTex", m_pELPAd );
			EtherLightAnimatorComponent::SetGlobalMultiplier( 1.3f );
			m_State = 3;
			m_StartTime = curTime;
		}
	} else if ( m_State == 3 ) {
		if ( duration > m_GlitchDuration ) {
			m_State = 4;
			m_pStaticModel->SetMaterialParamVector( 0, "lineSpeedAndRadius", kbVec4( 0.1f, 0.08f, 0.01f, 0.01f ) );
			m_StartTime = curTime;
		}
	} else if ( m_State == 4 ) {
			

		if ( duration > m_AdLength ) {
			m_State = 0;
			m_StartTime = curTime;
		}
	}
}
