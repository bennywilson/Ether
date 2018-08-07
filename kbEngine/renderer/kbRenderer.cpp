//==============================================================================
// kbRenderer.cpp
//
// Renderer implementation
//
// 2018 kbEngine 2.0
//==============================================================================
#include <stdio.h>
#include <math.h>
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbRenderer.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbLightComponent.h"
#include "kbConsole.h"

const float g_DebugLineSpacing = 0.0165f + 0.007f;
const float g_DebugTextSize = 0.0165f;

kbRenderer * g_pRenderer = nullptr;

/**
 *	kbRenderWindow::~kbRenderWindow
 */
kbRenderWindow::kbRenderWindow( HWND inHwnd, const RECT & windowDimensions, const float nearPlane, const float farPlane ) :
	m_Hwnd( inHwnd ),
	m_ViewPixelWidth( 0 ),
	m_ViewPixelHeight( 0 ),
	m_fViewPixelWidth( 0.0f ),
	m_fViewPixelHeight( 0.0f ),
	m_fViewPixelHalfWidth( 0.0f ),
	m_fViewPixelHalfHeight( 0.0f ) {

	m_ProjectionMatrix.MakeIdentity();
	m_InverseProjectionMatrix.MakeIdentity();
	m_ViewMatrix.MakeIdentity();
	m_ViewProjectionMatrix.MakeIdentity();
	m_InverseViewProjectionMatrix.MakeIdentity();
	m_CameraRotation.x = m_CameraRotation.y = m_CameraRotation.z = m_CameraRotation.w = 0.0f;

	m_ViewPixelWidth = windowDimensions.right - windowDimensions.left;
	m_ViewPixelHeight = windowDimensions.bottom - windowDimensions.top;
	m_fViewPixelWidth = static_cast<float>( m_ViewPixelWidth );
	m_fViewPixelHeight = static_cast<float>( m_ViewPixelHeight );
	m_fViewPixelHalfWidth = m_fViewPixelWidth* 0.5f;
	m_fViewPixelHalfHeight = m_fViewPixelHeight * 0.5f;
	m_ProjectionMatrix.CreatePerspectiveMatrix( kbToRadians( 75.0f ), m_fViewPixelWidth / m_fViewPixelHeight, nearPlane, farPlane );
}

/**
 *	kbRenderWindow::~kbRenderWindow
 */
kbRenderWindow::~kbRenderWindow() {
	
	{
		std::map< const kbComponent *, kbRenderObject * >::iterator iter;
	
		for ( iter = m_RenderObjectMap.begin(); iter != m_RenderObjectMap.end(); iter++ ) {
		   delete iter->second;
		}
	}
	
	{
		std::map< const kbLightComponent *, kbRenderLight * >::iterator iter;
	
		for ( iter = m_RenderLightMap.begin(); iter != m_RenderLightMap.end(); iter++ ) {
		   delete iter->second;
		}
	}
}


/**
 *	kbRenderer::SetRenderViewTransform
 */
void kbRenderer::SetRenderViewTransform( const HWND hwnd, const kbVec3 & position, const kbQuat & rotation ) {
	int viewIndex = -1;

	if ( hwnd == nullptr ) {
		viewIndex = 0;
	} else {
		for ( int i = 0 ; i < m_RenderWindowList.size(); i++ ) {
			if ( m_RenderWindowList[i]->GetHWND() == hwnd ) {
				viewIndex = i;
				break;
			}
		}
	}

	if ( viewIndex < 0 || viewIndex >= m_RenderWindowList.size() ) {
		kbError( "Invalid view index" );
	}

	m_RenderWindowList[viewIndex]->m_CameraPosition_GameThread = position;
	m_RenderWindowList[viewIndex]->m_CameraRotation_GameThread = rotation;
}

/**
 *	kbRenderer::GetRenderViewTransform
 */
