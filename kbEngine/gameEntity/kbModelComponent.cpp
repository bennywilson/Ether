//===================================================================================================
// kbModelComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"		// <--- TODO: Temp, game entity should not be accessed from renderer
#include "kbModel.h"
#include "kbModelComponent.h"

KB_DEFINE_COMPONENT(kbModelComponent)

/**
 *	kbModelComponent::Constructor
 */
void kbModelComponent::Constructor() {
	m_RenderPass = RP_Lighting;
	m_bCastsShadow = false;
}

/**
 *	kbModelComponent::~kbModelComponent
 */
kbModelComponent::~kbModelComponent() {
}

/**
 *	kbModelComponent::SetShaderParam
 */
void kbModelComponent::SetShaderParam( const int index, const kbVec4 & newParam ) {
	if ( index >= 0 && index < m_ShaderParams.size() ) {
		m_ShaderParams[index] = newParam;
		GetOwner()->MarkAsDirty();
	}
}
