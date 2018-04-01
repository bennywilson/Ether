//===================================================================================================
// kbTerrainComponent.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"		// <--- TODO: Temp, game entity should not be accessed from renderer
#include "kbTerrainComponent.h"

KB_DEFINE_COMPONENT(kbTerrainComponent)

struct debugNormal
{
	kbVec3 normal;
	kbVec3 position;
};

std::vector<debugNormal> terrainNormals;

/**
 *	kbTerrainComponent
 */
void kbTerrainComponent::Constructor() {
	m_pHeightMap = nullptr;
	m_HeightScale = 0.3f;
	m_TerrainWidth = 256.0f;
	m_TerrainLength = 256.0;
	m_bRegenerateTerrain = false;
}

/**
 *	~kbTerrainComponent
 */
kbTerrainComponent::~kbTerrainComponent() {
   if ( m_pHeightMap ) {
      m_pHeightMap->Release();
      m_pHeightMap = nullptr;
   }

   m_TerrainModel.Release();
}

/**
 *	kbTerrainComponent::PostLoad
 */
void kbTerrainComponent::PostLoad() {
	Super::PostLoad();

	if ( m_pHeightMap != nullptr ) {
		m_bRegenerateTerrain = true;
	}
}

/**
 *	kbTerrainComponent::EditorChange
 */
void kbTerrainComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "HeightMap" ) {
		m_bRegenerateTerrain = true;
	}
}

/**
 *	kbTerrainComponent::GenerateTerrain
 */
void kbTerrainComponent::GenerateTerrain() {
	if ( m_pHeightMap == nullptr ) {
		kbError( "No height map file found for terrain component on entity %s", m_pParent->GetName().c_str() );
		return;
	}

	struct pixelData {
		byte r;
		byte g;
		byte b;
		byte a;
	};

	unsigned int width, length;

	const pixelData *const pTextureBuffer = ( pixelData * )m_pHeightMap->GetCPUTexture( width, length );

	// Build terrain here
	const int numVerts = width * length;
	const unsigned int numIndices = ( width - 1 ) * ( length - 1 ) * 6;
	const float HalfTerrainWidth = m_TerrainWidth * 0.5f;
	const float HalfTerrainLength = m_TerrainLength * 0.5f;
	const float stepSize = m_TerrainWidth / (float) width;

	m_TerrainModel.CreateDynamicModel( numVerts, numIndices );

	vertexLayout * pVerts = ( vertexLayout * ) m_TerrainModel.MapVertexBuffer();

	int currentVert = 0;
	for ( unsigned int startY = 0; startY < length; startY++ ) {
		for ( unsigned int startX = 0; startX < width; startX++ ) {
			int textureIndex = ( startY * width ) + startX;

			float divisor = 1.0f;
			float height = 0.0f;//( float ) pTextureBuffer[textureIndex].r;

			for ( int tempY = 0; tempY < 8; tempY++ ) {
				for ( int tempX = 0; tempX < 8; tempX++ ) {
					int texIdx = ( ( startY + tempY ) * width ) + ( tempX + startX );

					if ( texIdx >= 0 && texIdx < numVerts ) {
						divisor += 1.0f;
						height += ( float ) pTextureBuffer[texIdx].r;
					}
				}
			}

			height *= ( m_HeightScale / divisor );
			pVerts[currentVert].Clear();
			pVerts[currentVert].position.Set( -HalfTerrainWidth +  ( startX * stepSize ), height, -HalfTerrainLength + ( startY * stepSize ) );
			pVerts[currentVert].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
			pVerts[currentVert].SetNormal( kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ) );
			currentVert++;
		}
	}
	m_TerrainModel.UnmapVertexBuffer();

	unsigned long * pIndices = ( unsigned long* ) m_TerrainModel.MapIndexBuffer();
	int currentIndexToWrite = 0;

	for ( unsigned int startY = 0; startY < length; startY++ ) {
		for ( unsigned int startX = 0; startX < width; startX++ ) {
			int currentIndex = ( startY * width ) + startX;

			kbVec3 xVec, zVec;

			if ( startX < width - 1 ) {
				xVec = pVerts[currentIndex + 1].position - pVerts[currentIndex].position;
			} else {
				xVec = pVerts[currentIndex].position - pVerts[currentIndex - 1].position;
			}

			if ( startY < length - 1 ) {
				zVec = pVerts[ currentIndex ].position - pVerts[ currentIndex + width ].position;
			} else {
				zVec = pVerts[ currentIndex - width ].position - pVerts[ currentIndex ].position;
			}

			xVec.Normalize();
			zVec.Normalize();
			kbVec3 finalVec = xVec.Cross( zVec ).Normalized();
			pVerts[currentIndex].SetNormal( finalVec );

			debugNormal newNormal;
			newNormal.normal = finalVec;
			newNormal.position = pVerts[ currentIndex ].position;
			terrainNormals.push_back( newNormal );
		}
	}

	for ( unsigned int y = 0; y < length - 1; y++ ) {
		for ( unsigned int x = 0; x < width - 1; x++ ) {
			
			unsigned int currentIndex = ( y * width ) + x;
			pIndices[ currentIndexToWrite + 2 ] = currentIndex;
			pIndices[ currentIndexToWrite + 1 ] = currentIndex + 1;
			pIndices[ currentIndexToWrite + 0 ] = currentIndex + width;

			pIndices[ currentIndexToWrite + 5 ] = currentIndex + 1;
			pIndices[ currentIndexToWrite + 4 ] = currentIndex + 1 + width;
			pIndices[ currentIndexToWrite + 3 ] = currentIndex + width;

			currentIndexToWrite += 6;
		}
	}

	m_TerrainModel.UnmapIndexBuffer();

	g_pRenderer->AddRenderObject( this, &m_TerrainModel, kbVec3( 0.0f, 0.0f, 0.0f ), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3( 1.0f, 1.0f, 1.0f ) );
}

/**
 *	kbTerrainComponent::Update_Internal
 */
void kbTerrainComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_TerrainModel.GetMeshes().size() > 0 && m_pParent->IsDirty() ) {
		g_pRenderer->UpdateRenderObject( this, &m_TerrainModel, m_pParent->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), m_pParent->GetScale() );
	}
}

/**
 *	kbTerrainComponent::RenderSync
 */
void kbTerrainComponent::RenderSync() {
	Super::RenderSync();

	if ( m_bRegenerateTerrain ) {
		GenerateTerrain();
		m_bRegenerateTerrain = false;
	}
}