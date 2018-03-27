#include "kbCore.h"
#include "kbVector.h"
#include "kbModel.h"
#include "kbRayTracer.h"

const bool DEBUG_BOUNCES = false;
const bool DEBUG_PHOTON_HITS = false;
const float BUMP_SCALE_AMT = 0.25f;

unsigned int debugX, debugY;

const int NUM_FINAL_GATHER_RAYS = 256;
const int MAX_NUM_RAY_BOUNCES = 3;
const int NUM_PHOTON_BOUNCES = 10;
const int NUM_PHOTONS = 10000;
const float PHOTON_TRAVEL_ATTEN = 20.0f;
const float PHOTON_GATHER_EXTENT = 2.0f;
const float SHADOW_TEST_PULL = 0.02f;
const int NUM_SHADOW_POINTS_TO_TEST = 16;
const kbVec4 AMBIENT_COLOR = kbVec4( 0.0f, 0.0f, 0.0f, 0.0f );
bool skipRefract = false;

kbOctreeHelper< photon_t > photonOctreeHelper;

/*
 * kbRayTracer::kbRayTracer
 */
kbRayTracer::kbRayTracer() {
	//kbOctree< int, 8 >	octree;
	//kbOctreeHelper<double> someHelper;
	//octree.DoIt( someHelper );

	/*kbRayTraceLight * newLight = new kbRayTraceDirectionalLight;
	// newLight->SetDirection( kbVec3( -100.0f, -70.5f, -45.0f ) );
	newLight->SetDirection( kbVec3( -150.0f, -70.5f, 0.0f ) );
	newLight->SetRadius( 50.0f );
	newLight->SetColor( kbVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	m_Lights.push_back( newLight );*/

	/*{
		kbRayTraceLight * newLight = new kbRayTracePointLight;
		newLight->SetPosition( kbVec3( 0, 2, -17 ) );
		newLight->SetRadius( 50.0f );
		newLight->SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
		m_Lights.push_back( newLight );
	}*/

	{
		// Box map
		kbRayTraceLight * newLight = new kbRayTracePointLight;
		newLight->SetPosition( kbVec3( 0, 5, 0 ) );
		newLight->SetRadius( 50.0f );
		newLight->SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
		m_Lights.push_back( newLight );
	}
}

/*
 * kbRayTracer::~kbRayTracer
 */
kbRayTracer::~kbRayTracer() {

	for ( unsigned int i = 0; i < m_Lights.size(); i++ ) {
		delete m_Lights[i];
	}

	for ( unsigned int i = 0; i < m_SceneMaterials.size(); i++ ) {
		delete[] m_SceneMaterials[i].textureData;
		delete[] m_SceneMaterials[i].materialData;
	}
}

/*
 * kbRayTracer::RenderScene
 */	
