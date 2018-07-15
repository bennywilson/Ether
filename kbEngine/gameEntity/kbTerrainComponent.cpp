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

struct patchVertLayout {
	kbVec3 position;
	kbVec2 uv;
	byte patchIndices[4];
};

struct debugNormal
{
	kbVec3 normal;
	kbVec3 position;
};
std::vector<debugNormal> terrainNormals;


/**
 *	grassRenderObject_t::Initialize
 */
void kbGrass::grassRenderObject_t::Initialize( const kbVec3 & ownerPosition ) {
	kbErrorCheck( m_pModel == nullptr && m_pComponent == nullptr, "grassRenderObject_t::Initialize() - m_pModel or m_pComponent is not NULL" );

	m_pModel = new kbModel();
	m_pComponent = new kbComponent();

	m_RenderObject.m_pComponent = m_pComponent;
	m_RenderObject.m_pModel = m_pModel;
	m_RenderObject.m_RenderPass = ERenderPass::RP_Lighting;
	m_RenderObject.m_Position = ownerPosition;
	m_RenderObject.m_Orientation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	m_RenderObject.m_Scale.Set( 1.0f, 1.0f, 1.0f );
//	m_RenderObject.m_EntityId
//	m_RenderObject.m_MatrixList
	m_RenderObject.m_bCastsShadow = false;
}

/**
 *	grassRenderObject_t::Shutdown
 */
void kbGrass::grassRenderObject_t::Shutdown() {
	kbErrorCheck( m_pModel != nullptr && m_pComponent != nullptr, "grassRenderObject_t::Initialize() - m_pModel or m_pComponent is not NULL" );

	delete m_pComponent;
	m_pComponent = nullptr;

	delete m_pModel;
	m_pModel = nullptr;
}

/**
 *  kbGrass::Constructor
 */
void kbGrass::Constructor() {

	m_GrassCellsPerTerrainSide = 1;
	m_GrassCellLength = 0;

	m_PatchStartCullDistance = 200.0f;
	m_PatchEndCullDistance = 300.0f;

	m_PatchesPerCellSide = 3;

	m_BladeMinWidth = 1.0f;
	m_BladeMaxWidth = 2.0f;

	m_BladeMinHeight = 5.0f;
	m_BladeMaxHeight = 10.0f;

	m_pDiffuseMap = nullptr;

	m_pOwningTerrainComponent = nullptr;

	m_bNeedsMaterialUpdate = false;
}

/**
 *  kbGrass::~kbGrass
 */
kbGrass::~kbGrass() {
	for ( int i = 0; i < m_GrassRenderObjects.size(); i++ ) {
		m_GrassRenderObjects[i].Shutdown();
	}
}

/**
 *  kbGrass::EditorChange
 */
void kbGrass::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( m_GrassCellsPerTerrainSide < 0 ) {
		kbWarning( "kbGrass::EditorChange() - Grass Cells Per Terrain Side must be greater than 0" );
		m_GrassCellsPerTerrainSide = 1;
	}

	m_bNeedsMaterialUpdate = true;
}

/**
 *  kbGrass::RenderSync
 */
void kbGrass::RenderSync() {
	Super::RenderSync();

	if ( m_bNeedsMaterialUpdate ) {
		UpdateMaterial();
	}
}

/**
 *  kbGrass::SetEnable_Internal
 */
void kbGrass::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );

	if ( isEnabled ) {

		for ( int i = 0; i < m_GrassRenderObjects.size(); i++ ) {
			g_pRenderer->AddRenderObject( m_GrassRenderObjects[i].m_RenderObject );
		}

	} else {

		for ( int i = 0; i < m_GrassRenderObjects.size(); i++ ) {
			g_pRenderer->RemoveRenderObject( m_GrassRenderObjects[i].m_RenderObject );
		}

	}
}

/**
 *  kbGrass::UpdateMaterial
 */
