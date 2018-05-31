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
void kbModelComponent::SetShaderParam( const std::string & paramName, const kbShaderParamOverrides_t::kbShaderParam_t & shaderParam ) {

    for ( int i = 0; i < m_ShaderParams.m_ParamOverrides.size(); i++ ) {
        if ( m_ShaderParams.m_ParamOverrides[i].m_VarName == paramName ) {
            m_ShaderParams.m_ParamOverrides[i] = shaderParam;
            return;
        }
    }

    m_ShaderParams.m_ParamOverrides.push_back( shaderParam );
}
