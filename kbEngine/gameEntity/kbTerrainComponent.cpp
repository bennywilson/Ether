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
 *  kbGrass::Constructor
 */
void kbGrass::Constructor() {
    m_Dummy = 0;
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
	m_bRegenerateTerrain = false;
	m_pTerrainShader = nullptr;
	m_pSplatMap = nullptr;
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

    SetMaterialParams();
	g_pRenderer->UpdateRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), GetOwner()->GetScale(), RP_Lighting, &m_ShaderOverrideList , &m_ShaderParamOverride );
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
		g_pRenderer->RemoveRenderObject( this );
	}

	m_TerrainModel.CreateDynamicModel( numVerts, numIndices );

	vertexLayout *const pVerts = (vertexLayout *) m_TerrainModel.MapVertexBuffer();

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

    SetMaterialParams();
    g_pRenderer->AddRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one, RP_Lighting, &m_ShaderOverrideList, &m_ShaderParamOverride );

    if ( m_Grass.size() > 0 ) {
        m_GrassModel.CreatePointCloud( 1, "./assets/Shaders/grass.kbShader" );
        vertexLayout *const pVerts = (vertexLayout *) m_GrassModel.MapVertexBuffer();
        pVerts[0].position.Set( 0.0f, 0.0f, 0.0f );
        pVerts[0].SetColor( kbVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        m_GrassModel.UnmapVertexBuffer();
        g_pRenderer->AddRenderObject( &m_Grass[0], &m_GrassModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one );
    }
}

/**
 *	kbTerrainComponent::SetEnable_Internal
 */	
void kbTerrainComponent::SetEnable_Internal( const bool isEnabled ) {

	if ( m_TerrainModel.NumVertices() == 0 ) {
		return;
	}

	if ( isEnabled ) {
		SetMaterialParams();
		g_pRenderer->AddRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one, RP_Lighting, &m_ShaderOverrideList, &m_ShaderParamOverride );

        if ( m_GrassModel.NumVertices() > 0 ) {
            g_pRenderer->AddRenderObject( &m_Grass[0], &m_GrassModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3::one );
        }
	} else {
		g_pRenderer->RemoveRenderObject( this );

        if ( m_GrassModel.NumVertices() > 0 ) {
            g_pRenderer->RemoveRenderObject( &m_Grass[0] );
        }
	}
}

/**
 *	kbTerrainComponent::Update_Internal
 */
void kbTerrainComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_TerrainModel.GetMeshes().size() > 0 && GetOwner()->IsDirty() ) {

		SetMaterialParams();
		g_pRenderer->UpdateRenderObject( this, &m_TerrainModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), GetOwner()->GetScale(), RP_Lighting, &m_ShaderOverrideList , &m_ShaderParamOverride );

        if ( m_GrassModel.NumVertices() > 0 ) {
            g_pRenderer->UpdateRenderObject( &m_Grass[0], &m_GrassModel, GetOwner()->GetPosition(), kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), GetOwner()->GetScale() );     
        }
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
}

/**
 *	kbTerrainComponent::SetMaterialParams
 */
