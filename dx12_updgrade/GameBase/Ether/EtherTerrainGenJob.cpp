//===================================================================================================
// EtherTerrainGenJob.cpp
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbMath.h"
#include "kbJobManager.h"
#include "EtherWorldGen.h"

/**
 *	CalculateNormalFromTriangle
 */
kbVec3 CalculateNormalFromTriangle( const kbVec3 & v0, const kbVec3 & v1, const kbVec3 & v2 ) {
	kbVec3 rightVec = v1 - v2;
	kbVec3 upVec = v0 - v2;

	rightVec.Normalize();
	upVec.Normalize();

	kbVec3 surfaceNormal = rightVec.Cross( upVec );
	return surfaceNormal.Normalized();
}

/**
 *	GenerateTerrainChunk
 */
void GenerateTerrainChunk( vertexLayout *const pTerrainVerts, const int x, const int y, const int numVertsPerSide, const int actualChunkDimensions, const float DiamondSquareMidPtMaxHeight ) {

	if ( numVertsPerSide <= 2 ) {
		return;
	}

	const int halfNumVertsPerSide = numVertsPerSide / 2;
	const int centerVertX = halfNumVertsPerSide;
	const int centerVertY = halfNumVertsPerSide;

	const int UL = x + ( y * actualChunkDimensions );
	const int UR = UL + numVertsPerSide - 1;
	const int LR = x + ( y + ( numVertsPerSide - 1 ) ) * actualChunkDimensions + numVertsPerSide - 1;
	const int LL = LR - numVertsPerSide + 1;

	float centerVertHeight = pTerrainVerts[UL].position.y + pTerrainVerts[UR].position.y + pTerrainVerts[LL].position.y + pTerrainVerts[LR].position.y;
	centerVertHeight /= 4;
	centerVertHeight += ( kbfrand() - 0.5f ) * DiamondSquareMidPtMaxHeight; 

	const int centerVertIndex = ( y + halfNumVertsPerSide ) * actualChunkDimensions + ( x + halfNumVertsPerSide );
	pTerrainVerts[ centerVertIndex ].position.y = centerVertHeight;
	
	const int upperMid = ( y * actualChunkDimensions ) + ( x + halfNumVertsPerSide );
	pTerrainVerts[ upperMid ].position.y = ( pTerrainVerts[UL].position.y + pTerrainVerts[UR].position.y ) * 0.5f;

	const int lowerMid = ( ( y + numVertsPerSide - 1 ) * actualChunkDimensions )  + ( x + halfNumVertsPerSide );
	pTerrainVerts[ lowerMid ].position.y = ( pTerrainVerts[LL].position.y + pTerrainVerts[LR].position.y ) * 0.5f;

	const int leftMid = ( y + halfNumVertsPerSide ) * actualChunkDimensions + x;
	pTerrainVerts[ leftMid ].position.y = ( pTerrainVerts[UL].position.y + pTerrainVerts[LL].position.y ) * 0.5f;

	const int rightMid = leftMid + numVertsPerSide - 1;
	pTerrainVerts[ rightMid ].position.y = ( pTerrainVerts[UR].position.y + pTerrainVerts[LR].position.y ) * 0.5f;

	const int newNumVerts = ( numVertsPerSide / 2 ) + 1;
	GenerateTerrainChunk( pTerrainVerts, x, y, newNumVerts, actualChunkDimensions, DiamondSquareMidPtMaxHeight );
	GenerateTerrainChunk( pTerrainVerts, x + newNumVerts - 1, y, newNumVerts, actualChunkDimensions, DiamondSquareMidPtMaxHeight );
	GenerateTerrainChunk( pTerrainVerts, x, y + newNumVerts - 1, newNumVerts, actualChunkDimensions, DiamondSquareMidPtMaxHeight );
	GenerateTerrainChunk( pTerrainVerts, x + newNumVerts - 1, y + newNumVerts - 1, newNumVerts, actualChunkDimensions, DiamondSquareMidPtMaxHeight );
}

/**
 *	GenerateFlatShadedTriangleIndices
 */
struct worldObjectInfo_t {
	kbVec3 loc;
	int type;
	const EtherEnviroObject * m_pEnviroObject;
};