void kbRayTracer::RenderScene( rayTraceInfo_t & rayTraceInfo ) {
	DWORD TickTime = GetTickCount();

	WriteToFile( "hello %10.5f %f \n", 12.3f, 9.1f );

	if ( rayTraceInfo.m_FinalBuffer != NULL ) {
		delete[] rayTraceInfo.m_FinalBuffer;
	}

	rayTraceInfo.m_FinalBuffer = new unsigned int[ rayTraceInfo.m_BufferWidth * rayTraceInfo.m_BufferHeight ];

	m_ViewerPosition = rayTraceInfo.m_ViewerPosition;

	// photon map
	const int NUM_PHOTONS_PER_LIGHT = NUM_PHOTONS / m_Lights.size();

	for ( unsigned int iLight = 0; iLight < m_Lights.size(); iLight++ ) {
		kbRayTraceDirectionalLight * pLight = dynamic_cast< kbRayTraceDirectionalLight * >( m_Lights[iLight] );
		if ( pLight != NULL ) {
			kbVec3 range = m_SceneBounds.Max() - m_SceneBounds.Min();
			kbVec3 center = m_SceneBounds.Center();
				
			for ( int iPhotons = 0; iPhotons < NUM_PHOTONS_PER_LIGHT; iPhotons++ ) {
				kbVec3 startPhoton = ( GetRandomVector()  * 2.0f ) - 1.0f;
				
				startPhoton.MultiplyComponents( range );
				startPhoton += center;
				startPhoton = startPhoton + ( pLight->GetDirection() * -9999.0f );

				photon_t photon;
				photon.m_Position = startPhoton;
				photon.m_Direction = pLight->GetDirection();
				photon.m_Normal = pLight->GetDirection();
				photon.m_Color = pLight->GetColor();
				photon.m_TravelDistance = 99999.0f;
				FirePhoton( photon, NUM_PHOTON_BOUNCES );	
			}
		}

		kbRayTracePointLight * pointLight = dynamic_cast< kbRayTracePointLight * >( m_Lights[iLight] );

		if ( pointLight != NULL ) {
			const int PHOTONS_PER_SIDE = NUM_PHOTONS_PER_LIGHT / 6;
			const kbVec3 normals[] = { kbVec3( 0, 0, 1 ), kbVec3( 0, 0, -1 ), kbVec3( 0, 1, 0 ), kbVec3( 0, -1, 0 ), kbVec3( 1, 0, 0 ), kbVec3( -1, 0, 0 ) };

			for ( int iSide = 0; iSide < 6; iSide++ ) {
				const float sqrtNSamples = sqrt( ( float ) PHOTONS_PER_SIDE );
				const float oneOverSqrtN = 1.0f / sqrtNSamples;
				const kbVec3 normal = normals[iSide];

				kbMatrix transform;
				transform.MakeMatrixFromZAxis( normal );
				kbVec4 irradiance( 0.0f, 0.0f, 0.0f, 0.0f );
			
				for ( int a = 0; a < sqrtNSamples; a++ ) {
					for ( int b = 0; b < sqrtNSamples; b++ ) {
						const float frand1 = rand() / ( float ) RAND_MAX;
						const float frand2 = rand() / ( float ) RAND_MAX;

						const float x = ( a + frand1 ) * oneOverSqrtN;
						const float y = ( b + frand2 ) * oneOverSqrtN;
				
						const float theta = 2.0f * acos( sqrt( 1.0f - x ) );
						const float phi = 2.0f * 3.14159f * y;

						kbVec3 photonDir( sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) );
						photonDir = photonDir * transform;
						photonDir.Normalize();

						photon_t photon;
						photon.m_Position = pointLight->GetPosition() + ( SHADOW_TEST_PULL  * photonDir );
						photon.m_Direction = photonDir;
						photon.m_Normal = photonDir;
						photon.m_Color = pointLight->GetColor();
						photon.m_TravelDistance = 99999.0f;
						FirePhoton( photon, NUM_PHOTON_BOUNCES );
					}
				}
			}	// end of checking 6 sides
		}
	}

	// ray-trace
	for ( unsigned int currentIndex = 0; currentIndex < rayTraceInfo.m_BufferWidth * rayTraceInfo.m_BufferHeight; currentIndex++ ) {
		debugX = currentIndex % rayTraceInfo.m_BufferWidth;
		debugY = currentIndex / rayTraceInfo.m_BufferWidth;

		ray_t rayInfo;
		rayInfo.position = rayTraceInfo.m_PositionBuffer[currentIndex].ToVec3();
		rayInfo.normal = rayTraceInfo.m_NormalBuffer[currentIndex].ToVec3();
		rayInfo.color = rayTraceInfo.m_ColorBuffer[currentIndex];
		rayInfo.reflection = rayTraceInfo.m_PositionBuffer[currentIndex].w;
		rayInfo.refraction = rayTraceInfo.m_NormalBuffer[currentIndex].w;
		rayInfo.viewerPos = rayTraceInfo.m_ViewerPosition;

		// check if there's an area of the screen that hasn't been drawn to for some reason
		if ( rayInfo.normal.LengthSqr() < 0.5f ) {
			continue;
		}

		rayInfo.normal.Normalize();
		skipRefract = false;
		TraceRay( rayInfo, MAX_NUM_RAY_BOUNCES );

		// convert final color into 4-byte argb
		unsigned int r = ( unsigned int ) min( 255.0f, rayInfo.color.x * 255.0f );
		unsigned int g = ( unsigned int ) min( 255.0f, rayInfo.color.y * 255.0f );
		unsigned int b = ( unsigned int ) min( 255.0f, rayInfo.color.z * 255.0f );
		rayTraceInfo.m_FinalBuffer[currentIndex] = ( 0 << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	DWORD finalTime = GetTickCount() - TickTime;

	char finalmsg[256];

	sprintf_s( finalmsg, "Rendering took %d ms\n", finalTime );
	WriteToFile( finalmsg);

	//debugLine_t a;
	//a.start.Set( -2.856164 -5.056787 -2.661420, 
}

/*
 * kbRayTracer::FirePhoton
 */
void kbRayTracer::FirePhoton( const photon_t & photon, const int numBounces, const int triToSkip ) {

	if ( numBounces == 0 ) {
		return;
	}

	// Check for photon collision
	photon_t newPhoton;
	kbVec4 dummyMat;
	int hitTriangleIndex = CastRay( photon.m_Position, photon.m_Direction, newPhoton.m_Position, newPhoton.m_Color, newPhoton.m_Normal, dummyMat );

	if ( hitTriangleIndex >= 0 ) {

		if ( newPhoton.m_Color.x < 0.0f || newPhoton.m_Color.y < 0.0f || newPhoton.m_Color.z < 0.0f ) {
			Error( "Bad Color" );
		}

		if ( newPhoton.m_Color.z < 0.5f ) {
			newPhoton.m_Color *= 3.0f;
		}

		newPhoton.m_Color.MultiplyComponents( photon.m_Color );

		const float travelledDist = ( photon.m_Position - newPhoton.m_Position ).Length();
		const float nDotL = saturate( -photon.m_Direction.Dot( newPhoton.m_Normal ) );
		const float attenuation = saturate( travelledDist / PHOTON_TRAVEL_ATTEN );
		newPhoton.m_Color *= ( 1.0f - attenuation ) * nDotL;

		if ( newPhoton.m_Color.ToVec3().LengthSqr() > 0.01f ) {	

			if ( numBounces != NUM_PHOTON_BOUNCES ) {
				m_ScenePhotons.AddElement( photonOctreeHelper, newPhoton );
				DebugDrawPhotonInfo( photon, newPhoton );
			}

			if ( numBounces > 1 ) {
				const kbVec3 randomVec = ( ( GetRandomVector() * 2.0f ) - 1.0f ) * BUMP_SCALE_AMT;					
				kbVec3 hitNormal = newPhoton.m_Normal + randomVec;

				if ( hitNormal.LengthSqr() < 0.0001f ) {
					hitNormal = newPhoton.m_Normal;
				} else {
					hitNormal.Normalize();
				}

				kbVec3 reflectVector = ReflectVector( hitNormal, -photon.m_Direction );
				reflectVector.Normalize();

				newPhoton.m_Direction = reflectVector;
				newPhoton.m_TravelDistance = photon.m_TravelDistance;
				const float pull = -0.1f;
				newPhoton.m_Position += newPhoton.m_Direction * pull;
				FirePhoton( newPhoton, numBounces - 1, hitTriangleIndex );
			}
		}
	}
}

/*
 * kbRayTracer::PointIsInShadow
 */
float kbRayTracer::PointIsInShadow( const kbVec3 & testPoint, const kbVec3 & testPointNormal, const unsigned int lightIndex ) {

	const float tempRad = 0.33f;

	kbVec3 pointsToCheck[ NUM_SHADOW_POINTS_TO_TEST ];

	pointsToCheck[0] = m_Lights[lightIndex]->GetPosition();
	for ( int i = 1; i < NUM_SHADOW_POINTS_TO_TEST; i++ ) {
		pointsToCheck[i] = m_Lights[lightIndex]->GetPosition() + ( GetRandomVector() * tempRad );
	}

	float shadowAmt = 0.0f;
	for ( int i = 0; i < NUM_SHADOW_POINTS_TO_TEST; i++ ) {
		kbVec3 d = pointsToCheck[i] - testPoint;
		d.Normalize();
		float vecLen = ( pointsToCheck[i] - testPoint ).Length();

		kbVec3 smallD( d.x * SHADOW_TEST_PULL, d.y * SHADOW_TEST_PULL, d.z * SHADOW_TEST_PULL );
		kbVec3 o = testPoint + smallD;

		for ( unsigned int iTris = 0; iTris < m_SceneTriangles.size(); iTris++ ) {
			triHitInfo_t hitInfo;
			if ( TestRayTriangleIntersection( iTris, o, d, vecLen, hitInfo ) ) {

				/**********************************************************************************************************/
				kbVec3 normal = m_SceneVertices[ m_SceneTriangles[hitInfo.triIndex].v0 ].GetNormal() * hitInfo.uvw.x;
				normal  += m_SceneVertices[ m_SceneTriangles[hitInfo.triIndex].v1 ].GetNormal() * hitInfo.uvw.y;
				normal  += m_SceneVertices[ m_SceneTriangles[hitInfo.triIndex].v2 ].GetNormal() * hitInfo.uvw.z;
				normal.Normalize();

				if ( normal.Dot( testPointNormal ) > 0.98f ) {
					static int breakHere = 0;
					breakHere++;
					continue;
				}
				/**********************************************************************************************************/

				shadowAmt += 1.0f;
				break;
			}
		}
	} 

	return shadowAmt / NUM_SHADOW_POINTS_TO_TEST;
}

/*
 * kbRayTracer::TestRayTriangleIntersection
 */
bool kbRayTracer::TestRayTriangleIntersection( const unsigned int triIndex, const kbVec3 & o, const kbVec3 & d, const float vecLen, triHitInfo_t & hitInfo ) {
	const kbVec3 v0 = m_SceneVertices[ m_SceneTriangles[triIndex].v0 ].position;
	const kbVec3 v1 = m_SceneVertices[ m_SceneTriangles[triIndex].v1 ].position;
	const kbVec3 v2 = m_SceneVertices[ m_SceneTriangles[triIndex].v2 ].position;

	const kbVec3 e1 = v1 - v0;
	const kbVec3 e2 = v2 - v0;
	const kbVec3 p = d.Cross( e2 );
	const float a = e1.Dot( p );
	const float epsilon = 0.0001f;
	if ( a > -epsilon && a < epsilon ) {
		return false;
	}
	const float f = 1.0f / a;
	const kbVec3 s = o - v0;
	const float u = f * ( s.Dot( p ) );
	if ( u < 0.0f || u > 1.0f ) {
		return false;
	}
	const kbVec3 q = s.Cross( e1 );
	float v = f * ( d.Dot( q ) );

	if ( v < 0.0f || ( u + v ) > 1.0f ) {
		return false;
	}

	const float t = f * ( e2.Dot( q ) );

	if ( t >= vecLen || t < 0.0f ) {
		return false;
	}

	hitInfo.t = t;
	hitInfo.uvw.Set( ( 1.0f - u - v ), u, v );
	hitInfo.triIndex = triIndex;

	return true;
}

/*
 * kbRayTracer::AddModel
 */
void kbRayTracer::AddModel( const kbModel * model ) {
	m_Model = model;

	m_SceneVertices = m_Model->GetVertices();
	m_SceneBounds.Reset();

	const std::vector< kbModel::mesh_t > & meshes = m_Model->GetMeshes();
	for ( unsigned int iMesh = 0; iMesh < meshes.size(); iMesh++ ) {

		for ( unsigned int iVerts = 0; iVerts < meshes[iMesh].m_VertIndices.size(); iVerts += 3 ) {
			rayTraceTri_t newTri;
			newTri.v0 = meshes[iMesh].m_VertIndices[iVerts + 0];
			newTri.v1 = meshes[iMesh].m_VertIndices[iVerts + 1];
			newTri.v2 = meshes[iMesh].m_VertIndices[iVerts + 2];
			newTri.materialIndex = meshes[iMesh].m_MaterialIndex;
			m_SceneTriangles.push_back( newTri );
		}
	}

	for ( unsigned int iVerts = 0; iVerts < m_SceneVertices.size(); iVerts++ ) {
		m_SceneBounds.AddPoint( m_SceneVertices[iVerts].position );
	}

	for ( unsigned int iMat = 0; iMat < model->GetMaterials().size(); iMat++ ) {
		rayTraceMat_t newMat;
		newMat.textureWidth = model->GetMaterials()[iMat].m_Width;
		newMat.textureHeight = model->GetMaterials()[iMat].m_Height;

		newMat.textureData = new kbVec4[newMat.textureWidth * newMat.textureHeight];
		memcpy( newMat.textureData, model->GetMaterials()[iMat].m_RawDiffuseData, sizeof( kbVec4 ) * newMat.textureWidth * newMat.textureHeight );

		newMat.materialData = new kbVec4[newMat.textureWidth * newMat.textureHeight];
		memcpy( newMat.materialData, model->GetMaterials()[iMat].m_RawReflectionData, sizeof( kbVec4 ) * newMat.textureWidth * newMat.textureHeight );
		m_SceneMaterials.push_back( newMat );
	}
}

/*
 * kbRayTracer::DebugDrawPhotonInfo
 */
void kbRayTracer::DebugDrawPhotonInfo( const photon_t & oldPhoton, const photon_t & newPhoton ) {

	if ( DEBUG_BOUNCES ) {
		debugLine_t newLine;
		newLine.start = oldPhoton.m_Position;
		newLine.end = newPhoton.m_Position;
		newLine.color.Set( 0.0f, 1.0f, 0.0f, 1.0f );
		debugLines.push_back( newLine );
	}

	if ( DEBUG_PHOTON_HITS ) {
		debugLine_t newLine;
		newLine.start = newPhoton.m_Position;
		newLine.end = newLine.start + kbVec3( 0.0f, 0.1f, 0.0f );
		newLine.color = newPhoton.m_Color;
		debugLines.push_back( newLine );
	}
}

/*
 * kbRayTracer::GatherPhotons
 */
kbVec4 kbRayTracer::GatherPhotons( const kbVec3 & worldPos ) {

	if ( NUM_PHOTONS == 0 ) {
		return kbVec4( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	kbVec4 irradiance( 0.0f, 0.0f, 0.0f, 0.0f );
	const kbVec3 vecExtent( PHOTON_GATHER_EXTENT, PHOTON_GATHER_EXTENT, PHOTON_GATHER_EXTENT );

	const kbBounds testBounds( worldPos - vecExtent, worldPos + vecExtent );
	std::vector< photon_t * > photonList;
	const int numIntersections = m_ScenePhotons.GetElementsWithinBounds( testBounds, photonOctreeHelper, photonList );

	if ( photonList.size() > 0 ) {
		for ( unsigned int i = 0; i < photonList.size(); i++ ) {
			irradiance += photonList[i]->m_Color;
		}

		irradiance /= ( float ) photonList.size();
	}
	return irradiance;
}

/*
 * kbRayTracer::CastRay
 */
int kbRayTracer::CastRay( const kbVec3 & startPosition, const kbVec3 & rayDirection, kbVec3 & position, kbVec4 & color, kbVec3 & normal, kbVec4 & additionalProperties ) {
	const float pushAmount = SHADOW_TEST_PULL;

	triHitInfo_t nearestHitInfo;

	for ( unsigned int iTris = 0; iTris < m_SceneTriangles.size(); iTris++ ) {
		triHitInfo_t hitInfo;
		if ( TestRayTriangleIntersection( iTris, startPosition + ( rayDirection * pushAmount ), rayDirection, 99999.0f, hitInfo ) ) {
			if ( hitInfo.triIndex != -1 && hitInfo.t < nearestHitInfo.t ) {
				nearestHitInfo = hitInfo;
			}
		}
	}

	if ( nearestHitInfo.triIndex != -1 ) {
		position = startPosition + ( rayDirection * nearestHitInfo.t );

		normal = m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v0 ].GetNormal() * nearestHitInfo.uvw.x;
		normal  += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v1 ].GetNormal() * nearestHitInfo.uvw.y;
		normal  += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v2 ].GetNormal() * nearestHitInfo.uvw.z;

		// uvs
		kbVec2 st;
		st.x  = m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v0 ].uv.x * nearestHitInfo.uvw.x;
		st.x += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v1 ].uv.x * nearestHitInfo.uvw.y;
		st.x += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v2 ].uv.x * nearestHitInfo.uvw.z;

		st.y  = m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v0 ].uv.y * nearestHitInfo.uvw.x;
		st.y += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v1 ].uv.y * nearestHitInfo.uvw.y;
		st.y += m_SceneVertices[ m_SceneTriangles[nearestHitInfo.triIndex].v2 ].uv.y * nearestHitInfo. uvw.z;

		const unsigned char materialIndex = m_SceneTriangles[nearestHitInfo.triIndex].materialIndex;
		unsigned int finalU = static_cast< unsigned int >( st.x * ( m_SceneMaterials[materialIndex].textureWidth - 1 ) );
		unsigned int finalV = static_cast< unsigned int >( st.y * ( m_SceneMaterials[materialIndex].textureHeight - 1 ) );

		if ( finalU > m_SceneMaterials[materialIndex].textureWidth - 1 ) {
			finalU = m_SceneMaterials[materialIndex].textureWidth - 1;
		}

		if ( finalV > m_SceneMaterials[materialIndex].textureHeight - 1 ) {
			finalV = m_SceneMaterials[materialIndex].textureHeight - 1;
		}

		// get texture color
		color = m_SceneMaterials[materialIndex].textureData[ finalU + ( finalV * m_SceneMaterials[materialIndex].textureWidth ) ];		

		additionalProperties = m_SceneMaterials[materialIndex].materialData[ finalU + ( finalV * m_SceneMaterials[materialIndex].textureWidth ) ];

		if ( skipRefract && additionalProperties.y > 0.0f ) {
			CastRay( position + ( rayDirection * 0.01f ), rayDirection, position, color, normal, additionalProperties );
		}
	}
	return nearestHitInfo.triIndex;
}