void kbTerrainComponent::SetMaterialParams() {

	m_ShaderOverrideList.clear();
	if ( m_pTerrainShader != nullptr ) {
		m_ShaderOverrideList.push_back( m_pTerrainShader );
	}

	m_ShaderParamOverride.m_ParamOverrides.clear();
    m_ShaderParamOverride.SetTexture( "splatMap", m_pSplatMap );

	kbVec4 specFactors( 1.0f, 1.0f, 1.0f, 1.0f );
	kbVec4 specPowMult( 1.0f, 1.0f, 1.0f, 1.0f );
	float uvScale[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	for ( int i = 0; i < m_TerrainMaterials.size() && i < 4; i++ ) {
		const kbTerrainMatComponent & curMaterial = m_TerrainMaterials[i];
		m_ShaderParamOverride.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Diffuse" ).c_str(), curMaterial.GetDiffuseMap() );
		m_ShaderParamOverride.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Normal" ).c_str(), curMaterial.GetNormalMap() );
		m_ShaderParamOverride.SetTexture( std::string( "Mat" + std::to_string(i + 1) + "Specular" ).c_str(), curMaterial.GetSpecMap() );

		specFactors[i] = curMaterial.GetSpecFactor();
		specPowMult[i] = curMaterial.GetSpecPowerMultiplier();
		uvScale[(i*2) + 0] = curMaterial.GetUVScale().x;
		uvScale[(i*2) + 1] = curMaterial.GetUVScale().y;
	}

	m_ShaderParamOverride.SetVec4( "mat1And2UVScale", kbVec4( uvScale[0], uvScale[1], uvScale[2], uvScale[3] ) );
	m_ShaderParamOverride.SetVec4( "mat3And4UVScale", kbVec4( uvScale[4], uvScale[5], uvScale[6], uvScale[7] ) );

	m_ShaderParamOverride.SetVec4( "specFactors", specFactors );
	m_ShaderParamOverride.SetVec4( "specPowerMultipliers", specPowMult );
/*
	float4 mat1And2UVScale;
	float4 mat3And4UVScale;
	float4 specFactors;
	float4 specPowerMultipliers;
*/
/*
	if ( m_TerrainMaterials.size() > 0 ) {
		const kbTerrainMatComponent & curMaterial = m_TerrainMaterials[0];

		m_ShaderParamOverride.SetTexture( "Mat1Diffuse", curMaterial.GetDiffuseMap() );
		m_ShaderParamOverride.SetTexture( "Mat1Normal", curMaterial.GetNormalMap() );
		m_ShaderParamOverride.SetTexture( "Mat1Specular", curMaterial.GetSpecMap() );

		specFactors.x = curMaterial.GetSpecFactor();
		specPowerMultiplier.x = curMaterial.GetSpecPowerMultiplier();

		MatUVScale.x = curMaterial.GetUVScale().x;
		MatUVScale.y = curMaterial.GetUVScale().y;
	}

	if ( m_TerrainMaterials.size() > 1 ) {
		const kbTerrainMatComponent & curMaterial = m_TerrainMaterials[1];

		m_ShaderParamOverride.SetTexture( "Mat2Diffuse", curMaterial.GetDiffuseMap() );
		m_ShaderParamOverride.SetTexture( "Mat2Normal", curMaterial.GetNormalMap() );
		m_ShaderParamOverride.SetTexture( "Mat2Specular", curMaterial.GetSpecMap() );

		specFactors.y = curMaterial.GetSpecFactor();
		specPowerMultiplier.y = curMaterial.GetSpecPowerMultiplier();

		MatUVScale.z = curMaterial.GetUVScale().x;
		MatUVScale.w = curMaterial.GetUVScale().y;
	}
	m_ShaderParamOverride.SetVec4( "mat1And2UVScale", MatUVScale );

	MatUVScale.Set( 0.0f, 0.0f, 0.0f, 0.0f );

	if ( m_TerrainMaterials.size() > 2 ) {
		const kbTerrainMatComponent & curMaterial = m_TerrainMaterials[21];

		m_ShaderParamOverride.SetTexture( "Mat3Diffuse", curMaterial.GetDiffuseMap() );
		m_ShaderParamOverride.SetTexture( "Mat3Normal", curMaterial.GetNormalMap() );
		m_ShaderParamOverride.SetTexture( "Mat3Specular", curMaterial.GetSpecMap() );

		specFactors.z = m_TerrainMaterials[2].GetSpecFactor();

		MatUVScale.x = m_TerrainMaterials[2].GetUVScale().x;
		MatUVScale.y = m_TerrainMaterials[2].GetUVScale().y;
	}

	if ( m_TerrainMaterials.size() > 3 ) {
		m_ShaderParamOverride.SetTexture( "Mat4Diffuse", m_TerrainMaterials[3].GetDiffuseMap() );
		m_ShaderParamOverride.SetTexture( "Mat4Normal", m_TerrainMaterials[3].GetNormalMap() );
		m_ShaderParamOverride.SetTexture( "Mat4Specular", m_TerrainMaterials[3].GetSpecMap() );

		specFactors.w = m_TerrainMaterials[3].GetSpecFactor();

		MatUVScale.z = m_TerrainMaterials[3].GetUVScale().x;
		MatUVScale.y = m_TerrainMaterials[3].GetUVScale().y;
	}
*/
}