void kbRenderer::GetRenderViewTransform( const HWND hwnd, kbVec3 & position, kbQuat & rotation ) {
	int viewIndex = -1;

	if ( hwnd == nullptr ) {
		viewIndex = 0;
	} else {
		for ( int i = 0 ; i < m_RenderWindowList.size(); i++ ) {
			if ( m_RenderWindowList[i]->m_Hwnd == hwnd ) {
				viewIndex = i;
				break;
			}
		}
	}

	if ( viewIndex < 0 || viewIndex >= m_RenderWindowList.size() ) {
		kbError( "Invalid view index" );
	}

	position = m_RenderWindowList[viewIndex]->m_CameraPosition;
	rotation = m_RenderWindowList[viewIndex]->m_CameraRotation;
}

/**
 *	kbRenderWindow::BeginFrame
 */
void kbRenderWindow::BeginFrame() {
	kbMat4 translationMatrix( kbMat4::identity );
	translationMatrix[3].ToVec3() = -m_CameraPosition;
	kbMat4 rotationMatrix = m_CameraRotation.ToMat4();
	rotationMatrix.TransposeSelf();
	
	m_ViewMatrix = translationMatrix * rotationMatrix;
	m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	
	BeginFrame_Internal();
}

/**
 *	kbRenderWindow::EndFrame
 */
void kbRenderWindow::EndFrame() {
	EndFrame_Internal();
}

/**
 *	kbRenderWindow::Release
 */
void kbRenderWindow::Release() {
	Release_Internal();
}

/**
 *	kbRenderJob::Run()
 */
void kbRenderJob::Run() {

	SetThreadName( "Render thread" );
	while( m_bRequestShutdown == false ) {
		if ( g_pRenderer != nullptr && g_pRenderer->m_RenderThreadSync == 1 ) {
			g_pRenderer->RenderScene();
			g_pRenderer->m_RenderThreadSync = 0;
		}
	}

	MarkJobAsComplete();
};

/**
 *	kbRenderer::kbRenderer
 */
