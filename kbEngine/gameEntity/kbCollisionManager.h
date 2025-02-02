//===================================================================================================
// kbCollisionManager.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBCOLLISIONMANAGER_H_
#define _KBCOLLISIONMANAGER_H_

#include "kbCore.h"

/**
 *	kbCollisionComponent
 */
enum ECollisionType {
	CollisionType_Sphere,
	CollisionType_Box,
	CollisionType_StaticMesh,
	CollisionType_CustomTriangles
};

/**
 *	kbBoneCollisionSphere
 */
class kbBoneCollisionSphere : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbBoneCollisionSphere, kbGameComponent );
	friend class kbClothComponent;

//---------------------------------------------------------------------------------------------------
public:
	const kbString &							GetBoneName() const { return m_BoneName; }
	const kbVec4 &								GetSphere() const { return m_Sphere; }

private:
	kbString									m_BoneName;
	kbVec4										m_Sphere;
};

/**
 *	kbCollisionComponent
 */
class kbCollisionComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbCollisionComponent, kbGameComponent );
	friend class kbCollisionManager;

//---------------------------------------------------------------------------------------------------
public:
	virtual										~kbCollisionComponent();

	const std::vector<kbBoneCollisionSphere> &	GetLocalSpaceCollisionSpheres() const { return m_LocalSpaceCollisionSpheres; }
	const std::vector<kbVec4> &					GetWorldSpaceCollisionSpheres() const { return m_WorldSpaceCollisionSpheres; }
	void										SetWorldSpaceCollisionSphere( const int idx, const kbVec4 & newSphere );

	float										GetRadius() const { return m_Extent.Length(); }

	struct customTriangle_t {
		kbVec3									m_Vertex1;
		kbVec3									m_Vertex2;
		kbVec3									m_Vertex3;
	};
	void										SetCustomTriangleCollision( const std::vector<customTriangle_t> & inCollision );


protected:
	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:
	ECollisionType								m_CollisionType;
	kbVec3										m_Extent;

	std::vector<kbVec4>							m_WorldSpaceCollisionSpheres;
	std::vector<kbBoneCollisionSphere>			m_LocalSpaceCollisionSpheres;
	std::vector<customTriangle_t>				m_CustomTriangleCollision;
};

/**
 *	kbCollisionInfo_t
 */
struct kbCollisionInfo_t {
	kbCollisionInfo_t() :
		m_T( FLT_MAX ),
		m_pHitComponent( nullptr ),
		m_bHit( false ) { }

	kbVec3				m_HitLocation;
	float				m_T;
	kbGameComponent *	m_pHitComponent;
	bool				m_bHit;
};

/**
 *	kbCollisionManager
 */
class kbCollisionManager {

//---------------------------------------------------------------------------------------------------
public:
												kbCollisionManager();
												~kbCollisionManager();

	kbCollisionInfo_t							PerformLineCheck( const kbVec3 & start, const kbVec3 & end );

	void										RegisterComponent( kbCollisionComponent * Collision );
	void										UnregisterComponent( kbCollisionComponent * Collision );

private:
	std::vector<kbCollisionComponent*>			m_CollisionComponents;
};

extern kbCollisionManager g_CollisionManager;

#endif