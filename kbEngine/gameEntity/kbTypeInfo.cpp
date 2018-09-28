//==============================================================================
// kbTypeInfo.cpp
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer_Defs.h"
#include "kbMaterial.h"

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

ERenderPass_Enum ERenderPass_EnumClass;

EClothType_Enum EClothType_EnumClass;

ECollisionType_Enum ECollisionType_EnumClass;

EBillboardType_Enum EBillboardType_EnumClass;

// kbComponent
kbComponent_TypeInfo kbComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbComponent::kbComponent_TypeInfoVar;

// kbShaderParamComponent
kbShaderParamComponent_TypeInfo kbShaderParamComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbShaderParamComponent::kbShaderParamComponent_TypeInfoVar;

// kbGameComponent
kbGameComponent_TypeInfo kbGameComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbGameComponent::kbGameComponent_TypeInfoVar;

// kbModelComponent
kbModelComponent_TypeInfo kbModelComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbModelComponent::kbModelComponent_TypeInfoVar;

// kbStaticModelComponent
kbStaticModelComponent_TypeInfo kbStaticModelComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbStaticModelComponent::kbStaticModelComponent_TypeInfoVar;

// kbSkeletalModelComponent
kbSkeletalModelComponent_TypeInfo kbSkeletalModelComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbSkeletalModelComponent::kbSkeletalModelComponent_TypeInfoVar;

// kbGrass
kbGrass_TypeInfo kbGrass::typeInfo;
std::vector<kbTypeInfoClass *> kbGrass::kbGrass_TypeInfoVar;

// kbTerrainMatComponent
kbTerrainMatComponent_TypeInfo kbTerrainMatComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbTerrainMatComponent::kbTerrainMatComponent_TypeInfoVar;

// kbTerrainComponent
kbTerrainComponent_TypeInfo kbTerrainComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbTerrainComponent::kbTerrainComponent_TypeInfoVar;

// kbTransformComponent
kbTransformComponent_TypeInfo kbTransformComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbTransformComponent::kbTransformComponent_TypeInfoVar;

// kbLightComponent
kbLightComponent_TypeInfo kbLightComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbLightComponent::kbLightComponent_TypeInfoVar;

// kbDirectionalLightComponent
kbDirectionalLightComponent_TypeInfo kbDirectionalLightComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbDirectionalLightComponent::kbDirectionalLightComponent_TypeInfoVar;

// kbPointLightComponent
kbPointLightComponent_TypeInfo kbPointLightComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbPointLightComponent::kbPointLightComponent_TypeInfoVar;

// kbCylindricalComponent
kbCylindricalLightComponent_TypeInfo kbCylindricalLightComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbCylindricalLightComponent::kbCylindricalLightComponent_TypeInfoVar;

// kbLightShaftsComponent
kbLightShaftsComponent_TypeInfo kbLightShaftsComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbLightShaftsComponent::kbLightShaftsComponent_TypeInfoVar;

// kbFogComponent
kbFogComponent_TypeInfo kbFogComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbFogComponent::kbFogComponent_TypeInfoVar;

// kbParticleComponent
kbParticleComponent_TypeInfo kbParticleComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbParticleComponent::kbParticleComponent_TypeInfoVar;

// kbClothBone
kbClothBone_TypeInfo kbClothBone::typeInfo;
std::vector<kbTypeInfoClass *> kbClothBone::kbClothBone_TypeInfoVar;

// kbBoneCollisionSphere
kbBoneCollisionSphere_TypeInfo kbBoneCollisionSphere::typeInfo;
std::vector<kbTypeInfoClass *> kbBoneCollisionSphere::kbBoneCollisionSphere_TypeInfoVar;

// kbClothComponent
kbClothComponent_TypeInfo kbClothComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbClothComponent::kbClothComponent_TypeInfoVar;

// kbGameLogicComponent
kbGameLogicComponent_TypeInfo kbGameLogicComponent::typeInfo;
std::vector< kbTypeInfoClass *> kbGameLogicComponent::kbGameLogicComponent_TypeInfoVar;

// kbDamageComponent
kbDamageComponent_TypeInfo kbDamageComponent::typeInfo;
std::vector< kbTypeInfoClass *> kbDamageComponent::kbDamageComponent_TypeInfoVar;

// kbCollisionComponent
kbCollisionComponent_TypeInfo kbCollisionComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbCollisionComponent::kbCollisionComponent_TypeInfoVar;

// kbActorComponent
kbActorComponent_TypeInfo kbActorComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbActorComponent::kbActorComponent_TypeInfoVar;

// kbPlayerStartComponent
kbPlayerStartComponent_TypeInfo kbPlayerStartComponent::typeInfo;
std::vector<kbTypeInfoClass *> kbPlayerStartComponent::kbPlayerStartComponent_TypeInfoVar;

// kbSoundData
kbSoundData_TypeInfo kbSoundData::typeInfo;
std::vector<kbTypeInfoClass *> kbSoundData::kbSoundData_TypeInfoVar;

// kbDebugSphereCollision
kbDebugSphereCollision_TypeInfo kbDebugSphereCollision::typeInfo;
std::vector<kbTypeInfoClass *> kbDebugSphereCollision::kbDebugSphereCollision_TypeInfoVar;

typedef kbResource * kbResourcePtr;

void ReadInData( const std::string & fileName, void * classObject, const kbTypeInfoClass & typeInfo ) {
	ifstream inFile;
	inFile.open( fileName );

	if ( !inFile.is_open() ) {
		kbError( "ReadInData() - Failed to load", fileName );
	}

	char classType[256];
	char objectName[256];

	unsigned char * pClassObject = ( unsigned char * ) classObject;

	while( !inFile.eof() ) {
		inFile >> classType;
		inFile >> objectName;

		const kbTypeInfoVar * field = typeInfo.GetField( classType );

		if ( field != NULL ) {
			switch ( field->Type() ) {
				case KBTYPEINFO_SOUNDWAVE :
				case KBTYPEINFO_PTR :
				case KBTYPEINFO_TEXTURE :
				case KBTYPEINFO_STATICMODEL :
				case KBTYPEINFO_SHADER :
				{
					kbResource * pResource = g_ResourceManager.GetResource( objectName );

					if ( pResource == NULL ) {
						kbWarning( "ReadInData() - Resource %s was not found when loading in %s", objectName, fileName.c_str() );
						continue;
					}
			
					kbResourcePtr & pDestination = *( kbResourcePtr * ) ( pClassObject + field->Offset() );
					pDestination = pResource;
					break;
				}
			}
			/*if ( field->Type != KBTYPEINFO_PTR ) {
				memcpy( classObject 
			}*/
		} else {
			kbWarning( "Field %s does not exist, may be obsolete", classType );
		}
	}

	inFile.close();
}