kbRenderer::kbRenderer() :
	Back_Buffer_Width( 1280 ),
	Back_Buffer_Height( 1024 ),
	m_pCurrentRenderWindow( nullptr ),
	m_ViewMode_GameThread( ViewMode_Shaded ),
	m_ViewMode( ViewMode_Shaded ),
	m_FogColor_GameThread( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_FogColor_RenderThread( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_FogStartDistance_GameThread( 2100 ),
	m_FogStartDistance_RenderThread( 2200 ),
	m_FogEndDistance_GameThread( 2100 ),
	m_FogEndDistance_RenderThread( 2200 ),
	m_bConsoleEnabled( false ),
	m_pRenderJob( nullptr ),
	m_RenderThreadSync( 0 ) {

	g_pRenderer = this;


}

/**
 *	kbRenderer::~kbRenderer
 */
kbRenderer::~kbRenderer() {

}

/**
 *	kbRenderer::Init
 */
void kbRenderer::Init( HWND hwnd, const int width, const int height, const bool bUseHMD, const bool bUseHMDTrackingOnly ) {
	kbLog( "Initializing kbRenderer" );
	Init_Internal( hwnd, width, height, bUseHMD, bUseHMDTrackingOnly );
}

/**
 *	kbRenderer::LoadTexture
 */
void kbRenderer::LoadTexture( const char * name, int index, int width, int height ) {
	LoadTexture_Internal( name, index, width, height );
}

/**
 *	kbRenderer::AddRenderObject
 */
void kbRenderer::AddRenderObject( const kbRenderObject & renderObjectToAdd ) {
	kbErrorCheck( m_pCurrentRenderWindow != nullptr, "kbRenderer_DX11::AddRenderObject - NULL m_pCurrentRenderWindow" );
	kbErrorCheck( renderObjectToAdd.m_pComponent != nullptr, "kbRenderer_DX11::AddRenderObject - NULL component" );

	m_RenderObjectList_GameThread.push_back( renderObjectToAdd );
	kbRenderObject & renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = true;
	renderObj.m_bIsRemove = false;
}

/**
 *	kbRenderer_DX11::UpdateRenderObject
 */
void kbRenderer::UpdateRenderObject( const kbRenderObject & renderObjectToUpdate ) {
	kbErrorCheck( m_pCurrentRenderWindow != nullptr, "kbRenderer_DX11::UpdateRenderObject() - NULL m_pCurrentRenderWindow" );
	kbErrorCheck( renderObjectToUpdate.m_pComponent != nullptr, "kbRenderer_DX11::UpdateRenderObject() - NULL component" );

	m_RenderObjectList_GameThread.push_back( renderObjectToUpdate );
	kbRenderObject & renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = false;
	renderObj.m_bIsRemove = false;
}

/**
 *	kbRenderer::RemoveRenderObject
 */
void kbRenderer::RemoveRenderObject( const kbRenderObject & renderObjectToRemove ) {
	kbErrorCheck( m_pCurrentRenderWindow != nullptr, "kbRenderer_DX11::RemoveRenderObject() - NULL m_pCurrentRenderWindow" );
	kbErrorCheck( renderObjectToRemove.m_pComponent != nullptr, "kbRenderer_DX11::RemoveRenderObject - NULL component" );

	// Remove duplicates
	for ( int i = 0; i < m_RenderObjectList_GameThread.size(); i++ ) {
		if ( m_RenderObjectList_GameThread[i].m_pComponent == renderObjectToRemove.m_pComponent ) {
			m_RenderObjectList_GameThread.erase( m_RenderObjectList_GameThread.begin() + i );
			i--;
		}
	}

	m_RenderObjectList_GameThread.push_back( renderObjectToRemove );
	kbRenderObject & renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = false;
	renderObj.m_bIsRemove = true;
}

/*
 *	kbRenderer::DrawDebugText
 */
void kbRenderer::DrawDebugText( const std::string & theString, const float X, const float Y, const float ScreenCharW, const float ScreenCharH, const kbColor & Color ) {

	m_DebugStrings_GameThread.push_back( kbTextInfo_t() );

	kbTextInfo_t & newTextInfo = m_DebugStrings_GameThread[m_DebugStrings_GameThread.size() - 1];
	newTextInfo.TextInfo = theString;
	newTextInfo.screenX = X;
	newTextInfo.screenY = Y;
	newTextInfo.screenW = ScreenCharW;
	newTextInfo.screenH = ScreenCharH;
	newTextInfo.color = Color;
}

/**
 *	kbRenderer::AddLight
 */
void kbRenderer::AddLight( const kbLightComponent * pLightComponent, const kbVec3 & pos, const kbQuat & orientation ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::AddLight - nullptr Render Window" );
	}

	kbRenderLight newLight;

	newLight.m_pLightComponent = pLightComponent;
	newLight.m_Position = pos;
	newLight.m_Orientation = orientation;
	newLight.m_Radius = pLightComponent->GetRadius();
	newLight.m_Length = pLightComponent->GetLength();

	// If there are empty entries in the splits distance array, a value of FLT_MAX ensures the split won't be selected in the projection shader
	newLight.m_CascadedShadowSplits[0] = FLT_MAX;
	newLight.m_CascadedShadowSplits[1] = FLT_MAX;
	newLight.m_CascadedShadowSplits[2] = FLT_MAX;
	newLight.m_CascadedShadowSplits[3] = FLT_MAX;

	if ( pLightComponent->CastsShadow() && pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
		for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
			newLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	newLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	newLight.m_bCastsShadow = pLightComponent->CastsShadow();
	newLight.m_bIsFirstAdd = true;
	newLight.m_bIsRemove = false;
	m_LightList_GameThread.push_back( newLight );
}

/**
 *	kbRenderer::UpdateLight
 */
void kbRenderer::UpdateLight( const kbLightComponent * pLightComponent, const kbVec3 & pos, const kbQuat & orientation ) {

	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::UpdateLight - nullptr Render Window" );
	}

	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {
		if ( m_LightList_GameThread[i].m_pLightComponent == pLightComponent ) {
			if ( m_LightList_GameThread[i].m_bIsRemove == false ) {
				m_LightList_GameThread[i].m_Position = pos;
				m_LightList_GameThread[i].m_Orientation = orientation;
				m_LightList_GameThread[i].m_bCastsShadow = pLightComponent->CastsShadow();
				m_LightList_GameThread[i].m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
				m_LightList_GameThread[i].m_Radius = pLightComponent->GetRadius();
				m_LightList_GameThread[i].m_Length = pLightComponent->GetLength();

				memset( &m_LightList_GameThread[i].m_CascadedShadowSplits, 0, sizeof( m_LightList_GameThread[i].m_CascadedShadowSplits ) );
				if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
					const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
					for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
						m_LightList_GameThread[i].m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
					}
				}
			}
			return;
		}
	}
 
	kbRenderLight updateLight;
	updateLight.m_pLightComponent = pLightComponent;
	updateLight.m_Position = pos;
	updateLight.m_Orientation = orientation;
	updateLight.m_bIsFirstAdd = false;
	updateLight.m_bIsRemove = false;
	updateLight.m_bCastsShadow = pLightComponent->CastsShadow();
	updateLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	updateLight.m_Radius = pLightComponent->GetRadius();
	updateLight.m_Length = pLightComponent->GetLength();

	memset( &updateLight.m_CascadedShadowSplits, 0, sizeof( updateLight.m_CascadedShadowSplits ) );
	if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		const kbDirectionalLightComponent *const dirLight = static_cast<const kbDirectionalLightComponent*>( pLightComponent );
		for ( int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++ ) {
			updateLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	m_LightList_GameThread.push_back( updateLight );
}

