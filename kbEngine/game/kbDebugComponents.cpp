/// kbDebugComponents.cpp
///
/// 2018-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbGameEntityHeader.h"
#include "kbDebugComponents.h"
#include "DX11/kbRenderer_DX11.h"		// TODO

/// kbDebugSphereCollision::Constructor
void kbDebugSphereCollision::Constructor() {
	m_pCollisionModel = (kbModel*)g_ResourceManager.GetResource( "../../kbEngine/assets/Models/UnitSphere.ms3d", true, true );

	m_render_object.m_casts_shadow = false;
	m_render_object.m_bIsSkinnedModel = false;
	m_render_object.m_pComponent = this;
	m_render_object.m_model = m_pCollisionModel;
	m_render_object.m_render_pass = RP_Lighting;
}

/// kbDebugSphereCollision::enable_internal
void kbDebugSphereCollision::enable_internal( const bool bEnable ) {
	Super::enable_internal( bEnable );

	m_render_object.m_model = m_pCollisionModel;
	if ( bEnable ) {
		g_pRenderer->AddRenderObject( m_render_object );
	} else {
		g_pRenderer->RemoveRenderObject( m_render_object );
	}
}

/// kbDebugSphereCollision::update_internal
void kbDebugSphereCollision::update_internal( const float DeltaTime ) {
	Super::update_internal( DeltaTime );

	m_render_object.m_position = GetOwner()->GetPosition();
	m_render_object.m_Orientation = GetOwner()->GetOrientation();
	m_render_object.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();

	m_render_object.m_model = m_pCollisionModel;
	g_pRenderer->UpdateRenderObject( m_render_object );
}