//===================================================================================================
// kbTerrainComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbTerrainComponent.h"

KB_DEFINE_COMPONENT(kbTerrainComponent)

struct debugNormal
{
	kbVec3 normal;
	kbVec3 position;
};
std::vector<debugNormal> terrainNormals;

/**
 *  kbTerrainMatComponent::Constructor
 */
void kbTerrainMatComponent::Constructor() {
    m_DiffuseTexture = nullptr;
}

/**
 *	kbTerrainComponent::Constructor
 */
void kbTerrainComponent::Constructor() {
	m_pHeightMap = nullptr;
	m_HeightScale = 0.3f;
	m_TerrainWidth = 256.0f;
	m_TerrainDimensions = 16;
	m_bRegenerateTerrain = false;
	m_pTerrainShader = nullptr;
	m_pSplatMap = nullptr;
}

/**
 *	~kbTerrainComponent::kbTerrainComponent
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

	m_bRegenerateTerrain = true;
}

/**
 *	kbTerrainComponent::GenerateTerrain
 */
void kbTerrainComponent::GenerateTerrain() {
	if ( m_pHeightMap == nullptr ) {
		kbError( "No height map file found for terrain component on entity %s", GetOwner()->GetName().c_str() );
		return;
	}

	struct pixelData {
		byte r;
		byte g;
		byte b;
		byte a;
	};

	unsigned int texWidth, texHeight;

	const pixelData *const pTextureBuffer = ( pixelData * )m_pHeightMap->GetCPUTexture( texWidth, texHeight );

	// Build terrain here
	const int numVerts = m_TerrainDimensions * m_TerrainDimensions;
	const unsigned int numIndices = ( m_TerrainDimensions - 1 ) * ( m_TerrainDimensions - 1 ) * 6;
	const float HalfTerrainWidth = m_TerrainWidth * 0.5f;
	const float stepSize = m_TerrainWidth / (float) texWidth;
	const float cellWidth = m_TerrainWidth / (float)m_TerrainDimensions;

	if ( m_TerrainModel.NumVertices() > 0 ) {
		g_pRenderer->RemoveRenderObject( this );
	}

	m_TerrainModel.CreateDynamicModel( numVerts, numIndices );

	vertexLayout *const pVerts = ( vertexLayout * ) m_TerrainModel.MapVertexBuffer();

	int blurSampleSize = 8;
	int currentVert = 0;
	for ( int startY = 0; startY < m_TerrainDimensions; startY++ ) {
		for ( int startX = 0; startX < m_TerrainDimensions; startX++ ) {


		//	int textureIndex = ( v * texWidth ) + u;

			float divisor = 1.0f;
			float height = 0.0f;

			for ( int tempY = 0; tempY < blurSampleSize; tempY++ ) {

				if ( tempY + startY >= m_TerrainDimensions ) {
					break;
				}

				for ( int tempX = 0; tempX < blurSampleSize; tempX++ ) {
					if ( tempX + startX >= m_TerrainDimensions ) {
						break;
					}

					const float u = (float)(startX + tempX) / (float)m_TerrainDimensions;
					const float v = (float)(startY + tempY) / (float)m_TerrainDimensions;
					const int textureIndex = static_cast<int>(( v * texWidth * texWidth ) + ( u * texWidth ) );

					divisor += 1.0f;
					height += ( float ) pTextureBuffer[textureIndex].r;
				}
			}

			height *= ( m_HeightScale / divisor );
			pVerts[currentVert].Clear();
			pVerts[currentVert].position.Set( -HalfTerrainWidth + ( startX * cellWidth ), height, -HalfTerrainWidth + ( startY * cellWidth ) );
			pVerts[currentVert].uv.Set( (float)(startX) / (float)m_TerrainDimensions, (float)(startY) / (float)m_TerrainDimensions );
			pVerts[currentVert].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
			pVerts[currentVert].SetNormal( kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ) );
			currentVert++;
		}
	}
	m_TerrainModel.UnmapVertexBuffer();

	unsigned long * pIndices = ( unsigned long* ) m_TerrainModel.MapIndexBuffer();
	int currentIndexToWrite = 0;

	for ( int startY = 0; startY < m_TerrainDimensions; startY++ ) {
		for ( int startX = 0; startX < m_TerrainDimensions; startX++ ) {
			int currentIndex = ( startY * m_TerrainDimensions ) + startX;

			kbVec3 xVec, zVec;

			if ( startX < m_TerrainDimensions - 1 ) {
				xVec = pVerts[currentIndex + 1].position - pVerts[currentIndex].position;
			} else {
				xVec = pVerts[currentIndex].position - pVerts[currentIndex - 1].position;
			}

			if ( startY < m_TerrainDimensions - 1 ) {
				zVec = pVerts[currentIndex ].position - pVerts[ currentIndex + m_TerrainDimensions].position;
			} else {
				zVec = pVerts[currentIndex - m_TerrainDimensions].position - pVerts[currentIndex].position;
			}

			xVec.Normalize();
			zVec.Normalize();
			kbVec3 finalVec = xVec.Cross( zVec ).Normalized();
			pVerts[currentIndex].SetNormal( finalVec );

			debugNormal newNormal;
			newNormal.normal = finalVec;
			newNormal.position = pVerts[currentIndex].position;
			terrainNormals.push_back( newNormal );
		}
	}

	for ( int y = 0; y < m_TerrainDimensions - 1; y++ ) {
		for ( int x = 0; x < m_TerrainDimensions - 1; x++ ) {
			
			const unsigned int currentIndex = ( y * m_TerrainDimensions ) + x;
			pIndices[currentIndexToWrite + 2] = currentIndex;
			pIndices[currentIndexToWrite + 1] = currentIndex + 1;
			pIndices[currentIndexToWrite + 0] = currentIndex + m_TerrainDimensions;

			pIndices[currentIndexToWrite + 5] = currentIndex + 1;
			pIndices[currentIndexToWrite + 4] = currentIndex + 1 + m_TerrainDimensions;
			pIndices[currentIndexToWrite + 3] = currentIndex + m_TerrainDimensions;

			currentIndexToWrite += 6;
		}
	}

	m_TerrainModel.UnmapIndexBuffer();

	kbShader * pShader = (kbShader*)g_ResourceManager.GetResource( "./assets/Shaders/basicTerrain.kbShader", true );
	std::vector<kbShader *> ShaderOverrideList;

	if ( pShader != nullptr ) {
		ShaderOverrideList.push_back( pShader );
	}

    kbShaderParamOverrides_t overrides;
    overrides.SetTexture( "shaderTexture", m_pSplatMap );
	g_pRenderer->AddRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one, RP_Lighting, &ShaderOverrideList, &overrides );
}