/**
 *	kbRenderer::RemoveLight
 */
void kbRenderer::RemoveLight( const kbLightComponent *const pLightComponent ) {
	
	if ( m_pCurrentRenderWindow == nullptr ) {
		kbError( "kbRenderer_DX11::RemoveLight - nullptr Render Window" );
	}

	kbRenderLight lightToRemove;
	lightToRemove.m_pLightComponent = pLightComponent;
	lightToRemove.m_bIsRemove = true;
	m_LightList_GameThread.push_back( lightToRemove );
}

/**
 *	kbRenderer::HackClearLight
 */
void kbRenderer::HackClearLight( const kbLightComponent *const pLightComponent ) {
	
	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {
		if ( m_LightList_GameThread[i].m_pLightComponent == pLightComponent ) {
			m_LightList_GameThread.erase( m_LightList_GameThread.begin() + i );
			i--;
		}
	}

}

/**
 *	kbRenderer::AddParticle
 */
void kbRenderer::AddParticle( const void *const pParticleComponent, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation ) {
	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_pModel = pModel;
	NewParticle.m_RenderPass = RP_Translucent;
	NewParticle.m_Position = pos;
	NewParticle.m_Orientation = orientation;
	NewParticle.m_bIsFirstAdd = true;
	NewParticle.m_bIsRemove = false;

	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer_DX11::UpdateParticle
 */
void kbRenderer::UpdateParticle( const void *const pParticleComponent, const kbModel *const pModel, const kbVec3 & pos, kbQuat & orientation ) {

	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_pModel = pModel;
	NewParticle.m_RenderPass = RP_Translucent;
	NewParticle.m_Position = pos;
	NewParticle.m_Orientation = orientation;
	NewParticle.m_bIsFirstAdd = false;

	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer::RemoveParticle
 */
void kbRenderer::RemoveParticle( const void *const pParticleComponent ) {
	kbRenderObject NewParticle;
	NewParticle.m_pComponent = static_cast<const kbComponent*>( pParticleComponent );
	NewParticle.m_bIsFirstAdd = false;
	NewParticle.m_bIsRemove = true;
	m_ParticleList_GameThread.push_back( NewParticle );
}

/**
 *	kbRenderer::AddLightShafts
 */
void kbRenderer::AddLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation ) {
	kbLightShafts newLightShafts;
	newLightShafts.m_pLightShaftsComponent = pComponent;
	newLightShafts.m_pTexture = pComponent->GetTexture();
	newLightShafts.m_Color = pComponent->GetColor();
	newLightShafts.m_Width = pComponent->GetBaseWidth();
	newLightShafts.m_Height = pComponent->GetBaseHeight();
	newLightShafts.m_NumIterations = pComponent->GetNumIterations();
	newLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	newLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	newLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	newLightShafts.m_Pos = pos;
	newLightShafts.m_Rotation = orientation;
	newLightShafts.m_Operation = ROO_Add;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			m_LightShafts_GameThread.erase( m_LightShafts_GameThread.begin() );
			break;
		}
	}
	m_LightShafts_GameThread.push_back( newLightShafts );
}

