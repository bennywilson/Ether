//===================================================================================================
// kbCollisionManager.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbCollisionManager.h"
#include "kbIntersectionTests.h"
#include "kbConsole.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbCollisionComponent)

kbCollisionManager g_CollisionManager;

kbConsoleVariable g_ShowCollision( "showcollision", false, kbConsoleVariable::Console_Bool, "Show collision", "" );

/**
 *	kbCollisionComponent::Constructor
 */
void kbCollisionComponent::Constructor() {
	m_CollisionType = CollisionType_Sphere;
	m_Extent.Set( 10.0f, 10.0f, 10.0f );
}

/**
 *	kbCollisionComponent::~kbCollisionComponent
 */
kbCollisionComponent::~kbCollisionComponent() {
}

/**
 *	kbCollisionComponent::SetEnable_Internal
 */
void kbCollisionComponent::SetEnable_Internal( const bool isEnabled ) {
	if ( isEnabled ) {
		g_CollisionManager.RegisterComponent( this );
	} else {
		g_CollisionManager.UnregisterComponent( this );
	}
}

/**
 *	kbCollisionComponent::Update_Internal
 */
void kbCollisionComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( g_ShowCollision.GetBool() ) {
		const kbVec3 collisionCenter = GetOwner()->GetPosition();//, pCollision->m_Extent.x 
		if ( m_CollisionType == ECollisionType::CollisionType_Sphere ) {
			g_pRenderer->DrawSphere( collisionCenter, m_Extent.x, 12, kbColor::green );
		} else if ( m_CollisionType == ECollisionType::CollisionType_Box ) {
			g_pRenderer->DrawBox( kbBounds( collisionCenter - m_Extent, collisionCenter + m_Extent ), kbColor::green );
		}
	}
}

/**
 *	kbCollisionComponent::SetWorldSpaceCollisionSphere
 */
void kbCollisionComponent::SetWorldSpaceCollisionSphere( const int idx, const kbVec4 & newSphere ) {

	if ( idx < 0 || idx >= m_LocalSpaceCollisionSpheres.size() ) {
		kbError( "kbCollisionComponent::SetWorldSpaceCollisionSphere() - Invalid idx %d provided", idx );
		return;
	}
	
	if ( m_WorldSpaceCollisionSpheres.size() == 0 ) {
		m_WorldSpaceCollisionSpheres.resize( m_LocalSpaceCollisionSpheres.size() );
	}

	m_WorldSpaceCollisionSpheres[idx] = newSphere; 
}

/**
 *	kbCollisionComponent::SetCustomTriangleCollision
 */
void kbCollisionComponent::SetCustomTriangleCollision( const std::vector<customTriangle_t> & inCollision ) {

	if ( IsEnabled() ) {
		g_CollisionManager.UnregisterComponent( this );
	}

	m_CollisionType = CollisionType_CustomTriangles;
	m_CustomTriangleCollision = inCollision;

	if ( IsEnabled() ) {
		g_CollisionManager.RegisterComponent( this );
	}
}

/**
 *	kbCollisionManager::kbCollisionManager
 */
kbCollisionManager::kbCollisionManager() {
}

/**
 *	kbCollisionManager::~kbCollisionManager
 */
kbCollisionManager::~kbCollisionManager() {
	kbErrorCheck( m_CollisionComponents.size() == 0, "kbCollisionManager::~kbCollisionManager() - There are still %d registered components", (int)m_CollisionComponents.size() );
}

/**
 *	kbCollisionManager::PerformLineCheck
 */
