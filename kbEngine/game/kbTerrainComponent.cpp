//===================================================================================================
// kbTerrainComponent.cpp
//
//
// 2016 blk 1.0
//===================================================================================================
#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbGameEntityHeader.h"
#include "kbTerrainComponent.h"
#include "kbRenderer.h"
#include "kbGame.h"

KB_DEFINE_COMPONENT(kbTerrainComponent)

static float g_TerrainLOD = 1.0f;
bool g_bCullGrass = false;

void kbTerrainComponent::SetTerrainLOD(const float lod) {
	g_TerrainLOD = lod;

	if (g_pGame == nullptr) {
		return;
	}

	const std::vector<kbGameEntity*>& gameEnts = g_pGame->GetGameEntities();
	for (int i = 0; i < gameEnts.size(); i++) {

		kbGameEntity* const pEnt = gameEnts[i];
		for (int iComp = 0; iComp < pEnt->NumComponents(); iComp++) {
			kbTerrainComponent* const pTerrain = pEnt->GetComponent(iComp)->GetAs<kbTerrainComponent>();
			if (pTerrain == nullptr) {
				continue;
			}

			pTerrain->RegenerateTerrain();
		}
	}

}

struct patchVertLayout {
	Vec3 position;
	Vec2 uv;
	byte patchIndices[4];
};

struct debugNormal
{
	Vec3 normal;
	Vec3 position;
};
std::vector<debugNormal> terrainNormals;


/**
 *	grassRenderObject_t::Initialize
 */
void kbGrass::grassRenderObject_t::Initialize(const Vec3& ownerPosition) {
	blk::error_check(m_pModel == nullptr && m_pComponent == nullptr, "grassRenderObject_t::Initialize() - m_pModel or m_pComponent is not NULL");

	m_pModel = new kbModel();
	m_pComponent = new kbGameComponent();

	m_RenderObject.m_pComponent = m_pComponent;
	m_RenderObject.m_pModel = m_pModel;
	m_RenderObject.m_RenderPass = ERenderPass::RP_Lighting;
	m_RenderObject.m_Position = ownerPosition;
	m_RenderObject.m_Orientation.set(0.0f, 0.0f, 0.0f, 1.0f);
	m_RenderObject.m_Scale.set(1.0f, 1.0f, 1.0f);
	//	m_RenderObject.m_EntityId
	//	m_RenderObject.m_MatrixList
	m_RenderObject.m_bCastsShadow = false;
}

/**
 *	grassRenderObject_t::Shutdown
 */
void kbGrass::grassRenderObject_t::Shutdown() {
	blk::error_check(m_pModel != nullptr && m_pComponent != nullptr, "grassRenderObject_t::Initialize() - m_pModel or m_pComponent is not NULL");

	delete m_pComponent;
	m_pComponent = nullptr;

	delete m_pModel;
	m_pModel = nullptr;
}

/**
 *  kbGrass::Constructor
 */
void kbGrass::Constructor() {

	m_pGrassShader = nullptr;

	m_GrassCellsPerTerrainSide = 1;
	m_GrassCellLength = 0;

	m_PatchStartCullDistance = 200.0f;
	m_PatchEndCullDistance = 300.0f;

	m_PatchesPerCellSide = 3;

	m_BladeMinWidth = 1.0f;
	m_BladeMaxWidth = 2.0f;

	m_BladeMinHeight = 5.0f;
	m_BladeMaxHeight = 10.0f;

	m_MaxBladeJitterOffset = 0.0f;
	m_MaxPatchJitterOffset = 0.0f;

	m_pOwningTerrainComponent = nullptr;

	m_bUpdateMaterial = false;
	m_bUpdatePointCloud = false;

	m_FakeAODarkness = 0.25f;
	m_FakeAOPower = 2.0f;
	m_FakeAOClipPlaneFadeStartDist = 0.0f;
}

/**
 *  kbGrass::~kbGrass
 */
