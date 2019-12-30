//==============================================================================
// kbTypeInfo.cpp
//
// 2016-2019 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer_Defs.h"
#include "kbMaterial.h"
#include "kbLevelComponent.h"

using namespace std;

kbNameToTypeInfoMap * g_NameToTypeInfoMap = nullptr;

/**
 *	kbNameToTypeInfoMap::kbNameToTypeInfoMap()
 */
kbNameToTypeInfoMap::kbNameToTypeInfoMap() {
	RegisterVectorOperations<kbString>( "kbString" );
	RegisterVectorOperations<float>( "float" );
	RegisterVectorOperations<kbVec4>( "kbVec4" );
}

/**
 *	kbNameToTypeInfoMap::~kbNameToTypeInfoMap()
 */
kbNameToTypeInfoMap::~kbNameToTypeInfoMap() {
}

/**
 *	kbNameToTypeInfoMap::AddTypeInfo()
 */
void kbNameToTypeInfoMap::AddTypeInfo( const kbTypeInfoClass *const classToAdd ) {
	m_Map[ classToAdd->GetClassName() ] = classToAdd;
}

/**
 *	kbNameToTypeInfoMap::AddEnum()
 */
void kbNameToTypeInfoMap::AddEnum( const std::string & enumName, const std::vector< std::string > & enumFields ) {
	m_EnumMap[ enumName ] = enumFields;
}

kbComponent * ConstructClassFromName( const std::string & className ) {
	const kbTypeInfoClass *const typeInfo = g_NameToTypeInfoMap->GetTypeInfoFromClassName( className );

	if ( typeInfo == nullptr ) {
		return nullptr;
	}

	return typeInfo->ConstructInstance();
}


// Should be a macro with each member specified

ECullMode_Enum ECullMode_EnumClass;

ERenderPass_Enum ERenderPass_EnumClass;

EClothType_Enum EClothType_EnumClass;

ECollisionType_Enum ECollisionType_EnumClass;

EBillboardType_Enum EBillboardType_EnumClass;

eWidgetAnchor_Enum EWidgetAnchor_EnumClass;

eWidgetAxisLock_Enum EWidgetAxisLock_EnumClass;

DEFINE_KBCLASS(kbComponent)

DEFINE_KBCLASS(kbEditorGlobalSettingsComponent)

DEFINE_KBCLASS(kbEditorLevelSettingsComponent)

DEFINE_KBCLASS(kbShaderParamComponent)

DEFINE_KBCLASS(kbMaterialComponent)

DEFINE_KBCLASS(kbGameComponent)

DEFINE_KBCLASS(kbAnimEvent)

DEFINE_KBCLASS(kbVectorAnimEvent)

DEFINE_KBCLASS(kbModelEmitter)

DEFINE_KBCLASS(kbModelComponent)

DEFINE_KBCLASS(kbStaticModelComponent)

DEFINE_KBCLASS(kbAnimComponent)

DEFINE_KBCLASS(kbSkeletalModelComponent)

DEFINE_KBCLASS(kbFlingPhysicsComponent)

DEFINE_KBCLASS(kbGrass)

DEFINE_KBCLASS(kbTerrainComponent)

DEFINE_KBCLASS(kbTransformComponent)

DEFINE_KBCLASS(kbLightComponent)

DEFINE_KBCLASS(kbDirectionalLightComponent)

DEFINE_KBCLASS(kbPointLightComponent)

DEFINE_KBCLASS(kbCylindricalLightComponent)

DEFINE_KBCLASS(kbLightShaftsComponent)

DEFINE_KBCLASS(kbFogComponent)

DEFINE_KBCLASS(kbParticleComponent)

DEFINE_KBCLASS(kbClothBone)

DEFINE_KBCLASS(kbBoneCollisionSphere)

DEFINE_KBCLASS(kbClothComponent)

DEFINE_KBCLASS(kbGameLogicComponent)

DEFINE_KBCLASS(kbDamageComponent)

DEFINE_KBCLASS(kbCollisionComponent)

DEFINE_KBCLASS(kbActorComponent)

DEFINE_KBCLASS(kbPlayerStartComponent)

DEFINE_KBCLASS(kbSoundData)

DEFINE_KBCLASS(kbDebugSphereCollision)

DEFINE_KBCLASS(kbLevelComponent)

DEFINE_KBCLASS(kbShaderModifierComponent)

DEFINE_KBCLASS(kbDeleteEntityComponent)

DEFINE_KBCLASS(kbPlaySoundComponent)

DEFINE_KBCLASS(kbUIComponent)

DEFINE_KBCLASS(kbUIWidgetComponent)

DEFINE_KBCLASS(kbUISlider)

typedef kbResource * kbResourcePtr;