void GenerateFlatShadedTriangleIndices( vertexLayout *const pInputVerts, vertexLayout *const pOutputVerts, unsigned long *const m_IndexBufferOutput, std::vector<kbVec3> & collisionOutput, int cellDimensions, EtherTerrainChunkGenJob *const pTerrainJob, std::vector<kbVec3> & dynamicCollisionOuput ) {
	/*const int numVertsPerSide = cellDimensions + 1;
	const int totalNumVerts = numVertsPerSide * numVertsPerSide;
	int iVertexBuffer = 0;

	vertexLayout *const pOutVerts = new vertexLayout[cellDimensions * cellDimensions * 2 * 3 + g_MaxDynamicVertices];

	std::vector<worldObjectInfo_t> worldObjects;

	int cellSkipper = -1;
	for ( int z = 0; z < cellDimensions; z++ ) {
		for ( int x = 0; x < cellDimensions; x++ ) {
			int index = ( z * numVertsPerSide ) + x;

			int order[6];

			if ( ( x + z ) & 1 ) {
				order[0] = index;
				order[1] = index + numVertsPerSide + 1;
				order[2] = index + numVertsPerSide;
				order[3] = index;
				order[4] = index + 1;
				order[5] = index + numVertsPerSide + 1;
			} else {
				order[0] = index;
				order[1] = index + 1;
				order[2] = index + numVertsPerSide;
				order[3] = index + 1;
				order[4] = index + numVertsPerSide + 1;
				order[5] = index + numVertsPerSide;
			}

			pOutVerts[iVertexBuffer + 0] = pInputVerts[order[0]];
			pOutVerts[iVertexBuffer + 1] = pInputVerts[order[1]];
			pOutVerts[iVertexBuffer + 2] = pInputVerts[order[2]];

			m_IndexBufferOutput[iVertexBuffer + 0] = iVertexBuffer + 2;
			m_IndexBufferOutput[iVertexBuffer + 1] = iVertexBuffer + 1;
			m_IndexBufferOutput[iVertexBuffer + 2] = iVertexBuffer + 0;

			kbVec3 triNormal = CalculateNormalFromTriangle( pOutVerts[m_IndexBufferOutput[iVertexBuffer+1]].position, pOutVerts[m_IndexBufferOutput[iVertexBuffer+0]].position, pOutVerts[m_IndexBufferOutput[iVertexBuffer+2]].position );		

			pOutVerts[iVertexBuffer + 0].SetNormal( triNormal );
			pOutVerts[iVertexBuffer + 1].SetNormal( triNormal );
			pOutVerts[iVertexBuffer + 2].SetNormal( triNormal );

			// Decrease that chance that two co-planar polies have the same normal (and create a diamond looking shape)
			const float randomizer = ( rand() % 2000 ) / 10000.0f;
			triNormal *= 1.0f + randomizer;

			pOutVerts[iVertexBuffer + 3] = pInputVerts[order[3]];
			pOutVerts[iVertexBuffer + 4] = pInputVerts[order[4]];
			pOutVerts[iVertexBuffer + 5] = pInputVerts[order[5]];

			m_IndexBufferOutput[iVertexBuffer + 3] = iVertexBuffer + 5;
			m_IndexBufferOutput[iVertexBuffer + 4] = iVertexBuffer + 4;
			m_IndexBufferOutput[iVertexBuffer + 5] = iVertexBuffer + 3;

			triNormal = CalculateNormalFromTriangle( pOutVerts[m_IndexBufferOutput[iVertexBuffer+4]].position, pOutVerts[m_IndexBufferOutput[iVertexBuffer+3]].position, pOutVerts[m_IndexBufferOutput[iVertexBuffer+5]].position );
			pOutVerts[iVertexBuffer + 3].SetNormal( triNormal );
			pOutVerts[iVertexBuffer + 4].SetNormal( triNormal );
			pOutVerts[iVertexBuffer + 5].SetNormal( triNormal );

			// Collision triangles
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 0].position );
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 1].position );
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 2].position );
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 3].position );
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 4].position );
			collisionOutput.push_back( pOutVerts[iVertexBuffer + 5].position );

			if ( cellSkipper == -1 ) {
				for ( int iPass = 0; iPass < 2; iPass++ ) {
					float RandChance = 0.25f;
					const std::vector<EtherEnviroObject> * pEnviroObjectList = nullptr;

					if ( iPass == 0 ) {
						if ( ( x % 1 ) != 0 || ( z % 1) != 0 ) {
							continue;
						}

						RandChance = 0.49f;
						pEnviroObjectList = &pTerrainJob->m_EnviroComponent->GetCoverObjects();
					} else if ( iPass == 1 ) {
						if ( ( x % 1 ) != 0 || ( z % 1 ) != 0 ) {
							continue;
						}
						RandChance = 0.25f;
						pEnviroObjectList = &pTerrainJob->m_EnviroComponent->GetEnviroObjects();
					}

					if ( pEnviroObjectList == nullptr ) {
						continue;
					}

					float intPart;
					float RandGen = kbfrand();//NormalizedNoise( pInputVerts[order[4]].position.x * 0.000100f, pInputVerts[order[4]].position.z * 0.000100f);
					modf( RandGen * 100.0f, &intPart );
					if ( RandGen < RandChance ) {
						continue;
					}

					worldObjectInfo_t newWorldObjectInfo;

					const kbVec3 coverPos = 0.25f * ( pInputVerts[order[0]].position + pInputVerts[order[1]].position + pInputVerts[order[2]].position + pInputVerts[order[3]].position );
					newWorldObjectInfo.loc = coverPos;
					newWorldObjectInfo.m_pEnviroObject = &(*pEnviroObjectList)[(int)intPart % pEnviroObjectList->size()];
					newWorldObjectInfo.type = iPass;

					worldObjects.push_back( newWorldObjectInfo );
				}
			} else {
				cellSkipper++;
				if ( cellSkipper >= 4 ) {
					cellSkipper = -1;
				}
			}
			iVertexBuffer += 6;
		}
	}

	kbMat4 rotationMatrix;
	rotationMatrix.MakeIdentity();

	std::vector<EtherCoverObject> & outCoverObjectList = *pTerrainJob->m_pCoverObjects;

	int iIndexBuffer = iVertexBuffer;
	int iCollisionBuffer = 0;
	for ( int iObj = 0; iObj < worldObjects.size(); iObj++ ) {
		const float rotationAngle = kbfrand() * kbPI;
		rotationMatrix[0][0] = cos( rotationAngle );
		rotationMatrix[2][0] = -sin( rotationAngle );
		rotationMatrix[0][2] = -rotationMatrix[2][0];
		rotationMatrix[2][2] = rotationMatrix[0][0];
		
		const EtherEnviroObject & EnviroObject = *worldObjects[iObj].m_pEnviroObject;
		const kbModel *const pModel = EnviroObject.GetModel();
		const std::vector<vertexLayout>	& dynVerts = pModel->GetCPUVertices();
		const std::vector<ushort> & dynIndices = pModel->GetCPUIndices();
		
		if ( dynVerts.size() + pTerrainJob->m_NumDynamicVertices > g_MaxDynamicVertices || dynIndices.size() + pTerrainJob->m_NumDynamicIndices > g_MaxDynamicVertices ) {
			continue;
		}
		
		pTerrainJob->m_NumDynamicVertices += (int)dynVerts.size();
		pTerrainJob->m_NumDynamicIndices += (int)dynIndices.size();
		const kbVec3 objScale = EnviroObject.GetMinScale() + ( EnviroObject.GetMaxScale() - EnviroObject.GetMinScale() ) * kbfrand();
		const int startVert = iVertexBuffer;

		if ( worldObjects[iObj].type == 0 ) {
			const kbVec3 minBounds = pModel->GetBounds().Min() * objScale.x + worldObjects[iObj].loc;
			const kbVec3 maxBounds = pModel->GetBounds().Max() * objScale.x + worldObjects[iObj].loc;
			
			outCoverObjectList.push_back( EtherCoverObject( kbBounds( minBounds, maxBounds ), 200 ) );
		//	kbLog( "Cover obj pos = %f %f %f", outCoverObjectList[outCoverObjectList.size()-1].GetPosition().x, outCoverObjectList[outCoverObjectList.size()-1].GetPosition().y, outCoverObjectList[outCoverObjectList.size()-1].GetPosition().z );
			EtherCoverObject & NewCoverObject = outCoverObjectList[ outCoverObjectList.size() - 1 ];
		}

		for ( int iVert = 0; iVert < dynVerts.size(); iVert++, iVertexBuffer++ ) {
			vertexLayout newVert = dynVerts[iVert];
			newVert.position.x *= objScale.x;
			newVert.position.y *= objScale.y;
			newVert.position.z *= objScale.z;
			newVert.position = rotationMatrix.TransformPoint( newVert.position );
			newVert.SetNormal( rotationMatrix.TransformPoint( newVert.GetNormal() ) );
			newVert.position += worldObjects[iObj].loc;
		
			pOutVerts[iVertexBuffer] = newVert;
		}
		
		const size_t dynIdxSize = dynIndices.size();
		for ( int iIndex = 0; iIndex < dynIdxSize; iIndex++, iIndexBuffer++, iCollisionBuffer++ ) {
			m_IndexBufferOutput[iIndexBuffer] = dynIndices[iIndex] + startVert;
		
			kbVec3 newVert = dynVerts[dynIndices[iIndex]].position;
			newVert.x *= objScale.x;
			newVert.y *= objScale.y;
			newVert.z *= objScale.z;
			newVert = rotationMatrix.TransformPoint( newVert );
			newVert += worldObjects[iObj].loc;
			dynamicCollisionOuput.push_back( newVert );
		}
	}

	// "Zero out" the rest of the indices to avoid garbage triangles
	for ( ; iIndexBuffer < cellDimensions * cellDimensions * 2 * 3 + g_MaxDynamicVertices; iIndexBuffer++ ) {
		m_IndexBufferOutput[iIndexBuffer] = 0;
	}

	// Write out verts
	for ( iVertexBuffer = 0; iVertexBuffer < cellDimensions * cellDimensions * 2 * 3 + g_MaxDynamicVertices; iVertexBuffer++ ) {
		pOutputVerts[iVertexBuffer] = pOutVerts[iVertexBuffer];
	}
	delete[] pOutVerts;*/
}