kbGrass::~kbGrass() {
	for (int i = 0; i < m_GrassRenderObjects.size(); i++) {
		m_GrassRenderObjects[i].Shutdown();
	}
}

/**
 *  kbGrass::EditorChange
 */
void kbGrass::EditorChange(const std::string& propertyName) {
	Super::EditorChange(propertyName);

	if (m_GrassCellsPerTerrainSide < 0) {
		blk::warn("kbGrass::EditorChange() - Grass Cells Per Terrain Side must be greater than 0");
		m_GrassCellsPerTerrainSide = 1;
	}

	const std::string propertiesThatRegenGrass[7] = { "PatchStartCullDistance", "PatchEndCullDistance",
		"PatchesPerCellSide", "MaxPatchJitterOffset", "MaxBladeJitterOffset", "MinPatchJitterOffset", "GrassCellsPerTerrainSide" };

	for (int i = 0; i < 7; i++) {
		if (propertyName == propertiesThatRegenGrass[i]) {
			m_bUpdatePointCloud = true;
		}
	}
	m_bUpdateMaterial = true;
}

/**
 *  kbGrass::RenderSync
 */
void kbGrass::RenderSync() {
	Super::RenderSync();

	if (m_bUpdateMaterial || m_bUpdatePointCloud) {
		RefreshGrass();
	}
}

/**
 *  kbGrass::SetEnable_Internal
 */
void kbGrass::SetEnable_Internal(const bool isEnabled) {
	Super::SetEnable_Internal(isEnabled);

	if (isEnabled) {

		for (int i = 0; i < m_GrassRenderObjects.size(); i++) {
			g_pRenderer->AddRenderObject(m_GrassRenderObjects[i].m_RenderObject);
		}

	} else {

		for (int i = 0; i < m_GrassRenderObjects.size(); i++) {
			g_pRenderer->RemoveRenderObject(m_GrassRenderObjects[i].m_RenderObject);
		}

	}
}

/**
 *  kbGrass::RefreshGrass
 */