/**
 *	kbRenderer_DX11::UpdateLightShafts
 */
void kbRenderer::UpdateLightShafts( const kbLightShaftsComponent *const pComponent, const kbVec3 & pos, const kbQuat & orientation ) {
	kbLightShafts updatedLightShafts;
	updatedLightShafts.m_pLightShaftsComponent = pComponent;
	updatedLightShafts.m_pTexture = pComponent->GetTexture();
	updatedLightShafts.m_Color = pComponent->GetColor();
	updatedLightShafts.m_Width = pComponent->GetBaseWidth();
	updatedLightShafts.m_Height = pComponent->GetBaseHeight();
	updatedLightShafts.m_NumIterations = pComponent->GetNumIterations();
	updatedLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	updatedLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	updatedLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	updatedLightShafts.m_Pos = pos;
	updatedLightShafts.m_Rotation = orientation;
	updatedLightShafts.m_Operation = ROO_Update;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			if ( m_LightShafts_GameThread[i].m_Operation == ROO_Remove ) {
				return;
			}
		}
	}
	m_LightShafts_GameThread.push_back( updatedLightShafts );
}

/**
 *	kbRenderer_DX11::RemoveLightShafts
 */
void kbRenderer::RemoveLightShafts( const kbLightShaftsComponent *const pComponent ) {
	kbLightShafts removeLightShafts;
	removeLightShafts.m_pLightShaftsComponent = pComponent;
	removeLightShafts.m_Operation = ROO_Remove;

	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent ) {
			m_LightShafts_GameThread.erase( m_LightShafts_GameThread.begin() );
			break;
		}
	}
	m_LightShafts_GameThread.push_back( removeLightShafts );
}

/**
 *	kbRenderer_DX11::UpdateFog
 */
void kbRenderer::UpdateFog( const kbColor & color, const float startDistance, const float endDistance ) {
	m_FogColor_GameThread = color;
	m_FogStartDistance_GameThread = startDistance;
	m_FogEndDistance_GameThread = endDistance;
}

/**
 *	kbRenderer_DX11::RenderSync
 */