/**
 *	EtherTerrainChunkGenJob::EtherTerrainChunkGenJob
 */
EtherTerrainChunkGenJob::EtherTerrainChunkGenJob() :
	m_TrisPerChunkSide( 0 ),
	m_ChunkWorldLength( 0 ),
	m_TerrainGenNoiseScale( 0.0f ),
	m_MaxTerrainHeight( 0.0f ),
	m_MaxTerrainCellMidPointHeight( 0.0f ),
	m_Position( kbVec3::zero ),
	m_EnviroComponent( nullptr ),
	m_VertexBufferOutput( nullptr ),
	m_IndexBufferOutput( nullptr ),
	m_CollisionMeshOutput( nullptr ),
	m_DynamicCollisionMeshOutput( nullptr ),
	m_pCoverObjects( nullptr ),
	m_NumDynamicVertices( 0 ),
	m_NumDynamicIndices( 0 ) {
}

/**
 *	EtherTerrainChunkGenJob::Reset
 */
void EtherTerrainChunkGenJob::Reset() {
	m_TrisPerChunkSide = 0;
	m_ChunkWorldLength = 0;
	m_TerrainGenNoiseScale = 0.0f;
	m_MaxTerrainHeight = 0.0f;
	m_MaxTerrainCellMidPointHeight = 0.0f;
	m_Position = kbVec3::zero;
	m_EnviroComponent = nullptr;
	m_VertexBufferOutput = nullptr;
	m_IndexBufferOutput = nullptr;
	m_CollisionMeshOutput = nullptr;
	m_DynamicCollisionMeshOutput = nullptr;
	m_pCoverObjects = nullptr;
	m_NumDynamicVertices = 0 ;
	m_NumDynamicIndices = 0;
}