/*
 * kbRayTracer::CastRay
 */
void kbRayTracer::TraceRay( ray_t & rayInfo, const int maxNumBounces ) {
	if ( maxNumBounces == 0 ) {
		return;
	}

	// Calculate reflection
	kbVec4 reflectionRayColor( 0.0f, 0.0f, 0.0f, 0.0f );
	if ( rayInfo.reflection > 0.0f ) {
		rayInfo.normal.Normalize();
		kbVec3 reflectVector = ReflectVector( rayInfo.normal, ( rayInfo.viewerPos - rayInfo.position ).GetNormalizedVector() );

		reflectVector.Normalize();
		ray_t reflectionRay;

		// fixme: bias too high
		kbVec4 dummyMat;
		if ( CastRay( rayInfo.position + ( reflectVector * 0.5f ), reflectVector, reflectionRay.position, reflectionRay.color, reflectionRay.normal, dummyMat ) >= 0 ) {
			reflectionRay.normal.Normalize();
			reflectionRay.reflection = dummyMat.x;
			reflectionRay.refraction = dummyMat.y;
			reflectionRay.viewerPos = rayInfo.viewerPos;

			TraceRay( reflectionRay, maxNumBounces - 1 );
			reflectionRayColor = reflectionRay.color;
		} else {
			reflectionRayColor.Set( 1.0f, 0.0f, 1.0f, 0.0f );
		}
	}

	// Calculate refraction
	kbVec4 refractionColor( 0.0f, 0.0f, 0.0f, 0.0f );
	if ( rayInfo.refraction > 0.0f ) {
		skipRefract = true;
		rayInfo.color.Set( 1.0f, 0.0f, 1.0f, 1.0f );

		kbVec3 normal = rayInfo.normal;
		normal.Normalize();
		// hack need to tell what side of a face was hit

		float refractionIndex1 = 1.0f;
		float refractionIndex2 = 1.1f;
		float r = refractionIndex1 / refractionIndex2;
		
		/*if ( inside ) {
			normal = -normal;
		//	r = refractionIndex2 / refractionIndex1;
		}*/
		kbVec3 incomingRay = ( rayInfo.position - rayInfo.viewerPos ).GetNormalizedVector();
		float w = -incomingRay.Dot( normal ) * r;
		float k = sqrt( 1.0f + ( w - r ) * ( w + r ) );

		kbVec3 refractDir = r * incomingRay + ( w - k ) * normal;

		refractDir.Normalize();
		ray_t refractionRay;
		kbVec4 dummyMat;
		if ( CastRay( rayInfo.position + refractDir, refractDir, refractionRay.position, refractionRay.color, refractionRay.normal, dummyMat ) >= 0 ) {
			refractionRay.reflection = dummyMat.x;
			refractionRay.refraction = dummyMat.y;
			refractionRay.viewerPos = rayInfo.position;
			TraceRay( refractionRay, maxNumBounces - 1 );
			refractionColor = refractionRay.color;
		} /*else {
			char msg[256];
			sprintf( msg, "pos = %f %f %f, dir = %f %f %f\n", rayInfo.position.x, rayInfo.position.y, rayInfo.position.z, refractDir.x, refractDir.y, refractDir.z );
			WriteToFile( msg );
			//refractionColor.Set( 1.0f, 0.0f, 1.0f, 0.0f );
		}*/
		//rayInfo.color = refractionColor;
		//return;
	}

	kbVec4 incomingLight( 0.0f, 0.0f, 0.0f, 0.0f );

	// direct lighting
	for ( unsigned int iLight = 0; iLight < m_Lights.size(); iLight++ ) {
		float shadowCoverage = PointIsInShadow( rayInfo.position, rayInfo.normal, iLight );

		if ( shadowCoverage < 1.0f ) {
			const kbVec4 lightColor = ( 1.0f - shadowCoverage ) * m_Lights[iLight]->LightPoint( rayInfo.position, rayInfo.normal );
			incomingLight += lightColor;
		}
	}

	// final gather
	if ( NUM_FINAL_GATHER_RAYS > 0 && rayInfo.reflection < 1.0f && maxNumBounces == MAX_NUM_RAY_BOUNCES ) {
		const float sqrtNSamples = sqrt( ( float ) NUM_FINAL_GATHER_RAYS );
		const float oneOverSqrtN = 1.0f / sqrtNSamples;

		kbMatrix transform;
		transform.MakeMatrixFromZAxis( rayInfo.normal );
		kbVec4 irradiance( 0.0f, 0.0f, 0.0f, 0.0f );
	
		for ( int a = 0; a < sqrtNSamples; a++ ) {
			for ( int b = 0; b < sqrtNSamples; b++ ) {
				const float frand1 = rand() / ( float ) RAND_MAX;
				const float frand2 = rand() / ( float ) RAND_MAX;

				const float x = ( a + frand1 ) * oneOverSqrtN;
				const float y = ( b + frand2 ) * oneOverSqrtN;
		
				const float theta = 2.0f * acos( sqrt( 1.0f - x ) );
				const float phi = 2.0f * kbPI * y;

				kbVec3 endTrace( sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) );
				endTrace = endTrace * transform;
				endTrace.Normalize();
				
				kbVec3 photonGatherPos, dummyNormal;
				kbVec4 dummyColor, dummyMat;
				if ( CastRay( rayInfo.position + ( endTrace * SHADOW_TEST_PULL ), endTrace, photonGatherPos, dummyColor, dummyNormal, dummyMat ) >= 0 ) {
					/*********************************************************************************************************
					if ( dummyNormal.Dot( rayInfo.normal ) > 0.999f ) {
						static int breakHere = 0;
						breakHere++;
						continue;
					}
					/**********************************************************************************************************/
					irradiance += GatherPhotons( photonGatherPos );
				}
			}
		}

		irradiance = irradiance / NUM_FINAL_GATHER_RAYS;
		incomingLight += irradiance;

	}

	rayInfo.color.MultiplyComponents( incomingLight );

	rayInfo.color = ( rayInfo.color * ( 1.0f - rayInfo.reflection ) ) + ( reflectionRayColor * rayInfo.reflection );
	rayInfo.color = ( rayInfo.color * ( 1.0f - rayInfo.refraction ) ) + ( refractionColor * rayInfo.refraction );
}
