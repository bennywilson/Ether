//===================================================================================================
// kbGameEntityHeader.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef KBGAMEENTITYHEADER_H_
#define KBGAMEENTITYHEADER_H_

void CopyVarToComponent( const class kbComponent * Src, class kbComponent * Dst, const class kbTypeInfoVar * currentVar );

#define KB_DECLARE_COMPONENT( className, parentClassName ) \
private: \
	/*className & operator=( const className & componentToCopy );*/ \
	/*className & operator=( const className * componentToCopy );*/	\
	void Constructor(); \
	typedef parentClassName Super; \
	virtual void CollectAncestorTypeInfo() { CollectAncestorTypeInfo_Interal( className##_TypeInfoVar ); } \
	friend class className##_TypeInfo; \
	static className##_TypeInfo typeInfo; \
	static std::vector< class kbTypeInfoClass * > className##_TypeInfoVar; \
protected: \
	virtual void CollectAncestorTypeInfo_Interal( std::vector< class kbTypeInfoClass * > & collection ) { Super::CollectAncestorTypeInfo_Interal( collection ); collection.push_back( ( kbTypeInfoClass * )( &typeInfo ) ); } \
public: \
	className() { Constructor(); if ( GetTypeInfo().size() == 0 ) { CollectAncestorTypeInfo(); }} \
	/* className( const className & componentToCopy );*/ \
	virtual const char * GetComponentClassName() const { return #className; } \
	virtual const std::vector< class kbTypeInfoClass * > & GetTypeInfo() const { return className##_TypeInfoVar; } \
	virtual bool IsA( const void *const type ) const { if ( type != (kbTypeInfoClass*)( &typeInfo ) ) { return Super::IsA( type ); } else { return true; } } \
	const static className##_TypeInfo * GetType() { return &typeInfo; } \
	virtual kbComponent * Duplicate() const { return new className( *this ); }

#include "kbVector.h"
#include "kbComponent.h"
#include "kbModelComponent.h"
#include "kbStaticModelComponent.h"
#include "kbSkeletalModelComponent.h"
#include "kbParticleComponent.h"
#include "kbTerrainComponent.h"
#include "kbCollisionManager.h"
#include "kbLightComponent.h"
#include "kbClothComponent.h"
#include "kbGameEntity.h"
#include "kbRenderer_Defs.h"
#include "kbMaterial.h"
#include "kbSoundComponent.h"
#include "kbTypeInfo.h"
#include "kbFile.h"

#define KB_DEFINE_COMPONENT( className )

/*
className::className( const className & componentToCopy ) { \
	Constructor(); \
	for ( int i = 0; i < GetTypeInfo().size(); i++ ) { \
		typedef std::map< std::string, kbTypeInfoVar >::const_iterator iteratorType; \
		for (iteratorType it = GetTypeInfo()[i]->GetMemberFieldsMap().begin(); it != GetTypeInfo()[i]->GetMemberFieldsMap().end(); it++ ) { \
			CopyVarToComponent( &componentToCopy, this, &it->second); \
		} \
	} \
}*/
#endif