void kbRenderer::RenderSync() {	

	// Copy requested game thread data over to their corresponding render thread structures
	m_DebugLines = m_DebugLines_GameThread;
	m_DebugLines_GameThread.clear();

	m_DebugBillboards = m_DebugBillboards_GameThread;
	m_DebugBillboards_GameThread.clear();

	m_DebugModels = m_DebugModels_GameThread;
	m_DebugModels_GameThread.clear();

	m_DebugStrings = m_DebugStrings_GameThread;
	m_DebugStrings_GameThread.clear();

	m_ScreenSpaceQuads_RenderThread = m_ScreenSpaceQuads_GameThread;
	m_ScreenSpaceQuads_GameThread.clear();

	m_ViewMode = m_ViewMode_GameThread;

	// Add/update render objects
	for ( int i = 0; i < m_RenderObjectList_GameThread.size(); i++ )
	{
		kbRenderObject * renderObject = nullptr;

		if ( m_RenderObjectList_GameThread[i].m_bIsRemove ) {
			kbRenderObject *const pRenderObject = m_pCurrentRenderWindow->m_RenderObjectMap[ m_RenderObjectList_GameThread[i].m_pComponent ];
			m_pCurrentRenderWindow->m_RenderObjectMap.erase( m_RenderObjectList_GameThread[i].m_pComponent );
			delete pRenderObject;
		} else {
			const kbComponent *const pComponent = m_RenderObjectList_GameThread[i].m_pComponent;
			kbErrorCheck( pComponent != nullptr, "kbRenderer_DX11::RenderSync() - Adding/updating a render object with a NULL component" );

			if ( m_RenderObjectList_GameThread[i].m_bIsFirstAdd == false ) {

				// Updating a renderobject 
				std::map< const kbComponent *, kbRenderObject * >::iterator it = m_pCurrentRenderWindow->m_RenderObjectMap.find( m_RenderObjectList_GameThread[i].m_pComponent );
				if ( it == m_pCurrentRenderWindow->m_RenderObjectMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist" );
				}

				renderObject = it->second;
				*renderObject = m_RenderObjectList_GameThread[i];
				if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) && renderObject->m_pModel->NumBones() > 0 ) {
					const kbSkeletalModelComponent *const skelComp = static_cast<const kbSkeletalModelComponent*>( pComponent );
					renderObject->m_MatrixList = skelComp->GetFinalBoneMatrices();
					renderObject->m_bIsSkinnedModel = true;
				}
			} else {

				// Adding new renderobject
				renderObject = m_pCurrentRenderWindow->m_RenderObjectMap[pComponent];
				kbErrorCheck( renderObject == nullptr, "kbRenderer_DX11::AddRenderObject() - Model %s already added", m_RenderObjectList_GameThread[i].m_pModel->GetFullName().c_str() );

				if ( pComponent->IsA( kbSkeletalModelComponent::GetType() ) && m_RenderObjectList_GameThread[i].m_pModel->NumBones() > 0 ) {

					renderObject = new kbRenderObject();
					*renderObject = m_RenderObjectList_GameThread[i];
					renderObject->m_bIsSkinnedModel = true;
					const kbSkeletalModelComponent *const skelComp = static_cast<const kbSkeletalModelComponent*>( pComponent );
					renderObject->m_MatrixList = skelComp->GetFinalBoneMatrices();
				} else {
					renderObject = new kbRenderObject;
					*renderObject = m_RenderObjectList_GameThread[i];
				}

				m_pCurrentRenderWindow->m_RenderObjectMap[m_RenderObjectList_GameThread[i].m_pComponent] = renderObject;
			}
		}
	}
	m_RenderObjectList_GameThread.clear();

	// Light
	for ( int i = 0; i < m_LightList_GameThread.size(); i++ ) {

		if ( m_LightList_GameThread[i].m_bIsRemove ) {

			kbRenderLight *const pRenderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];
			m_pCurrentRenderWindow->m_RenderLightMap.erase( m_LightList_GameThread[i].m_pLightComponent );
			delete pRenderLight;
		} else {
			kbRenderLight * renderLight = nullptr;

			bool bIsFirstAdd = m_LightList_GameThread[i].m_bIsFirstAdd;
			if ( bIsFirstAdd ) {
				renderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];

				if ( renderLight != nullptr ) {
					bIsFirstAdd = false;
					kbError( "kbRenderer_DX11::AddLight - Warning, adding a render light that already exists" );
				} else {
					renderLight = new kbRenderLight;
					m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent] = renderLight;
				}
			} else {

				std::map< const kbLightComponent *, kbRenderLight * >::iterator it = m_pCurrentRenderWindow->m_RenderLightMap.find( m_LightList_GameThread[i].m_pLightComponent );

				if ( it == m_pCurrentRenderWindow->m_RenderLightMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateLight - Error, Updating a RenderObject that doesn't exist" );
				} else {
					renderLight = it->second;
				}
			}

			*renderLight = m_LightList_GameThread[i];
		}
	}
	m_LightList_GameThread.clear();

	// Particles
	for ( int i = 0; i < m_ParticleList_GameThread.size(); i++ ) {
		const void *const pComponent = m_ParticleList_GameThread[i].m_pComponent;

		if ( m_ParticleList_GameThread[i].m_bIsRemove ) {
			kbRenderObject * renderParticle = m_pCurrentRenderWindow->m_RenderParticleMap[pComponent];
			m_pCurrentRenderWindow->m_RenderParticleMap.erase( pComponent );
			delete renderParticle;
		} else {
			kbRenderObject * renderParticle = nullptr;

			if ( m_ParticleList_GameThread[i].m_bIsFirstAdd ) {
				renderParticle = m_pCurrentRenderWindow->m_RenderParticleMap[pComponent];

				if ( renderParticle != nullptr ) {
					kbError( "kbRenderer_DX11::AddParticle - Adding a particle that already exists" );
				} else {
					renderParticle = new kbRenderObject;
					m_pCurrentRenderWindow->m_RenderParticleMap[pComponent] = renderParticle;
				}
			} else {
				std::map< const void *, kbRenderObject * >::iterator it = m_pCurrentRenderWindow->m_RenderParticleMap.find( pComponent );
				if ( it == m_pCurrentRenderWindow->m_RenderParticleMap.end() || it->second == nullptr ) {
					kbError( "kbRenderer_DX11::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist" );
				}

				 renderParticle = it->second;
			}

			*renderParticle = m_ParticleList_GameThread[i];
		}
	}
	m_ParticleList_GameThread.clear();

	// Light Shafts
	for ( int i = 0; i < m_LightShafts_GameThread.size(); i++ ) {
		if ( m_LightShafts_GameThread[i].m_Operation == ROO_Add ) {
			bool bAlreadyExists = false;
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					kbError( "kbRenderer_DX11::SetReadyToRender() - Adding light shafts that already exist" );
					bAlreadyExists = true;
					break;
				}
			}

			if ( bAlreadyExists == false ) {
				m_LightShafts_RenderThread.push_back( m_LightShafts_GameThread[i] );
			}
		} else if (  m_LightShafts_GameThread[i].m_Operation == ROO_Remove ) {
			bool bExists = false;
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					m_LightShafts_RenderThread.erase( m_LightShafts_RenderThread.begin() + j );
					bExists = true;
					break;
				}
			}

			if ( bExists == false ) {
				kbError( "kbRenderer_DX11::SetReadyToRender() - Removing light shafts that do not exist" );
			}
		} else {
			for ( int j = 0; j < m_LightShafts_RenderThread.size(); j++ ) {
				if ( m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent ) {
					m_LightShafts_RenderThread[j] = m_LightShafts_GameThread[i];
					break;
				}
			}
		}
	}

	m_LightShafts_GameThread.clear();

	// Camera
	for ( int i = 0; i < m_RenderWindowList.size(); i++ ) {

		m_RenderWindowList[i]->m_CameraPosition = m_RenderWindowList[i]->m_CameraPosition_GameThread;
		m_RenderWindowList[i]->m_CameraRotation = m_RenderWindowList[i]->m_CameraRotation_GameThread;
	}

	// Fog
	m_FogColor_RenderThread = m_FogColor_GameThread;
	m_FogStartDistance_RenderThread = m_FogStartDistance_GameThread;
	m_FogEndDistance_RenderThread = m_FogEndDistance_GameThread;

	RenderSync_Internal();
}