void kbGrass::UpdateMaterial() {

	std::vector<kbVec4> bladeOffsets;

	m_GrassCellLength = m_pOwningTerrainComponent->GetTerrainWidth() / (float)m_GrassCellsPerTerrainSide;
	const float patchLen = m_GrassCellLength / (float)m_PatchesPerCellSide;
	const float halfPatchLen = patchLen * 0.5f;

	//float grassCellHalfSize = ( m_DistanceBetweenPatches / 2.0f ) * 0.95f;
	for ( int i = 0; i < 64; i++ ) {

		kbMat4 matrix = kbMat4::identity;
		const float angle = kbfrand() * kbPI;
		float cosPIOver2 = cos( angle );
		float sinPIOver2 = sin( angle );
		matrix[0][0] = cosPIOver2;
		matrix[2][0] = -sinPIOver2;
		matrix[0][2] = sinPIOver2;
		matrix[2][2] = cosPIOver2;

		kbVec4 startVec( 0.0f, 0.0f, 1.0f, 0.0f );
		startVec = startVec.TransformPoint( matrix );

		kbVec4 offset;
		offset.x = startVec.x;
		offset.y = startVec.z;
		offset.z = halfPatchLen * kbfrand();
		offset.w = halfPatchLen * kbfrand();
		bladeOffsets.push_back( offset );
	}

	m_GrassShaderOverrides.m_ParamOverrides.clear();
	m_GrassShaderOverrides.SetTexture( "grassMap", m_pGrassMap );
	m_GrassShaderOverrides.SetTexture( "heightMap", m_pOwningTerrainComponent->GetHeightMap() );

	m_GrassShaderOverrides.SetVec4List( "bladeOffsets", bladeOffsets );
	m_GrassShaderOverrides.SetVec4( "bladeParameters", kbVec4( m_BladeMinWidth, m_BladeMaxWidth, m_BladeMinHeight, m_BladeMaxHeight ) );
	m_GrassShaderOverrides.SetVec4( "GrassData1", kbVec4( m_pOwningTerrainComponent->GetHeightScale(), m_pOwningTerrainComponent->GetOwner()->GetPosition().y, patchLen, 0.0f ) );
	m_GrassShaderOverrides.SetVec4( "GrassData2", kbVec4( m_PatchStartCullDistance, 1.0f / ( m_PatchEndCullDistance - m_PatchStartCullDistance ), 0.0f, 0.0f ) );

	if ( m_pDiffuseMap != nullptr ) {
		m_GrassShaderOverrides.SetTexture( "grassDiffuseMap", m_pDiffuseMap );
	}


	for ( int i = 0; i < m_GrassRenderObjects.size(); i++ ) {
		g_pRenderer->RemoveRenderObject( m_GrassRenderObjects[i].m_RenderObject );
		m_GrassRenderObjects[i].Shutdown();
	}
	m_GrassRenderObjects.clear();

	m_GrassRenderObjects.insert( m_GrassRenderObjects.begin(), m_GrassCellsPerTerrainSide * m_GrassCellsPerTerrainSide, grassRenderObject_t() );
	const float halfCellLen = m_GrassCellLength * 0.5f;
	const float halfCellLenSqr = sqrt( halfCellLen * halfCellLen );

	const float halfTerrainWidth = m_pOwningTerrainComponent->GetTerrainWidth() * 0.5f;
	const kbVec3 terrainMin = m_pOwningTerrainComponent->GetOwner()->GetPosition() - kbVec3( halfTerrainWidth, 0.0f, halfTerrainWidth );

	int cellIdx = 0;
	for ( int yCell = 0; yCell < m_GrassCellsPerTerrainSide; yCell++ ) {
		for ( int xCell = 0; xCell < m_GrassCellsPerTerrainSide; xCell++, cellIdx++ ) {

			grassRenderObject_t & renderObj = m_GrassRenderObjects[cellIdx];
			renderObj.Initialize( m_pOwningTerrainComponent->GetOwner()->GetPosition() );
			renderObj.m_RenderObject.m_CullDistance = m_PatchEndCullDistance + halfCellLenSqr;

			const kbVec3 cellStart = terrainMin + kbVec3( m_GrassCellLength * xCell, 0.0f, m_GrassCellLength * yCell );
			const kbVec3 cellCenter = cellStart + kbVec3( m_GrassCellLength * 0.5f, 0.0f, m_GrassCellLength * 0.5f );
			const kbVec3 halfCell = kbVec3( m_GrassCellLength * 0.5f, 0.0f, m_GrassCellLength * 0.5f );

			renderObj.m_pModel->CreatePointCloud( m_PatchesPerCellSide * m_PatchesPerCellSide, "./assets/Shaders/grass.kbShader", kbMaterial::CM_None, sizeof(patchVertLayout) );

			patchVertLayout *const pVerts = (patchVertLayout *) renderObj.m_pModel->MapVertexBuffer();

			int iVert = 0;
			for ( int startY = 0; startY < m_PatchesPerCellSide; startY ++ ) {
				for ( int startX = 0; startX < m_PatchesPerCellSide; startX ++) {
					kbVec3 localPointPos = kbVec3( patchLen * startX, 0.0f, patchLen * startY ) - halfCell;
					pVerts[iVert].position = localPointPos;

					kbVec3 globalPointPos = cellStart + kbVec3( patchLen * startX, 0.0f, patchLen * startY );
					pVerts[iVert].uv.Set ( ( globalPointPos.x - terrainMin.x ) / m_pOwningTerrainComponent->GetTerrainWidth(), ( globalPointPos.z - terrainMin.z ) / m_pOwningTerrainComponent->GetTerrainWidth() );
					pVerts[iVert].patchIndices[0] = rand() % 60;
					pVerts[iVert].patchIndices[1] = pVerts[iVert].patchIndices[2] = pVerts[iVert].patchIndices[3] = pVerts[iVert].patchIndices[0];
					iVert++;
				}
			}
		    renderObj.m_pModel->UnmapVertexBuffer();

			m_GrassRenderObjects[cellIdx].m_RenderObject.m_Position = cellCenter;
			m_GrassRenderObjects[cellIdx].m_RenderObject.m_ShaderParamOverrides = m_GrassShaderOverrides;
			g_pRenderer->AddRenderObject( m_GrassRenderObjects[cellIdx].m_RenderObject );
		}
	}

	m_bNeedsMaterialUpdate = false;
}

