#ifndef _KBRAYTRACER_H_
#define _KBRAYTRACER_H_

#include "kbOctree.h"

/*
 * rayTraceInfo_t
 */
struct rayTraceInfo_t {
	rayTraceInfo_t() :
		m_FinalBuffer( NULL ),
		m_ColorBuffer( NULL ),
		m_PositionBuffer( NULL ),
		m_NormalBuffer( NULL ),
		m_BufferWidth( 0 ),
		m_BufferHeight( 0 ),
		m_ViewerPosition( 0.0f, 0.0f, 0.0f ) { }

	unsigned int *		m_FinalBuffer;
	const kbVec4 *		m_ColorBuffer;
	const kbVec4 *		m_PositionBuffer;
	const kbVec4 *		m_NormalBuffer;

	unsigned int		m_BufferWidth;
	unsigned int		m_BufferHeight;

	kbVec3				m_ViewerPosition;
};

/*
 * kbRayTraceLight
 */
class kbRayTraceLight {
public:
					kbRayTraceLight() :
						position( 0.0f, 0.0f, 0.0f ),
						direction( 0.0f, -1.0f, 0.0f ),
						color( 1.0f, 1.0f, 1.0f, 1.0f ),
						radius( 1.0f ),
						radiusSqr( 1.0f ),
						m_LightSourceRadius( 0.0f ) { }

	const kbVec3 &	GetPosition() const { return position; }
	const kbVec3 &	GetDirection() const { return direction; }
	float			GetRadius() const { return radius; }
	const kbVec4 &	GetColor() const { return color; }
	float			GetLightSourceRadius() const { return m_LightSourceRadius; }

	virtual void	SetPosition( const kbVec3 & newPosition ) { position = newPosition; }
	virtual void	SetDirection( const kbVec3 & newDirection ) { direction = newDirection; }
	void			SetRadius( const float newRadius ) { radius = newRadius, radiusSqr = newRadius * newRadius; }
	void			SetColor( const kbVec4 & newColor ) { color = newColor; }
	void			SetLightSourceRadius( const float newRadius ) { m_LightSourceRadius = newRadius; }

	virtual	kbVec4	LightPoint( const kbVec3 & testPoint, const kbVec3 & testNormal ) = 0;

protected:
	kbVec3			position;
	kbVec3			direction;
	kbVec4			color;
	float			radius;
	float			radiusSqr;
	float			m_LightSourceRadius;
};

/*
 * kbRayTracePointLight
 */
class kbRayTracePointLight : public kbRayTraceLight {
public:
	kbRayTracePointLight() { }

	virtual kbVec4 LightPoint( const kbVec3 & testPoint, const kbVec3 & testNormal ) {
		kbVec3 testPointToLight = position - testPoint;
		float distSqr = testPointToLight.LengthSqr();
		if ( distSqr > radiusSqr ) {
			return kbVec4( 0.0f, 0.0f, 0.0f, 1.0f );
		}
		
		const float distance = sqrt( distSqr );
		const float attenuation = 1.0f - distance / radius;
		testPointToLight.Normalize();
		const float nDotL = saturate( testNormal.Dot( testPointToLight ) );
		return kbVec4( color.x * attenuation * nDotL, color.y * attenuation * nDotL, color.z * attenuation * nDotL, 1.0f );
	}
};

/*
 * kbRayTraceDirectionalLight
 */
class kbRayTraceDirectionalLight : public kbRayTraceLight {
public:
	kbRayTraceDirectionalLight() { }

	void SetDirection( const kbVec3 & newDirection ) { 
		direction = newDirection;
		direction.Normalize();
		position = direction * -999999.0f;
	}

	virtual kbVec4 LightPoint( const kbVec3 & testPoint, const kbVec3 & testNormal ) {
		const float nDotL = max( 0, testNormal.Dot( -direction ) );
		return kbVec4( color.x * nDotL, color.y * nDotL, color.z * nDotL, 1.0f );
	}
};

struct photon_t {
	kbVec4	m_Color;
	kbVec3	m_Normal;
	kbVec3	m_Position;
	kbVec3	m_Direction;
	float	m_TravelDistance;
};

struct ray_t {
	ray_t() :
		color( 0.0f, 0.0f, 0.0f, 0.0f ) { }

	kbVec4	color;
	kbVec3	normal;
	kbVec3	position;
	kbVec3	viewerPos;
	float	reflection;
	float	refraction;
};

template <> 
class kbOctreeHelper< photon_t > {
public:
	bool IntersectsBounds( const kbBounds & bounds, const photon_t & element ) const {
		return bounds.ContainsPoint( element.m_Position );
	}
};

struct triHitInfo_t {
	triHitInfo_t() : 
		t( FLT_MAX ),
		uvw( 0.0f, 0.0f, 0.0f ),
		triIndex( -1 ) { }
	float			t;
	kbVec3			uvw;
	int				triIndex;
};

/*
 * kbRayTracer
 */
class kbRayTracer {
public:
										kbRayTracer();
										~kbRayTracer();

	void								AddModel( const kbModel * model );

	void								RenderScene( rayTraceInfo_t & rayTraceInfo );

private:

	float								PointIsInShadow( const kbVec3 & testPoint, const kbVec3 & testPointNormal, const unsigned int lightIndex );
	void								FirePhoton( const photon_t & photon, const int numBounces, const int triToSkip = -1 );
	kbVec4								GatherPhotons( const kbVec3 & position );

	void								TraceRay( ray_t & rayInfo, const int maxNumBounces );//kbVec3 & position, kbVec4 & color, kbVec3 & normal );
	bool								TestRayTriangleIntersection( const unsigned int triIndex, const kbVec3 & startPoint, const kbVec3 & direction, const float vecLength, triHitInfo_t & uvw );
	int									CastRay( const kbVec3 & startDirection, const kbVec3 & direction, kbVec3 & position, kbVec4 & color, kbVec3 & normal, kbVec4 & additionalProperties );

	const kbModel *						m_Model;
	std::vector< kbRayTraceLight * >	m_Lights;

	// temp - scene model will be fed into an acclerated structure of some kind
	struct rayTraceTri_t {
		unsigned int	v0;
		unsigned int	v1;
		unsigned int	v2;
		unsigned char	materialIndex;
	};

	struct rayTraceMat_t {
		kbVec4 *		textureData;
		unsigned int	textureWidth;
		unsigned int	textureHeight;

		kbVec4 *		materialData;		// r = reflection, g = refraction
	};

	std::vector< vertexLayOut >			m_SceneVertices;
	std::vector< rayTraceTri_t >		m_SceneTriangles;
	std::vector< rayTraceMat_t >		m_SceneMaterials;

	kbBounds							m_SceneBounds;

	kbOctree< photon_t, 8, 64 >			m_ScenePhotons;

	kbVec3								m_ViewerPosition;

public:
	struct debugLine_t {
		kbVec3 start;
		kbVec3 end;
		kbVec4 color;
	};

	std::vector< debugLine_t > debugLines;
	const std::vector< debugLine_t > & GetDebugLines() const { return debugLines; }
	const kbBounds & GetSceneBounds() const { return m_SceneBounds; }

private:
	void	DebugDrawPhotonInfo( const photon_t &  oldPhoton, const photon_t & newPhoton );
};

#endif