void kbGrass::RefreshGrass() {

	const float startRefreshGrassTime = g_GlobalTimer.TimeElapsedSeconds();

	std::vector<Vec4> bladeOffsets;

	const float PatchesPerCellSide = kbClamp((float)m_PatchesPerCellSide, 1.0f, 99999999.0f);

	//float grassCellHalfSize = ( m_DistanceBetweenPatches / 2.0f ) * 0.95f;
	for (int i = 0; i < 64; i++) {

		Mat4 matrix = Mat4::identity;
		const float angle = 2.0f * kbfrand() * kbPI;
		float cosPIOver2 = cos(angle);
		float sinPIOver2 = sin(angle);
		matrix[0][0] = cosPIOver2;
		matrix[2][0] = -sinPIOver2;
		matrix[0][2] = sinPIOver2;
		matrix[2][2] = cosPIOver2;

		Vec4 startVec(0.0f, 0.0f, 1.0f, 0.0f);
		startVec = startVec.transform_point(matrix);

		Vec4 offset;
		offset.x = startVec.x;
		offset.y = startVec.z;
		offset.z = m_MaxBladeJitterOffset * kbfrand();
		offset.w = m_MaxBladeJitterOffset * kbfrand();
		bladeOffsets.push_back(offset);
	}

	m_GrassCellLength = m_pOwningTerrainComponent->GetTerrainWidth() / (float)m_GrassCellsPerTerrainSide;
	const float patchLen = m_GrassCellLength / (float)PatchesPerCellSide;

	m_GrassShaderOverrides.m_ParamOverrides.clear();
	if (m_pGrassShader != nullptr) {
		m_GrassShaderOverrides.m_pShader = m_pGrassShader;
	} else {
		m_GrassShaderOverrides.m_pShader = (kbShader*)g_ResourceManager.GetResource("./assets/Shaders/Environment/grass.kbshader", true, true);
	}

	m_GrassShaderOverrides.SetTexture("heightMap", m_pOwningTerrainComponent->GetHeightMap());
	m_GrassShaderOverrides.SetVec4List("bladeOffsets", bladeOffsets);
	m_GrassShaderOverrides.SetVec4("GrassData0", Vec4(m_PatchStartCullDistance, 1.0f / (m_PatchEndCullDistance - m_PatchStartCullDistance), m_BladeMinHeight, m_BladeMaxHeight));
	m_GrassShaderOverrides.SetVec4("GrassData1", Vec4(m_pOwningTerrainComponent->GetHeightScale(), m_pOwningTerrainComponent->GetOwner()->GetPosition().y, patchLen, 0.0f));
	m_GrassShaderOverrides.SetVec4("fakeAOData", Vec4(m_FakeAODarkness, m_FakeAOPower, m_FakeAOClipPlaneFadeStartDist, 0.0f));

	for (int i = 0; i < m_ShaderParamList.size(); i++) {
		if (m_ShaderParamList[i].GetParamName().stl_str().empty()) {
			continue;
		}

		if (m_ShaderParamList[i].GetTexture() != nullptr) {
			m_GrassShaderOverrides.SetTexture(m_ShaderParamList[i].GetParamName().stl_str(), m_ShaderParamList[i].GetTexture());
		} else {
			m_GrassShaderOverrides.SetVec4(m_ShaderParamList[i].GetParamName().stl_str(), m_ShaderParamList[i].GetVector());
		}
	}

	const Vec2 collisionMapPos = Vec2(m_pOwningTerrainComponent->GetOwner()->GetPosition().x, m_pOwningTerrainComponent->GetOwner()->GetPosition().z);
	m_GrassShaderOverrides.SetVec4("collisionMapCenter", Vec4(collisionMapPos.x, collisionMapPos.y, m_pOwningTerrainComponent->GetTerrainWidth() * 0.5f, 1.0f / (m_pOwningTerrainComponent->GetTerrainWidth() * 0.5f)));

	struct pixelData {
		byte r;
		byte g;
		byte b;
		byte a;
	};
	static const kbString skGrassMaskMap("grassMaskMap");
	const pixelData* pGrassMaskMap = nullptr;
	uint grassMaskWidth = 0, grassMaskHeight = 0;

	for (size_t i = 0; i < m_ShaderParamList.size(); i++) {
		kbShaderParamComponent& curParam = m_ShaderParamList[i];
		if (curParam.GetTexture() == nullptr) {
			continue;
		}

		if (curParam.GetParamName() != skGrassMaskMap) {
			continue;
		}

		kbTexture* const pTex = const_cast<kbTexture*>(curParam.GetTexture());		// GetCPUTexture() is not a const function as it modifies internal state
		pGrassMaskMap = (pixelData*)pTex->GetCPUTexture(grassMaskWidth, grassMaskHeight);
		break;
	}


	if (m_bUpdatePointCloud) {
		for (int i = 0; i < m_GrassRenderObjects.size(); i++) {
			g_pRenderer->RemoveRenderObject(m_GrassRenderObjects[i].m_RenderObject);
			m_GrassRenderObjects[i].Shutdown();
		}
		m_GrassRenderObjects.clear();

		m_GrassRenderObjects.insert(m_GrassRenderObjects.begin(), (size_t)(m_GrassCellsPerTerrainSide * m_GrassCellsPerTerrainSide), grassRenderObject_t());
		const float halfCellLen = m_GrassCellLength * 0.5f;
		const float halfCellLenSqr = sqrt(halfCellLen * halfCellLen);

		const float halfTerrainWidth = m_pOwningTerrainComponent->GetTerrainWidth() * 0.5f;
		const Vec3 terrainMin = /*m_pOwningTerrainComponent->GetOwner()->GetPosition()*/ -Vec3(halfTerrainWidth, 0.0f, halfTerrainWidth);

		const Mat4 ownerRot = m_pOwningTerrainComponent->GetOwner()->GetOrientation().to_mat4();
		const Vec3 ownerPos = m_pOwningTerrainComponent->GetOwner()->GetPosition();
		const auto& grassZones = m_pOwningTerrainComponent->GetGrassZones();

		int cellIdx = 0;
		for (int yCell = 0; yCell < m_GrassCellsPerTerrainSide; yCell++) {
			for (int xCell = 0; xCell < m_GrassCellsPerTerrainSide; xCell++, cellIdx++) {

				grassRenderObject_t& renderObj = m_GrassRenderObjects[cellIdx];
				renderObj.Initialize(m_pOwningTerrainComponent->GetOwner()->GetPosition());
				renderObj.m_RenderObject.m_CullDistance = m_PatchEndCullDistance + halfCellLenSqr;

				const Vec3 cellStart = terrainMin + Vec3(m_GrassCellLength * xCell, 0.0f, m_GrassCellLength * yCell);
				const Vec3 cellCenter = cellStart + Vec3(m_GrassCellLength * 0.5f, 0.0f, m_GrassCellLength * 0.5f);
				const Vec3 halfCell = Vec3(m_GrassCellLength * 0.5f, 0.0f, m_GrassCellLength * 0.5f);

				patchVertLayout* pVerts = nullptr;
				bool bCreatedPointCloud = false;

				int iVert = 0;
				for (int startY = 0; startY < PatchesPerCellSide; startY++) {
					for (int startX = 0; startX < PatchesPerCellSide; startX++) {

						const Vec3 patchJitterOffset = Vec3(m_MaxPatchJitterOffset * kbfrand(), 0.0f, m_MaxPatchJitterOffset * kbfrand());
						const Vec3 globalPointPos = cellStart + Vec3(patchLen * startX, 0.0f, patchLen * startY) + patchJitterOffset;
						const float curU = kbSaturate((globalPointPos.x - terrainMin.x) / m_pOwningTerrainComponent->GetTerrainWidth());
						const float curV = kbSaturate((globalPointPos.z - terrainMin.z) / m_pOwningTerrainComponent->GetTerrainWidth());

						if (g_bCullGrass) {
							bool bSkipIt = true;
							const Vec3 pointWorldPos = ownerRot.transform_point(globalPointPos) + ownerPos;
							for (int i = 0; i < grassZones.size(); i++) {

								Vec3 boundsCenter = ownerRot.transform_point(grassZones[i].GetCenter()) + ownerPos;
								Vec3 boundsExtent = grassZones[i].GetExtents();

								const Vec3 boundsMin = boundsCenter - boundsExtent;
								const Vec3 boundsMax = boundsCenter + boundsExtent;
								const kbBounds grassBounds = kbBounds(boundsMin, boundsMax);
								if (grassBounds.ContainsPoint(pointWorldPos)) {
									bSkipIt = false;
									break;
								}
							}

							if (bSkipIt) {
								continue;
							}
						}

						if (pGrassMaskMap != nullptr) {

							const int textureIndex = static_cast<int>(((int)(curV * grassMaskWidth) * grassMaskWidth) + (curU * grassMaskWidth));
							if (pGrassMaskMap[textureIndex].g == 0) {
								continue;
							}
						}

						if (bCreatedPointCloud == false) {
							renderObj.m_pModel->CreatePointCloud(m_PatchesPerCellSide * m_PatchesPerCellSide, "./assets/Shaders/Environment/grass.kbshader", CullMode_None, sizeof(patchVertLayout));

							pVerts = (patchVertLayout*)renderObj.m_pModel->MapVertexBuffer();
							bCreatedPointCloud = true;
						}

						Vec3 localPointPos = patchJitterOffset + Vec3(patchLen * startX, 0.0f, patchLen * startY) - halfCell;
						pVerts[iVert].position = localPointPos;

						pVerts[iVert].uv.set(curU, curV);
						pVerts[iVert].patchIndices[0] = rand() % 60;		// Randomized blade jitters
						pVerts[iVert].patchIndices[1] = pVerts[iVert].patchIndices[2] = pVerts[iVert].patchIndices[3] = pVerts[iVert].patchIndices[0];
						const float randVal = kbfrand();

						pVerts[iVert].patchIndices[2] = 0;		// Unused I believe
						iVert++;
					}
				}
				if (bCreatedPointCloud) {
					renderObj.m_pModel->UnmapVertexBuffer(iVert);
					Mat4 rotMat = m_pOwningTerrainComponent->GetOwnerRotation().to_mat4();
					m_GrassRenderObjects[cellIdx].m_RenderObject.m_Position = cellCenter * rotMat + m_pOwningTerrainComponent->GetOwnerPosition();
					m_GrassRenderObjects[cellIdx].m_RenderObject.m_Scale = m_pOwningTerrainComponent->GetOwnerScale();
					m_GrassRenderObjects[cellIdx].m_RenderObject.m_Orientation = m_pOwningTerrainComponent->GetOwnerRotation();

					auto& renderObjMatList = m_GrassRenderObjects[cellIdx].m_RenderObject.m_Materials;
					renderObjMatList.clear();
					renderObjMatList.push_back(m_GrassShaderOverrides);
					g_pRenderer->AddRenderObject(m_GrassRenderObjects[cellIdx].m_RenderObject);
				}
			}
		}
	} else {

		for (int i = 0; i < m_GrassRenderObjects.size(); i++) {
			auto& renderObjMatList = m_GrassRenderObjects[i].m_RenderObject.m_Materials;
			renderObjMatList.clear();
			renderObjMatList.push_back(m_GrassShaderOverrides);
			g_pRenderer->UpdateRenderObject(m_GrassRenderObjects[i].m_RenderObject);
		}
	}

	m_bUpdateMaterial = false;
	m_bUpdatePointCloud = false;

	blk::log("Refreshing grass took %f seconds.", g_GlobalTimer.TimeElapsedSeconds() - startRefreshGrassTime);
}