/**
 *  kbTerrainMatComponent::Constructor
 */
void kbTerrainMatComponent::Constructor() {
    m_pDiffuseMap = nullptr;
	m_pNormalMap = nullptr;
	m_pSpecMap = nullptr;

	m_SpecFactor = 1.0f;
	m_SpecPowerMultiplier = 1.0f;
	m_UVScale.Set( 1.0f, 1.0f, 1.0f );
} 

/**
 *	kbTerrainComponent::Constructor
 */
void kbTerrainComponent::Constructor() {

	m_pHeightMap = nullptr;
	m_HeightScale = 0.3f;
	m_TerrainWidth = 256.0f;
	m_TerrainDimensions = 16;
	
	m_pTerrainShader = nullptr;
	m_pSplatMap = nullptr;

	m_bRegenerateTerrain = false;
}

/**
 *	kbTerrainComponent::kbTerrainComponent
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

	for ( int i = 0; i < m_Grass.size(); i++ ) {
		m_Grass[i].SetOwningTerrainComponent( this );
	}
}

/**
 *	kbTerrainComponent::EditorChange
 */
void kbTerrainComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

    std::string propertiesThatRegenTerrain[4] = { "HeightMap", "HeightScale", "Width", "Dimensions" };

    for ( int i = 0; i < 4; i++ ) {
        if ( propertyName == propertiesThatRegenTerrain[i] ) { 
        	m_bRegenerateTerrain = true;
        }
    }

	if ( m_bRegenerateTerrain ) {
		return;
	}

    UpdateTerrainMaterial();
	g_pRenderer->UpdateRenderObject( m_TerrainRenderObject );
}

