/// kbGameEntityHeader.h
///
/// 2016-2025 blk 1.0

#pragma once

void CopyVarToComponent(const class kbComponent* Src, class kbComponent* Dst, const class kbTypeInfoVar* currentVar);

#define DEFINE_KBCLASS(className) \
	class className##_TypeInfo className::typeInfo; \
	std::vector<class kbTypeInfoClass*> className::className##_TypeInfoVar; \

#define KB_DECLARE_COMPONENT(className, parentClassName) \
public: \
	className(const className&) = default; \
	className(className&&) = default; \
	className& operator=(const className&) = default; \
	className& operator=(className&&) = default; \
private: \
	void Constructor(); \
	typedef parentClassName Super; \
	virtual void CollectAncestorTypeInfo() { CollectAncestorTypeInfo_Internal( className##_TypeInfoVar ); } \
	friend class className##_TypeInfo; \
	static className##_TypeInfo typeInfo; \
	static std::vector< class kbTypeInfoClass * > className##_TypeInfoVar; \
protected: \
	virtual void CollectAncestorTypeInfo_Internal( std::vector< class kbTypeInfoClass * > & collection ) { Super::CollectAncestorTypeInfo_Internal( collection ); collection.push_back( ( kbTypeInfoClass * )( &typeInfo ) ); } \
public: \
	className() { Constructor(); if ( GetTypeInfo().size() == 0 ) { CollectAncestorTypeInfo(); }} \
	/* className( const className & componentToCopy );*/ \
	virtual const char * GetComponentClassName() const { return #className; } \
	virtual const std::vector< class kbTypeInfoClass * > & GetTypeInfo() const { return className##_TypeInfoVar; } \
	virtual bool IsA( const void *const type ) const { if ( type != (kbTypeInfoClass*)( &typeInfo ) ) { return Super::IsA( type ); } else { return true; } } \
	template<typename T> \
	T* GetAs() { if ( IsA( T::GetType() ) == false ) { return nullptr; } return (T*)this; } \
	const static className##_TypeInfo * GetType() { return &typeInfo; } \
	virtual kbComponent * Duplicate() const { return new className( *this ); }

#include "render_component.h"
#include "kbComponent.h"
#include "model_component.h"
#include "kbParticleComponent.h"
#include "kbTerrainComponent.h"
#include "kbCollisionManager.h"
#include "kbLightComponent.h"
#include "kbClothComponent.h"
#include "kbLevelComponent.h"
#include "kbUIComponent.h"
#include "kbSoundComponent.h"
#include "kbDebugComponents.h"
#include "kbTypeInfo.h"

#define KB_DEFINE_COMPONENT( className )