/**
 *	kbTerrainComponent::SetEnable_Internal
 */	
void kbTerrainComponent::SetEnable_Internal( const bool isEnabled ) {
	if ( m_TerrainModel.NumVertices() == 0 ) {
		return ;
	}

	kbShader *const pShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	std::vector<kbShader *> ShaderOverrideList;

	if ( pShader != nullptr ) {
		ShaderOverrideList.push_back( pShader );
	}
	if ( isEnabled ) {
        kbShaderParamOverrides_t overrides;
        overrides.SetTexture( "shaderTexture", m_pSplatMap );

		g_pRenderer->AddRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one, RP_Lighting, &ShaderOverrideList, &overrides );	
	} else {
		g_pRenderer->RemoveRenderObject( this );
	}
}

/**
 *	kbTerrainComponent::Update_Internal
 */
void kbTerrainComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_TerrainModel.GetMeshes().size() > 0 && GetOwner()->IsDirty() ) {

	kbShader * pShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	std::vector<kbShader *> ShaderOverrideList;

	if ( pShader != nullptr ) {
		ShaderOverrideList.push_back( pShader );
	}

        kbShaderParamOverrides_t overrides;
        overrides.SetTexture( "shaderTexture", m_pSplatMap );

		g_pRenderer->UpdateRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), GetOwner()->GetScale(), RP_Lighting, &ShaderOverrideList , &overrides );
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