/**
 *	kbTerrainComponent::GenerateTerrain
 */
void kbTerrainComponent::GenerateTerrain() {
    kbErrorCheck( m_pHeightMap != nullptr, "kbTerrainComponent::GenerateTerrain() - No height map file found for terrain component on entity %s", GetOwner()->GetName().c_str() );

	struct pixelData {
		byte r;
		byte g;
		byte b;
		byte a;
	};

	terrainNormals.clear();

	unsigned int texWidth, texHeight;

	const pixelData *const pTextureBuffer = ( pixelData * )m_pHeightMap->GetCPUTexture( texWidth, texHeight );

	// Build terrain here
	const int numVerts = m_TerrainDimensions * m_TerrainDimensions;
	const unsigned int numIndices = ( m_TerrainDimensions - 1 ) * ( m_TerrainDimensions - 1 ) * 6;
	const float HalfTerrainWidth = m_TerrainWidth * 0.5f;
	const float stepSize = m_TerrainWidth / (float) texWidth;
	const float cellWidth = m_TerrainWidth / (float)m_TerrainDimensions;

	if ( m_TerrainModel.NumVertices() > 0 ) {
		g_pRenderer->RemoveRenderObject( m_TerrainRenderObject );
	}

	m_TerrainModel.CreateDynamicModel( numVerts, numIndices );

	vertexLayout *const pVerts = (vertexLayout *) m_TerrainModel.MapVertexBuffer();

	int blurSampleSize = 1;
	int currentVert = 0;
	for ( int startY = 0; startY < m_TerrainDimensions; startY++ ) {
		for ( int startX = 0; startX < m_TerrainDimensions; startX++ ) {


		//	int textureIndex = ( v * texWidth ) + u;

			float divisor = 0.0f;
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
			height /= 255.0f;
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

			xVec = finalVec.Cross( zVec ).Normalized();
			zVec = xVec.Cross( finalVec ).Normalized();

			pVerts[currentIndex].SetTangent( -zVec );
			pVerts[currentIndex].SetBinormal( xVec );

			debugNormal newNormal;
			newNormal.normal = xVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
			terrainNormals.push_back( newNormal );

			newNormal.normal = zVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
			terrainNormals.push_back( newNormal );

			newNormal.normal = finalVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
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

    UpdateTerrainMaterial();

    g_pRenderer->AddRenderObject( m_TerrainRenderObject );
}

/**
 *	kbTerrainComponent::SetEnable_Internal
 */	
void kbTerrainComponent::SetEnable_Internal( const bool isEnabled ) {

	if ( m_TerrainModel.NumVertices() == 0 ) {
		return;
	}

	if ( isEnabled ) {
		UpdateTerrainMaterial();
		g_pRenderer->AddRenderObject( m_TerrainRenderObject );

		for ( int i = 0; i < m_Grass.size(); i++ ) {
			m_Grass[i].Enable( true );
		}

	} else {
		g_pRenderer->RemoveRenderObject( this );

		for ( int i = 0; i < m_Grass.size(); i++ ) {
			m_Grass[i].Enable( false );
		}
	}
}

/**
 *	kbTerrainComponent::Update_Internal
 */
void kbTerrainComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_TerrainModel.GetMeshes().size() > 0 && ( GetOwner()->IsDirty() ) ) {

		UpdateTerrainMaterial();
		g_pRenderer->UpdateRenderObject( m_TerrainRenderObject );
	}

	kbVec3 currentCameraPosition;
	kbQuat currentCameraRotation;
	g_pRenderer->GetRenderViewTransform( nullptr, currentCameraPosition, currentCameraRotation );
	/*for ( int i = 0; i < terrainNormals.size(); i += 3 ) {
		static float checkDist = 50000;
		if ( ( currentCameraPosition - terrainNormals[i].position ).LengthSqr() > checkDist ) {
			continue;
		}

		g_pRenderer->DrawLine( terrainNormals[i].position, terrainNormals[i].position + terrainNormals[i + 0].normal * 5.0f, kbColor::red );
		g_pRenderer->DrawLine( terrainNormals[i].position, terrainNormals[i].position + terrainNormals[i + 1].normal * 5.0f, kbColor::green );
		g_pRenderer->DrawLine( terrainNormals[i].position, terrainNormals[i].position + terrainNormals[i + 2].normal * 5.0f, kbColor::blue );
	}*/
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

	for ( int i = 0; i < m_Grass.size(); i++ )  {
		m_Grass[i].RenderSync();
	}
}