/**
 *	kbRenderer::DrawBillboard
 */
void kbRenderer::DrawBillboard( const kbVec3 & position, const kbVec2 & size, const int textureIndex, kbShader *const pShader, const int entityId ) {
	debugDrawObject_t billboard;
	billboard.m_Position = position;
	billboard.m_Scale.Set( size.x, size.y, size.x );
	billboard.m_pShader = pShader;
	billboard.m_TextureIndex = textureIndex;
	billboard.m_EntityId = entityId;

	m_DebugBillboards_GameThread.push_back( billboard );
}

/**
 *	kbRenderer::DrawModel
 */
void kbRenderer::DrawModel( const kbModel * pModel, const kbVec3 & position, const kbQuat & orientation, const kbVec3 & scale, const int entityId ) {
	debugDrawObject_t model;
	model.m_Position = position;
	model.m_Orientation = orientation;
	model.m_Scale = scale;
	model.m_pModel = pModel;
	model.m_EntityId = entityId;

	m_DebugModels_GameThread.push_back( model );
}

/**
 *	kbRenderer_DX11::DrawScreenSpaceQuad
 */
void kbRenderer::DrawScreenSpaceQuad( const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader *const pShader ) {
	ScreenSpaceQuad_t quadToAdd;
	quadToAdd.m_Pos.x = start_x;
	quadToAdd.m_Pos.y = start_y;
	quadToAdd.m_Size.x = size_x;
	quadToAdd.m_Size.y = size_y;
	quadToAdd.m_pShader = pShader;
	quadToAdd.m_TextureIndex = textureIndex;

	m_ScreenSpaceQuads_GameThread.push_back( quadToAdd );
}