/**
 *	EtherTerrainChunkGenJob::Run
 */
void EtherTerrainChunkGenJob::Run() {

	const float ChunkHalfLength = m_ChunkWorldLength * 0.5f;
	const float QuadLength = (float)( m_ChunkWorldLength / m_TrisPerChunkSide );
	const int numVertsPerSide = m_TrisPerChunkSide + 1;
	const int totalNumVerts = numVertsPerSide * numVertsPerSide;

	vertexLayout *const pTempTerrainVerts = new vertexLayout[totalNumVerts];

	// Generate vertex positions
	float currentWorldZ = -ChunkHalfLength + m_Position.z;
	int i = 0;
	for ( int index_z = 0; index_z < numVertsPerSide; index_z++, currentWorldZ += QuadLength ) {
		float currentWorldX = -ChunkHalfLength + m_Position.x;
		
		for ( int index_x = 0; index_x < numVertsPerSide; index_x++, currentWorldX += QuadLength, i++ ) {

			pTempTerrainVerts[i].position.x = currentWorldX;
			pTempTerrainVerts[i].position.y = 0;
			pTempTerrainVerts[i].position.z = currentWorldZ;

			pTempTerrainVerts[i].uv.x = 0;
			pTempTerrainVerts[i].uv.y = 0;

			pTempTerrainVerts[i].color[0] = 130;
			pTempTerrainVerts[i].color[1] = 150;
			pTempTerrainVerts[i].color[2] = 75;
			pTempTerrainVerts[i].color[3] = 0;

			pTempTerrainVerts[i].normal[0] = 0;
			pTempTerrainVerts[i].normal[1] = 0;
			pTempTerrainVerts[i].normal[2] = 0;
			pTempTerrainVerts[i].normal[3] = 0;

			pTempTerrainVerts[i].tangent[0] = 0;
			pTempTerrainVerts[i].tangent[1] = 255;
			pTempTerrainVerts[i].tangent[2] = 0;
			pTempTerrainVerts[i].tangent[3] = 0;
		}
	}
	
	// Diamond Square Algorithm
	const int UL = 0;
	const int UR = numVertsPerSide - 1;
	const int LR = numVertsPerSide * numVertsPerSide - 1;
	const int LL = LR - numVertsPerSide + 1;

	const int halfNumVertsPerSide = ( numVertsPerSide + 1 ) / 2;
	const int upperMid = ( numVertsPerSide ) / 2;
	const int lowerMid = LL + ( numVertsPerSide ) / 2;
	const int Center = totalNumVerts / 2;
	const int leftMid = numVertsPerSide * (halfNumVertsPerSide - 1);
	const int rightMid = leftMid + ( numVertsPerSide - 1 );

#define CalculateVertHeightForIndex(vertIndex) pTempTerrainVerts[vertIndex].position.y =  NormalizedNoise( (float) pTempTerrainVerts[vertIndex].position.x * m_TerrainGenNoiseScale, ( float ) pTempTerrainVerts[vertIndex].position.z * m_TerrainGenNoiseScale ) * m_MaxTerrainHeight;
	CalculateVertHeightForIndex( UL )
	CalculateVertHeightForIndex( upperMid )
	CalculateVertHeightForIndex( UR )
	CalculateVertHeightForIndex( leftMid )
	CalculateVertHeightForIndex( Center )
	CalculateVertHeightForIndex( rightMid )
	CalculateVertHeightForIndex( LL )
	CalculateVertHeightForIndex( lowerMid )
	CalculateVertHeightForIndex( LR )

	GenerateTerrainChunk( pTempTerrainVerts, 0, 0, halfNumVertsPerSide, numVertsPerSide, m_MaxTerrainCellMidPointHeight );
	GenerateTerrainChunk( pTempTerrainVerts, halfNumVertsPerSide - 1, 0, halfNumVertsPerSide, numVertsPerSide, m_MaxTerrainCellMidPointHeight );
	GenerateTerrainChunk( pTempTerrainVerts, 0, halfNumVertsPerSide - 1, halfNumVertsPerSide, numVertsPerSide, m_MaxTerrainCellMidPointHeight );
	GenerateTerrainChunk( pTempTerrainVerts, halfNumVertsPerSide - 1, halfNumVertsPerSide - 1, halfNumVertsPerSide, numVertsPerSide, m_MaxTerrainCellMidPointHeight );

	m_NumDynamicVertices = 0;
	m_NumDynamicIndices = 0;
	GenerateFlatShadedTriangleIndices( pTempTerrainVerts, m_VertexBufferOutput, m_IndexBufferOutput, *m_CollisionMeshOutput, m_TrisPerChunkSide, this, *m_DynamicCollisionMeshOutput );

	delete[] pTempTerrainVerts;
}