/**
 *	kbTerrainComponent::UpdateTerrainMaterial
 */
void kbTerrainComponent::UpdateTerrainMaterial() {

	m_ShaderOverrideList.clear();
	if ( m_pTerrainShader != nullptr ) {
		m_ShaderOverrideList.push_back( m_pTerrainShader );
	}

	m_TerrainShaderOverrides.m_ParamOverrides.clear();
    m_TerrainShaderOverrides.SetTexture( "splatMap", m_pSplatMap );

	kbVec4 specFactors( 1.0f, 1.0f, 1.0f, 1.0f );
	kbVec4 specPowMult( 1.0f, 1.0f, 1.0f, 1.0f );
	float uvScale[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	for ( int i = 0; i < m_TerrainMaterials.size() && i < 4; i++ ) {
		const kbTerrainMatComponent & curMaterial = m_TerrainMaterials[i];
		m_TerrainShaderOverrides.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Diffuse" ).c_str(), curMaterial.GetDiffuseMap() );
		m_TerrainShaderOverrides.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Normal" ).c_str(), curMaterial.GetNormalMap() );
		m_TerrainShaderOverrides.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Specular" ).c_str(), curMaterial.GetSpecMap() );

		specFactors[i] = curMaterial.GetSpecFactor();
		specPowMult[i] = curMaterial.GetSpecPowerMultiplier();
		uvScale[(i*2) + 0] = curMaterial.GetUVScale().x;
		uvScale[(i*2) + 1] = curMaterial.GetUVScale().y;
	}

	m_TerrainShaderOverrides.SetVec4( "mat1And2UVScale", kbVec4( uvScale[0], uvScale[1], uvScale[2], uvScale[3] ) );
	m_TerrainShaderOverrides.SetVec4( "mat3And4UVScale", kbVec4( uvScale[4], uvScale[5], uvScale[6], uvScale[7] ) );

	m_TerrainShaderOverrides.SetVec4( "specFactors", specFactors );
	m_TerrainShaderOverrides.SetVec4( "specPowerMultipliers", specPowMult );

	m_TerrainRenderObject.m_bCastsShadow = false;
	m_TerrainRenderObject.m_bIsSkinnedModel = false;
	m_TerrainRenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_TerrainRenderObject.m_Position = GetOwner()->GetPosition();
	m_TerrainRenderObject.m_EntityId = GetOwner()->GetEntityId();
	m_TerrainRenderObject.m_Scale.Set( 1.0f, 1.0f, 1.0f );
	m_TerrainRenderObject.m_pModel = &m_TerrainModel;
	m_TerrainRenderObject.m_RenderPass = RP_Lighting;
	m_TerrainRenderObject.m_OverrideShaderList = m_ShaderOverrideList;
	m_TerrainRenderObject.m_ShaderParamOverrides = m_TerrainShaderOverrides;
	m_TerrainRenderObject.m_pComponent = this;
}