#define AddVert( vert ) drawVert.position = vert; m_DebugLines_GameThread.push_back( drawVert );

/**
 *	kbRenderer_DX11::DrawLine
 */
void kbRenderer::DrawLine( const kbVec3 & start, const kbVec3 & end, const kbColor & color ) {

	/*if ( m_DebugLines_GameThread.size() >= m_DebugLines_GameThread.capacity() - 2 ) {
		return;
	}*/

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	AddVert( start );
	AddVert( end );
}

/**
 *	kbRenderer_DX11::DrawBox
 */
void kbRenderer::DrawBox( const kbBounds & bounds, const kbColor & color ) {

	const kbVec3 maxVert = bounds.Max();
	const kbVec3 minVert = bounds.Min();

	const kbVec3 LTF( minVert.x, maxVert.y, maxVert.z );
	const kbVec3 RTF( maxVert.x, maxVert.y, maxVert.z );
	const kbVec3 RBF( maxVert.x, minVert.y, maxVert.z );
	const kbVec3 LBF( minVert.x, minVert.y, maxVert.z );
	const kbVec3 LTB( minVert.x, maxVert.y, minVert.z );
	const kbVec3 RTB( maxVert.x, maxVert.y, minVert.z );
	const kbVec3 RBB( maxVert.x, minVert.y, minVert.z );
	const kbVec3 LBB( minVert.x, minVert.y, minVert.z );

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	AddVert( LTF ); AddVert( RTF );
	AddVert( RTF ); AddVert( RBF );
	AddVert( RBF ); AddVert( LBF );
	AddVert( LBF ); AddVert( LTF );

	AddVert( LTB ); AddVert( RTB );
	AddVert( RTB ); AddVert( RBB );
	AddVert( RBB ); AddVert( LBB );
	AddVert( LBB ); AddVert( LTB );

	AddVert( LTF ); AddVert( LTB );
	AddVert( RTF ); AddVert( RTB );
	AddVert( LBF ); AddVert( LBB );
	AddVert( RBF ); AddVert( RBB );
}

/**
 *	kbRenderer_DX11::DrawSphere
 */
void kbRenderer::DrawSphere( const kbVec3 & origin, const float radius, const int InNumSegments, const kbColor & color ) {
	const int numSegments = max( InNumSegments, 4 );
	const float angleInc = 2.0f * kbPI / (float) numSegments;
	float latitude = angleInc;
	float curSin = 0, curCos = 1.0f;
	//float cosX, sinX;
	kbVec3 pt1, pt2, pt3, pt4;

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	for ( int curYSeg = 0; curYSeg < numSegments; curYSeg++ ) {
		const float nextSin = sin( latitude );
		const float nextCos = cos( latitude );

		pt1 = kbVec3( curSin, curCos, 0.0f ) * radius + origin;
		pt3 = kbVec3( nextSin, nextCos, 0.0f ) * radius + origin;
		float longitude = angleInc;
		for ( int curXSeg = 0; curXSeg < numSegments; curXSeg++ ) {
			float sinX = sin( longitude );
			float cosX = cos( longitude );

			pt2 = kbVec3( cosX * curSin, curCos, sinX * curSin ) * radius + origin;
			pt4 = kbVec3( cosX * nextSin, nextCos, sinX * nextSin ) * radius + origin;
			AddVert( pt1 ); AddVert( pt2 );
			AddVert( pt1 ); AddVert( pt3 );
			pt1 = pt2;
			pt3 = pt4;
			longitude += angleInc;
		}

		curSin = nextSin;
		curCos = nextCos;
		latitude += angleInc;
	}
}


/*
 *	kbRenderer_DX11::DrawPreTransformedLine
 */
void kbRenderer::DrawPreTransformedLine( const std::vector<kbVec3> & vertList, const kbColor & color ) {
	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor( color );

	for ( int i = 0; i < vertList.size(); i++ ) {
		drawVert.position = vertList[i];
		m_DebugPreTransformedLines.push_back( drawVert );
	}
}