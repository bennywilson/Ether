//===================================================================================================
// kbDebugComponents.cpp
//
//
// 2018-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbDebugComponents.h"
#include "DX11/kbRenderer_DX11.h"		// TODO

/**
 *	kbDebugSphereCollision::Constructor
 */
void kbDebugSphereCollision::Constructor() {
	m_pCollisionModel = (kbModel*)g_ResourceManager.GetResource( "../../kbEngine/assets/Models/UnitSphere.ms3d", true, true );

	m_RenderObject.m_bCastsShadow = false;
	m_RenderObject.m_bIsSkinnedModel = false;
	m_RenderObject.m_pComponent = this;
	m_RenderObject.m_pModel = m_pCollisionModel;
	m_RenderObject.m_RenderPass = RP_Lighting;
}

/**
 *	kbDebugSphereCollision::SetEnable_Internal
 */
void kbDebugSphereCollision::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	m_RenderObject.m_pModel = m_pCollisionModel;
	if ( bEnable ) {
		g_pRenderer->AddRenderObject( m_RenderObject );
	} else {
		g_pRenderer->RemoveRenderObject( m_RenderObject );
	}
}

/**
 *	kbDebugSphereCollision::Update_Internal
 */
void kbDebugSphereCollision::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();

	m_RenderObject.m_pModel = m_pCollisionModel;
	g_pRenderer->UpdateRenderObject( m_RenderObject );
}