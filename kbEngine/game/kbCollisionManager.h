/// kbCollisionManager.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "blk_core.h"

/// kbCollisionComponent
enum ECollisionType {
	CollisionType_Sphere,
	CollisionType_Box,
	CollisionType_StaticMesh,
	CollisionType_CustomTriangles
};

/// kbBoneCollisionSphere
class kbBoneCollisionSphere : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbBoneCollisionSphere, kbGameComponent );
	friend class kbClothComponent;

//---------------------------------------------------------------------------------------------------
public:
	const kbString &							GetBoneName() const { return m_BoneName; }
	const Vec4 &								GetSphere() const { return m_Sphere; }

private:
	kbString									m_BoneName;
	Vec4										m_Sphere;
};


 /// kbCollisionComponent
class kbCollisionComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT( kbCollisionComponent, kbGameComponent );
	friend class kbCollisionManager;

public:
	virtual										~kbCollisionComponent();

	const std::vector<kbBoneCollisionSphere> &	GetLocalSpaceCollisionSpheres() const { return m_LocalSpaceCollisionSpheres; }
	const std::vector<Vec4> &					GetWorldSpaceCollisionSpheres() const { return m_WorldSpaceCollisionSpheres; }
	void										SetWorldSpaceCollisionSphere( const int idx, const Vec4 & newSphere );

	float										GetRadius() const { return m_Extent.length(); }

	struct customTriangle_t {
		Vec3									m_Vertex1;
		Vec3									m_Vertex2;
		Vec3									m_Vertex3;
	};
	void										SetCustomTriangleCollision( const std::vector<customTriangle_t> & inCollision );


protected:
	virtual void								enable_internal( const bool isEnabled ) override;
	virtual void								update_internal( const float DeltaTime ) override;

private:
	ECollisionType								m_CollisionType;
	Vec3										m_Extent;

	std::vector<Vec4>							m_WorldSpaceCollisionSpheres;
	std::vector<kbBoneCollisionSphere>			m_LocalSpaceCollisionSpheres;
	std::vector<customTriangle_t>				m_CustomTriangleCollision;
};

/// kbCollisionInfo_t
struct kbCollisionInfo_t {
	kbCollisionInfo_t() :
		m_T( FLT_MAX ),
		m_pHitComponent( nullptr ),
		m_bHit( false ) { }

	Vec3				m_HitLocation;
	float				m_T;
	kbGameComponent *	m_pHitComponent;
	bool				m_bHit;
};

/// kbCollisionManager
class kbCollisionManager {

//---------------------------------------------------------------------------------------------------
public:
												kbCollisionManager();
												~kbCollisionManager();

	kbCollisionInfo_t							PerformLineCheck( const Vec3 & start, const Vec3 & end );

	void										RegisterComponent( kbCollisionComponent * Collision );
	void										UnregisterComponent( kbCollisionComponent * Collision );

private:
	std::vector<kbCollisionComponent*>			m_CollisionComponents;
};

extern kbCollisionManager g_CollisionManager;