/**
 *	kbTerrainComponent::Constructor
 */
void kbTerrainComponent::Constructor() {

	m_pHeightMap = nullptr;
	m_HeightScale = 0.3f;
	m_TerrainWidth = 256.0f;
	m_TerrainDimensions = 16;
	m_TerrainSmoothAmount = 1;
	m_bDebugForceRegenTerrain = false;

	m_pSplatMap = nullptr;

	m_LastHeightMapLoadTime = -1.0f;
	m_bRegenerateTerrain = false;
}

/**
 *	kbTerrainComponent::kbTerrainComponent
 */
kbTerrainComponent::~kbTerrainComponent() {
	if (m_pHeightMap) {
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

	if (m_pHeightMap != nullptr) {
		m_bRegenerateTerrain = true;
	}

	for (int i = 0; i < m_Grass.size(); i++) {
		m_Grass[i].SetOwningTerrainComponent(this);
	}
}

/**
 *	kbTerrainComponent::EditorChange
 */
void kbTerrainComponent::EditorChange(const std::string& propertyName) {
	Super::EditorChange(propertyName);

	const std::string propertiesThatRegenTerrain[5] = { "HeightMap", "HeightScale", "Width", "Dimensions", "SmoothAmount" };

	for (int i = 0; i < 5; i++) {
		if (propertyName == propertiesThatRegenTerrain[i]) {
			m_bRegenerateTerrain = true;
		}
	}

	if (m_bRegenerateTerrain) {
		return;
	}

	RefreshMaterials();

	if (IsEnabled()) {
		g_pRenderer->UpdateRenderObject(m_RenderObject);
	}
}

/**
 *	kbTerrainComponent::GenerateTerrain
 */
void kbTerrainComponent::GenerateTerrain() {
	blk::error_check(m_pHeightMap != nullptr, "kbTerrainComponent::GenerateTerrain() - No height map file found for terrain component on entity %s", GetOwner()->GetName().c_str());

	struct pixelData {
		byte r;
		byte g;
		byte b;
		byte a;
	};

	terrainNormals.clear();

	unsigned int texWidth, texHeight;

	const pixelData* const pTextureBuffer = (pixelData*)m_pHeightMap->GetCPUTexture(texWidth, texHeight);

	// Build terrain here
	const int numVerts = m_TerrainDimensions * m_TerrainDimensions;
	const unsigned int numIndices = (m_TerrainDimensions - 1) * (m_TerrainDimensions - 1) * 6;
	const float HalfTerrainWidth = m_TerrainWidth * 0.5f;
	const float stepSize = m_TerrainWidth / (float)texWidth;
	const float cellWidth = m_TerrainWidth / (float)m_TerrainDimensions;

	if (m_TerrainModel.NumVertices() > 0) {
		g_pRenderer->RemoveRenderObject(m_RenderObject);
	}

	m_TerrainModel.CreateDynamicModel(numVerts, numIndices);

	vertexLayout* const pVerts = (vertexLayout*)m_TerrainModel.MapVertexBuffer();
	std::vector<Vec3> cpuVerts;
	cpuVerts.resize((size_t)m_TerrainDimensions * m_TerrainDimensions);

	int currentVert = 0;
	for (int startY = 0; startY < m_TerrainDimensions; startY++) {
		for (int startX = 0; startX < m_TerrainDimensions; startX++) {


			//	int textureIndex = ( v * texWidth ) + u;

			float divisor = 0.0f;
			float height = 0.0f;
			const int blurSampleSize = max(m_TerrainSmoothAmount, 1);
			for (int tempY = 0; tempY < blurSampleSize; tempY++) {

				if (tempY + startY >= m_TerrainDimensions) {
					break;
				}

				for (int tempX = 0; tempX < blurSampleSize; tempX++) {
					if (tempX + startX >= m_TerrainDimensions) {
						break;
					}

					const float u = (float)(startX + tempX) / (float)m_TerrainDimensions;
					const float v = (float)(startY + tempY) / (float)m_TerrainDimensions;
					const int textureIndex = static_cast<int>((v * texWidth * texWidth) + (u * texWidth));

					divisor += 1.0f;
					height += (float)pTextureBuffer[textureIndex].r;
				}
			}
			height /= 255.0f;
			height *= (m_HeightScale / divisor);
			pVerts[currentVert].Clear();
			cpuVerts[currentVert] = Vec3(-HalfTerrainWidth + ((startX + 1) * cellWidth), height, -HalfTerrainWidth + ((startY + 1) * cellWidth));
			pVerts[currentVert].position = cpuVerts[currentVert];
			cpuVerts[currentVert] += GetOwner()->GetPosition();

			pVerts[currentVert].uv.set((float)(startX) / (float)m_TerrainDimensions, (float)(startY) / (float)m_TerrainDimensions);
			pVerts[currentVert].SetColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
			pVerts[currentVert].SetNormal(Vec4(0.0f, 1.0f, 0.0f, 0.0f));
			currentVert++;
		}
	}
	m_TerrainModel.UnmapVertexBuffer();

	ushort* pIndices = (ushort*)m_TerrainModel.MapIndexBuffer();
	int currentIndexToWrite = 0;

	for (int startY = 0; startY < m_TerrainDimensions; startY++) {
		for (int startX = 0; startX < m_TerrainDimensions; startX++) {
			int currentIndex = (startY * m_TerrainDimensions) + startX;

			Vec3 xVec, zVec;

			if (startX < m_TerrainDimensions - 1) {
				xVec = pVerts[currentIndex + 1].position - pVerts[currentIndex].position;
			} else {
				xVec = pVerts[currentIndex].position - pVerts[currentIndex - 1].position;
			}

			if (startY < m_TerrainDimensions - 1) {
				zVec = pVerts[currentIndex].position - pVerts[currentIndex + m_TerrainDimensions].position;
			} else {
				zVec = pVerts[currentIndex - m_TerrainDimensions].position - pVerts[currentIndex].position;
			}

			xVec.normalize_self();
			zVec.normalize_self();
			Vec3 finalVec = xVec.cross(zVec).normalize_safe();

			xVec = finalVec.cross(zVec).normalize_safe();
			zVec = xVec.cross(finalVec).normalize_safe();

			pVerts[currentIndex].SetTangent(-zVec);
			pVerts[currentIndex].SetBitangent(xVec);

			debugNormal newNormal;
			newNormal.normal = xVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
			terrainNormals.push_back(newNormal);

			newNormal.normal = zVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
			terrainNormals.push_back(newNormal);

			newNormal.normal = finalVec;
			newNormal.position = pVerts[currentIndex].position + GetOwner()->GetPosition();
			terrainNormals.push_back(newNormal);
		}
	}

	for (int y = 0, triIdx = 0; y < m_TerrainDimensions - 1; y++) {
		for (int x = 0; x < m_TerrainDimensions - 1; x++, triIdx += 2) {

			const unsigned int currentIndex = (y * m_TerrainDimensions) + x;
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

	RefreshMaterials();

	g_pRenderer->AddRenderObject(m_RenderObject);

	// Update collision
	int collisionPatchSize = 8;

	std::vector<kbCollisionComponent::customTriangle_t> terrainCollision;
	terrainCollision.resize((size_t)((m_TerrainDimensions / collisionPatchSize) * (m_TerrainDimensions / collisionPatchSize)) * 2);

	size_t triIdx = 0;
	for (int y = 0; y < m_TerrainDimensions - collisionPatchSize; y += collisionPatchSize) {
		for (int x = 0; x < m_TerrainDimensions - collisionPatchSize; x += collisionPatchSize, triIdx += 2) {
			const size_t currentIndex = ((size_t)y * m_TerrainDimensions) + x;
			terrainCollision[triIdx + 0].m_Vertex1 = cpuVerts[currentIndex];
			terrainCollision[triIdx + 0].m_Vertex2 = cpuVerts[currentIndex + collisionPatchSize];
			terrainCollision[triIdx + 0].m_Vertex3 = cpuVerts[currentIndex + ((size_t)collisionPatchSize * m_TerrainDimensions)];

			terrainCollision[triIdx + 1].m_Vertex1 = cpuVerts[currentIndex + collisionPatchSize];
			terrainCollision[triIdx + 1].m_Vertex2 = cpuVerts[currentIndex + collisionPatchSize + ((size_t)m_TerrainDimensions * collisionPatchSize)];
			terrainCollision[triIdx + 1].m_Vertex3 = cpuVerts[currentIndex + ((size_t)collisionPatchSize * m_TerrainDimensions)];
		}
	}

	kbCollisionComponent* const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType(kbCollisionComponent::GetType());
	if (pCollision != nullptr) {
		pCollision->SetCustomTriangleCollision(terrainCollision);
	}
}

/**
 *  kbTerrainComponent::SetCollisionMap
 */
void kbTerrainComponent::SetCollisionMap(const kbRenderTexture* const pTexture) {
	for (int i = 0; i < m_Grass.size(); i++) {

		kbGrass& grass = m_Grass[i];
		grass.m_GrassShaderOverrides.SetTexture("collisionMap", pTexture);
		grass.m_GrassShaderOverrides.SetVec4("collisionMapPixelWorldSize", Vec4(GetTerrainWidth() / pTexture->GetWidth(), 0.0f, 0.0f, 0.0f));

		for (int cellIdx = 0; cellIdx < grass.m_GrassRenderObjects.size(); cellIdx++) {
			auto& grassRenderObj = grass.m_GrassRenderObjects[cellIdx];
			auto& renderObjMatList = grassRenderObj.m_RenderObject.m_Materials;
			renderObjMatList.clear();
			renderObjMatList.push_back(grass.m_GrassShaderOverrides);
			g_pRenderer->UpdateRenderObject(grassRenderObj.m_RenderObject);
		}
	}
}

/**
 *	kbTerrainComponent::SetEnable_Internal
 */
void kbTerrainComponent::SetEnable_Internal(const bool isEnabled) {

	if (m_TerrainModel.NumVertices() == 0) {
		return;
	}

	if (m_LastHeightMapLoadTime == -1.0f && m_pHeightMap != nullptr) {
		m_LastHeightMapLoadTime = m_pHeightMap->GetLastLoadTime();
	}

	if (isEnabled) {
		RefreshMaterials();
		g_pRenderer->AddRenderObject(m_RenderObject);

		for (int i = 0; i < m_Grass.size(); i++) {
			m_Grass[i].Enable(true);
		}

	} else {
		g_pRenderer->RemoveRenderObject(m_RenderObject);

		for (int i = 0; i < m_Grass.size(); i++) {
			m_Grass[i].Enable(false);
		}
	}
}

/**
 *	kbTerrainComponent::Update_Internal
 */
void kbTerrainComponent::Update_Internal(const float DeltaTime) {
	Super::Update_Internal(DeltaTime);

	if (m_pHeightMap != nullptr && m_pHeightMap->GetLastLoadTime() != m_LastHeightMapLoadTime) {
		m_LastHeightMapLoadTime = m_pHeightMap->GetLastLoadTime();
		this->RegenerateTerrain();
	}

	if (m_TerrainModel.GetMeshes().size() > 0 && (GetOwner()->IsDirty() || m_bDebugForceRegenTerrain == true)) {
		RefreshMaterials();
		RegenerateTerrain();
		g_pRenderer->UpdateRenderObject(m_RenderObject);
		m_bDebugForceRegenTerrain = false;
	}

	/*
		const Mat4 ownerRot = GetOwnerRotation().to_mat4();
		const Vec3 ownerPos = GetOwnerPosition();
		for ( int i = 0; i < m_GrassZones.size(); i++ ) {

			Vec3 boundsCenter = ownerRot.transform_point( m_GrassZones[i].GetCenter() ) + ownerPos;
			Vec3 boundsExtent = m_GrassZones[i].GetExtents();

			const Vec3 boundsMin = boundsCenter - boundsExtent;
			const Vec3 boundsMax = boundsCenter + boundsExtent;
			g_pRenderer->DrawBox( kbBounds( boundsMin, boundsMax ), kbColor::red );
		}*/
}

/**
 *	kbTerrainComponent::RenderSync
 */
void kbTerrainComponent::RenderSync() {
	Super::RenderSync();

	if (m_bRegenerateTerrain) {
		GenerateTerrain();
		for (int i = 0; i < m_Grass.size(); i++) {
			m_Grass[i].m_bUpdatePointCloud = true;
			m_Grass[i].RefreshGrass();
		}
		m_bRegenerateTerrain = false;
	}

	for (int i = 0; i < m_Grass.size(); i++) {
		m_Grass[i].RenderSync();
	}
}

/**
 *	kbTerrainComponent::RefreshMaterials
 */
void kbTerrainComponent::RefreshMaterials() {
	m_RenderObject.m_bCastsShadow = false;
	m_RenderObject.m_bIsSkinnedModel = false;
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_EntityId = GetOwner()->GetEntityId();
	m_RenderObject.m_Scale.set(1.0f, 1.0f, 1.0f);
	m_RenderObject.m_pModel = &m_TerrainModel;
	m_RenderObject.m_RenderPass = RP_Lighting;
	m_RenderObject.m_pComponent = this;
}

/// kbGrassZone::Constructor
void kbGrassZone::Constructor() {
	m_Center.set(0.0f, 0.0f, 0.0f);
	m_Extents.set(100.0f, 100.0f, 100.0f);
}