kbCollisionInfo_t kbCollisionManager::PerformLineCheck( const kbVec3 & start, const kbVec3 & end ) {
	kbCollisionInfo_t collisionInfo;

	float LineLength = 0.0f;
	
	if ( end.Compare( start ) == false ) {
		LineLength = ( end - start ).Length();
	} else {
		// todo: Point check
		return collisionInfo;
	}

	const float oneOverLength = 1.0f / LineLength;
	const kbVec3 rayDir = ( end - start ) * oneOverLength;

	for ( int iCollisionComp = 0; iCollisionComp < m_CollisionComponents.size(); iCollisionComp++ ) {
		kbCollisionComponent *const pCollision = m_CollisionComponents[iCollisionComp];

		if ( pCollision->m_CollisionType == CollisionType_CustomTriangles ) {

			bool bHit = false;
			const std::vector<kbCollisionComponent::customTriangle_t> & triList = pCollision->m_CustomTriangleCollision;
			for ( int iTri = 0; iTri < triList.size(); iTri++ ) {
			
				const kbVec3 & v1 = triList[iTri].m_Vertex1;
				const kbVec3 & v2 = triList[iTri].m_Vertex2;
				const kbVec3 & v3 = triList[iTri].m_Vertex3;
				float t;
				if ( kbRayTriIntersection( t, start, rayDir, v1, v2, v3 ) ) {
					if ( t < collisionInfo.m_T && t >= 0 && t < LineLength ) {
						collisionInfo.m_T = t;
						bHit = true;
					}
				}
			}

			if ( bHit ) {
				collisionInfo.m_HitLocation = start + rayDir * collisionInfo.m_T;
				collisionInfo.m_pHitComponent = pCollision;
				collisionInfo.m_bHit = true;
			}

		} else if ( pCollision->m_CollisionType == CollisionType_StaticMesh ) {
			kbGameEntity *const pOwner = pCollision->GetOwner();
			kbStaticModelComponent *const pStaticModel = (kbStaticModelComponent*)pOwner->GetComponentByType( kbStaticModelComponent::GetType() );
			if ( pStaticModel == nullptr ) {
				kbWarning( "kbCollisionManager::PerformLineCheck() - Entity %s is missing a kbStaticModelComponent", pOwner->GetName().c_str() );
				continue;
			}
			kbModelIntersection_t intersection = pStaticModel->GetModel()->RayIntersection( start, rayDir, pOwner->GetPosition(), pOwner->GetOrientation() );
			if ( intersection.hasIntersection && intersection.t < LineLength && intersection.t < collisionInfo.m_T ) {
				collisionInfo.m_bHit = true;
				collisionInfo.m_HitLocation = start + rayDir * intersection.t;
				collisionInfo.m_T = intersection.t;
				collisionInfo.m_pHitComponent = pCollision;		
			}
		} else if ( pCollision->m_CollisionType == CollisionType_Sphere ) {
			kbGameEntity *const pCollisionOwner = pCollision->GetOwner();
			kbVec3 intersectionPt;
			if ( kbRaySphereIntersection( intersectionPt, start, rayDir, pCollisionOwner->GetPosition(), pCollision->m_Extent.x ) ) {
				const float t = ( intersectionPt - start ).Length() / LineLength;
				if ( t < collisionInfo.m_T ) {

					if ( pCollision->GetWorldSpaceCollisionSpheres().size() == 0 ) {
						collisionInfo.m_bHit = true;
						collisionInfo.m_HitLocation = intersectionPt;
						collisionInfo.m_T = t;
						collisionInfo.m_pHitComponent = pCollision;
					} else {
						for ( int iColSphere = 0; iColSphere < pCollision->GetWorldSpaceCollisionSpheres().size(); iColSphere++ ) {
							const kbVec4 & curSphere = pCollision->GetWorldSpaceCollisionSpheres()[iColSphere];
							if ( kbRaySphereIntersection( intersectionPt, start, rayDir, curSphere.ToVec3(), curSphere.a ) ) {
								const float innerT = ( intersectionPt - start ).Length() / LineLength;
								if ( innerT < collisionInfo.m_T ) {
									collisionInfo.m_bHit = true;
									collisionInfo.m_HitLocation = intersectionPt;
									collisionInfo.m_T = innerT;
									collisionInfo.m_pHitComponent = pCollision;									
								}
							}
						}
					}
				}
			}
		}
	}

	return collisionInfo;
}

/**
 *	kbCollisionManager::RegisterComponent
 */
void kbCollisionManager::RegisterComponent( kbCollisionComponent * Collision ) {
	if ( std::find( m_CollisionComponents.begin(), m_CollisionComponents.end(), Collision ) == m_CollisionComponents.end() ) {
		m_CollisionComponents.push_back( Collision );
	}
}

/**
 *	kbCollisionManager::UnregisterComponent
 */
void kbCollisionManager::UnregisterComponent( kbCollisionComponent * Collision ) {
	VectorRemoveFast( m_CollisionComponents, Collision );
}