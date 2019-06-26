//==============================================================================
// kbModel.cpp
//
// General model format based off of the ms3d specs
//
// 2016-2019 kbEngine 2.0
//==============================================================================
#include <fbxsdk.h>
#include "kbCore.h"
#include "kbVector.h"
#include "kbIntersectionTests.h"
#include "kbModel.h"
#include "kbRenderer.h"

#pragma pack( push, packing )
#pragma pack( 1 )

typedef struct {
	char				m_ID[10];
	int					m_Version;
} ms3dHeader_t;

 typedef struct {
	byte				m_flags;
	float				m_vertex[3];
	char				m_boneID;
	byte				m_refCount;
} ms3dVertex_t;

 typedef struct {
	 ushort				m_Flags;
	 ushort				m_VertexIndices[3];
	 float				m_VertexNormals[3][3];
	 float				u[3];
	 float				v[3];
	 byte				m_smoothingGroup;
	 byte				m_GroupIndex;
 } ms3dTriangle_t;

typedef struct {
    char				m_Name[32];
    float				m_Ambient[4];
    float				m_Diffuse[4];
    float				m_Specular[4];
    float				m_Emissive[4];
    float				m_Shininess;
    float				m_Transparency;
    char				m_Mode;
    char				m_Texture[128];
    char				m_AlphaMap[128];
} ms3dMaterial_t;

typedef struct {
	float				m_Time;
	float				m_Rotation[3];
} ms3dRotationKeyFrame_t;

typedef struct {
	float				m_Time;
	float				m_Position[3];
} ms3dPositionKeyFrame_t;

typedef struct {
	byte				m_Flags;
	char				m_Name[32];
	char				m_ParentName[32];
	float				m_Rotation[3];
	float				m_Position[3];
	ushort				m_NumRotationKeyFrames;
	ushort				m_NumPositionKeyFrames;
} ms3dBone_t;

#pragma pack( pop, packing )

/**
 *	kbModel::kbModel
 */
kbModel::kbModel() :
	m_NumVertices( 0 ),
	m_NumTriangles( 0 ),
	m_Stride( sizeof( vertexLayout ) ),
	m_bIsDynamicModel( false ),
    m_bIsPointCloud( false ),
	m_bVBIsMapped( false ),
	m_bIBIsMapped( false ),
	m_bCPUAccessOnly( false ) {
}

/**
 *	kbModel::~kbModel
 */
kbModel::~kbModel() {
	Release_Internal();
}

/**
 *	kbModel::Load_Internal
 */
bool kbModel::Load_Internal() {
	const std::string fileExt = GetFileExtension( GetFullFileName() );
	if ( fileExt == "ms3d" ) {
		return LoadMS3D();
	} else if ( fileExt == "fbx" ) {
		return LoadFBX();
	} else if ( fileExt == "diablo3" ) {
		return LoadDiablo3();
	}

	return false;
}

/**
 *	kbModel::LoadMS3D
 */
bool kbModel::LoadMS3D() {
	std::ifstream modelFile;
	modelFile.open( m_FullFileName, std::ifstream::in | std::ifstream::binary );
	kbErrorCheck( modelFile.good(), "kbModel::LoadMS3D() - Failed to load model %s", m_FullFileName.c_str() );
	
	// Find the file size
	modelFile.seekg( 0, std::ifstream::end );
	std::streamoff fileSize = modelFile.tellg();
	modelFile.seekg( 0, std::ifstream::beg );

	// Load file into memory
	char *const pMemoryFileBuffer = new char[fileSize];
	modelFile.read( pMemoryFileBuffer, fileSize );
	modelFile.close();

	const char * pPtr = pMemoryFileBuffer;

	// Header
	const ms3dHeader_t *const pHeader = (const ms3dHeader_t *) pPtr;
	pPtr += sizeof( ms3dHeader_t );

	kbErrorCheck( strncmp( pHeader->m_ID, "MS3D000000", 10 ) == 0, "kbModel::LoadResource_Internal - Invalid model header %d", pHeader->m_ID );

	// Vertices
	m_Bounds.Reset();

	ushort numVertices = *(ushort *) pPtr;
	pPtr += sizeof( ushort );

	kbVec3 * tempVertices = new kbVec3[ numVertices ];
	int * boneIndices = new int[numVertices];

	for ( uint i = 0; i < numVertices; i++ ) {
		const ms3dVertex_t * pVertices = ( ms3dVertex_t * ) pPtr;
		pPtr += sizeof( ms3dVertex_t );

		tempVertices[i].x = pVertices->m_vertex[0];
		tempVertices[i].y = pVertices->m_vertex[1];
		tempVertices[i].z = pVertices->m_vertex[2] * -1;	// flip from rhs to lhs

		boneIndices[i] = pVertices->m_boneID;

		m_Bounds.AddPoint( tempVertices[i] );
	}

	// Triangles
	const uint numTriangles = *(ushort *) pPtr;
	pPtr += sizeof( ushort );
	ms3dTriangle_t * tempTriangles = new ms3dTriangle_t[numTriangles];

	for ( uint i = 0; i < numTriangles; i++ ) {
		const ms3dTriangle_t * pTriangles = (ms3dTriangle_t *) pPtr;
		pPtr += sizeof( ms3dTriangle_t );
		tempTriangles[i] = *pTriangles;
	
	/*	tempTriangles[i].m_VertexIndices[0] = pTriangles->m_VertexIndices[2];
		tempTriangles[i].m_VertexIndices[1] = pTriangles->m_VertexIndices[1];
		tempTriangles[i].m_VertexIndices[2] = pTriangles->m_VertexIndices[0];

		tempTriangles[i].u[0] = pTriangles->u[2];
		tempTriangles[i].u[1] = pTriangles->u[1];
		tempTriangles[i].u[2] = pTriangles->u[0];

		tempTriangles[i].v[0] = pTriangles->v[2];
		tempTriangles[i].v[1] = pTriangles->v[1];
		tempTriangles[i].v[2] = pTriangles->v[0];*/

		// Flip z components of normals from rhs to lhs
		tempTriangles[i].m_VertexNormals[0][2] *= -1;
		tempTriangles[i].m_VertexNormals[1][2] *= -1;
		tempTriangles[i].m_VertexNormals[2][2] *= -1;
	}

	// Groups ------------------------------------------------//
	uint numSrcGroups = *(ushort *) pPtr;
	pPtr += sizeof( ushort );

	// Look for groups named "collision" and build collision geometry from them
	int collisionGroupIdx = -1;
	const char * pCollisionPtr = pPtr;
	for ( uint iGroup = 0; iGroup < numSrcGroups; iGroup++ ) {

		pCollisionPtr += sizeof( byte );
		const char * groupName = (char *)( pCollisionPtr );

		pCollisionPtr += 32;
		const uint numTris = *(ushort *)( pCollisionPtr );

		pCollisionPtr += sizeof( ushort );

		if ( strcmp( groupName, "collision" ) == 0 ) {
			kbErrorCheck( collisionGroupIdx == -1, "Too many collision groups in %s", m_FullFileName.c_str() );
			collisionGroupIdx = iGroup;

			// Use collision geometry to determine the kbModel's bounds
			m_Bounds.Reset();
			for ( uint iTris = 0; iTris < numTris; iTris++ ) {
				unsigned short curTriIndex = *(ushort *) pCollisionPtr;

				ms3dTriangle_t & curTri  = tempTriangles[ curTriIndex ];

				m_Bounds.AddPoint( tempVertices[ curTri.m_VertexIndices[ 0 ] ] );
				m_Bounds.AddPoint( tempVertices[ curTri.m_VertexIndices[ 1 ] ] );
				m_Bounds.AddPoint( tempVertices[ curTri.m_VertexIndices[ 2 ] ] );

				pCollisionPtr += sizeof( ushort );
			}
		} else {
			pCollisionPtr += sizeof( ushort ) * numTris;
		}

		pCollisionPtr += sizeof( char );
	}

	if ( collisionGroupIdx == -1 ) {
		m_Meshes.resize( numSrcGroups );
	} else {
		// Note: collision groups are not copied over to the kbModel
		m_Meshes.resize( numSrcGroups - 1 );
	}

	int ibIndex = 0;
	for ( uint iSrcGroupIdx = 0, iDestGroupIdx = 0; iSrcGroupIdx < numSrcGroups; iSrcGroupIdx++ ) {

		if ( iSrcGroupIdx == collisionGroupIdx ) {
			
			// Skip collision group
			pPtr += sizeof( byte ) + 32;
			const ushort NumTris = *(ushort *)pPtr;
			pPtr += sizeof( ushort ) + NumTris * sizeof( ushort ) + sizeof( char );

			continue;
		}

		mesh_t & currentMesh = m_Meshes[iDestGroupIdx];
		iDestGroupIdx++;

		pPtr += sizeof( byte );			// Skip flags
		pPtr += 32;						// Skip name

		currentMesh.m_NumTriangles = *(ushort *)pPtr;
		pPtr += sizeof( ushort );

		currentMesh.m_TriangleIndices = new ushort[ currentMesh.m_NumTriangles ];
		currentMesh.m_IndexBufferIndex = ibIndex;
		ibIndex += currentMesh.m_NumTriangles * 3;

		for ( uint iTris = 0; iTris < currentMesh.m_NumTriangles; iTris++ ) {
			currentMesh.m_TriangleIndices[iTris] = *(ushort *) pPtr;
			pPtr += sizeof( ushort );
		}

		currentMesh.m_MaterialIndex = *(byte *)pPtr;

		pPtr += sizeof( char );
	}

	const uint numMaterials = *(ushort *) pPtr;
	m_Materials.resize( numMaterials );
	pPtr += sizeof( ushort );

	std::string filePath = m_FullFileName;
	size_t stringPos = filePath.rfind( "\\" );
	if ( stringPos != std::string::npos ) {
		filePath.erase( stringPos );
		filePath.append( "/" );
	}

	// todo:don't load duplicate textures
	for ( uint iMat = 0; iMat < numMaterials; iMat++ ) {
		ms3dMaterial_t * pMat = (ms3dMaterial_t *) pPtr;
		pPtr += sizeof(ms3dMaterial_t);

		m_Materials[iMat].m_DiffuseColor.Set( pMat->m_Diffuse[0], pMat->m_Diffuse[1], pMat->m_Diffuse[2], 1.0f );
	}

	// create index buffer
	const uint numFinalTriangles = ibIndex / 3;

	D3D11_BUFFER_DESC indexBufferDesc = { 0 };
	indexBufferDesc.ByteWidth = sizeof( unsigned long ) * ibIndex;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	m_CPUIndices.resize( ibIndex );

	ibIndex = 0;

	std::unordered_map<vertexLayout, int, kbVertexHash> vertHash;

	for ( uint i = 0; i < m_Meshes.size(); i++ ) {

		kbErrorCheck( ibIndex == m_Meshes[i].m_IndexBufferIndex, "kbModel::Load_Internal() - Index buffer mismatch" );

		const kbMaterial & modelMaterial = GetMaterials()[m_Meshes[i].m_MaterialIndex];
		for ( uint iTris = 0; iTris < m_Meshes[i].m_NumTriangles; iTris++ ) {
			
			const int triangleIndex = m_Meshes[i].m_TriangleIndices[iTris];
			ms3dTriangle_t & currentTriangle = tempTriangles[triangleIndex];
			for ( int j = 0; j < 3; j++ ) {
				vertexLayout newVert;

				newVert.position = tempVertices[ currentTriangle.m_VertexIndices[ j ] ];
				newVert.uv.Set( currentTriangle.u[ j ], currentTriangle.v[ j ] );

				kbVec4 normal( currentTriangle.m_VertexNormals[j][0], currentTriangle.m_VertexNormals[j][1], currentTriangle.m_VertexNormals[j][2], 0 );
				newVert.SetNormal( normal );

				// TODO: We probably want vertex colors even if not cpu only
				if ( m_bCPUAccessOnly ) {
					newVert.SetColor( modelMaterial.GetDiffuseColor() );
				} else {
					newVert.color[0] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[1] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[2] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[3] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]]; 
				}

				auto it = vertHash.find( newVert );

				int vertIndex;
				if ( it == vertHash.end() ) {
					vertIndex = (int)m_CPUVertices.size();
					vertHash[newVert] = vertIndex;
					m_CPUVertices.push_back( newVert );
					m_Meshes[i].m_Bounds.AddPoint( newVert.position );

				} else {
					vertIndex = it->second;
				}

				m_CPUIndices[ibIndex + (2-j)] = vertIndex;

				// todo - Maybe should be editor only?
				m_Meshes[i].m_Vertices.push_back( newVert.position );
			}

			ibIndex += 3;
		}
	}

	if ( m_bCPUAccessOnly == false ) {
		m_IndexBuffer.CreateIndexBuffer( m_CPUIndices );

		std::vector<vertexLayout> verts( m_CPUVertices.size() );

		for ( uint i = 0; i < m_CPUVertices.size(); i++ ) {
			verts[i].position = m_CPUVertices[i].position;
			verts[i].uv = m_CPUVertices[i].uv;

			verts[i].normal[0] = m_CPUVertices[i].normal[0];
			verts[i].normal[1] = m_CPUVertices[i].normal[1];
			verts[i].normal[2] = m_CPUVertices[i].normal[2];
			verts[i].normal[3] = m_CPUVertices[i].normal[3];

			verts[i].tangent[0] = m_CPUVertices[i].tangent[0];
			verts[i].tangent[1] = m_CPUVertices[i].tangent[1];
			verts[i].tangent[2] = m_CPUVertices[i].tangent[2];
			verts[i].tangent[3] = m_CPUVertices[i].tangent[3];

			verts[i].color[0] = m_CPUVertices[i].color[0];
			verts[i].color[1] = m_CPUVertices[i].color[1];
			verts[i].color[2] = m_CPUVertices[i].color[2];
			verts[i].color[3] = m_CPUVertices[i].color[3];
			// color, normal, etc
		}

		m_VertexBuffer.CreateVertexBuffer( verts );
	}
   
	// Joints
	const float AnimationFPS = *( float * ) pPtr;
	pPtr += sizeof( float );

	const float CurrentTime = *( float * ) pPtr;
	pPtr += sizeof( float );

	const int TotalFrames = *( int * ) pPtr;
	pPtr += sizeof( int );

	const ushort numJoints = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );

	m_Bones.resize( numJoints );

	for ( unsigned i = 0; i < m_Bones.size(); i++ ) {
		const ms3dBone_t *const pJoint = (ms3dBone_t *)  pPtr;
		pPtr += sizeof( ms3dBone_t );

		m_Bones[i].m_Name = pJoint->m_Name;
		m_Bones[i].m_ParentIndex = -1;

		// Find this bone's parent index
		for ( uint parentIdx = 0; parentIdx < i; parentIdx++ ) {
			if ( m_Bones[parentIdx].m_Name == pJoint->m_ParentName ) {
				m_Bones[i].m_ParentIndex = parentIdx;
				break;
			}
		}

		kbWarningCheck( i == 0 || m_Bones[i].m_ParentIndex != 65535, "kbModel::LoadMS3D() - Missing parent in model %s at index %d", GetName().c_str(), i );

		m_Bones[i].m_RelativePosition.Set( pJoint->m_Position[0], pJoint->m_Position[1], -pJoint->m_Position[2] );

		// Convert rotation from euler angles to quaternions
		kbQuat rotationX( kbVec3::right, pJoint->m_Rotation[0] ); 
		kbQuat rotationY( kbVec3::up, pJoint->m_Rotation[1] );
		kbQuat rotationZ( kbVec3::forward, -pJoint->m_Rotation[2] );
		m_Bones[i].m_RelativeRotation = rotationX * rotationY * rotationZ;

		// Skip any animations
		pPtr += sizeof( ms3dRotationKeyFrame_t ) * pJoint->m_NumPositionKeyFrames;
		pPtr += sizeof( ms3dPositionKeyFrame_t ) * pJoint->m_NumRotationKeyFrames;
	}

	delete[] tempVertices;
	delete[] boneIndices;
	delete[] tempTriangles;
	delete[] pMemoryFileBuffer;

	// Build ref pose
	m_RefPose.insert( m_RefPose.begin(), m_Bones.size(), kbBoneMatrix_t() );
	m_InvRefPose.insert( m_InvRefPose.begin(), m_Bones.size(), kbBoneMatrix_t() );

	for ( int i = 0; i < m_Bones.size(); i++ ) {

		const int parent = m_Bones[i].m_ParentIndex;			
		const kbMat4 rotationmat = m_Bones[i].m_RelativeRotation.ToMat4();
		kbBoneMatrix_t parentMat;
		if ( parent != 65535 ) {
			parentMat = m_RefPose[parent];
		} else { 
			parentMat.SetIdentity();
		}
		m_RefPose[i].SetAxis( 0, rotationmat[0].ToVec3() );
		m_RefPose[i].SetAxis( 1, rotationmat[1].ToVec3() );
		m_RefPose[i].SetAxis( 2, rotationmat[2].ToVec3() );
		m_RefPose[i].SetAxis( 3,  m_Bones[i].m_RelativePosition  );
		m_RefPose[i] = m_RefPose[i] * parentMat;

		m_InvRefPose[i] = m_RefPose[i];
		m_InvRefPose[i].Invert();
	}

	return true;
}

/**
 *	kbModel::LoadFBX
 */
FbxManager * g_pFBXSDKManager = nullptr;

FbxAMatrix GetGeometryTransformation( FbxNode const* inNode ) {

	kbErrorCheck( inNode != nullptr, "GetGeometryTransformation() - null mesh" );

	const FbxVector4 lT = inNode->GetGeometricTranslation( FbxNode::eSourcePivot );
	const FbxVector4 lR = inNode->GetGeometricRotation( FbxNode::eSourcePivot );
	const FbxVector4 lS = inNode->GetGeometricScaling( FbxNode::eSourcePivot );

	return FbxAMatrix(lT, lR, lS);
}

bool kbModel::LoadFBX() {

	struct FBXData {
		FbxImporter * pImporter = nullptr;
		FbxScene * pScene = nullptr;

		~FBXData() {
			if ( pImporter != nullptr ) {
				pImporter->Destroy();
			}
			if ( pScene != nullptr ) {
				pScene->Destroy();
			}
		}
	} fbxData;

	if ( g_pFBXSDKManager == nullptr ) {
		g_pFBXSDKManager = FbxManager::Create();

		FbxIOSettings *const pIOsettings = FbxIOSettings::Create( g_pFBXSDKManager, IOSROOT );
		g_pFBXSDKManager->SetIOSettings( pIOsettings );
	}

	fbxData.pImporter = FbxImporter::Create( g_pFBXSDKManager, "" );

	bool bSuccess = fbxData.pImporter->Initialize( GetFullFileName().c_str(), -1, g_pFBXSDKManager->GetIOSettings() );
	if( bSuccess == false ) {
		return false;
	}

	fbxData.pScene = FbxScene::Create( g_pFBXSDKManager,"" );
	bSuccess = fbxData.pImporter->Import( fbxData.pScene );
	if( bSuccess == false ) {
		return false;
	}

	FbxNode * pRootNode = fbxData.pScene->GetRootNode();
	kbErrorCheck( pRootNode != nullptr, "kbModel::LoadFBX() - Root node not found in %s", GetFullFileName().c_str() );

	std::unordered_map<vertexLayout, int, kbVertexHash> vertexMap;
	std::vector<vertexLayout> vertexList;
	std::vector<ushort> indexList;

	std::map<int, kbBounds> boneToBounds;
	std::map<int, int> vertToBone;
	std::map<int, kbColor> boneToColor;


	for ( int iMesh = 0; iMesh < pRootNode->GetChildCount(); iMesh++ ) {
		
		FbxMesh *const pFBXMesh = pRootNode->GetChild(iMesh)->GetMesh();
		if ( pFBXMesh == nullptr ) {
			continue;
		}

		m_Meshes.push_back( mesh_t() );
		mesh_t & newMesh = m_Meshes[m_Meshes.size() - 1];
		newMesh.m_IndexBufferIndex = (unsigned int)indexList.size();
		newMesh.m_MaterialIndex = 0;
		newMesh.m_NumTriangles = pFBXMesh->GetPolygonCount();

		newMesh.m_Bounds.Reset();

		uint vertexCount = 0;


		int numDeformers = pFBXMesh->GetDeformerCount();
		FbxAMatrix geomXForm = GetGeometryTransformation(pRootNode->GetChild(iMesh));
		for ( int iDeform = 0; iDeform < numDeformers; iDeform++ ) {
			FbxSkin * pCurSkin = (FbxSkin*)pFBXMesh->GetDeformer( iDeform, FbxDeformer::eSkin );
			if ( pCurSkin == nullptr ) {
				continue;
			}


			uint numClusters = pCurSkin->GetClusterCount();
			for ( uint iCluster = 0; iCluster < numClusters; iCluster++ ) {
				FbxCluster * pCurCluster = pCurSkin->GetCluster( iCluster );
				std::string curJointName = pCurCluster->GetLink()->GetName();

				boneToBounds[iCluster].Reset();
				kbColor boneColor( kbfrand() * 0.5f + 0.5f, kbfrand() * 0.5f + 0.5f, kbfrand() * 0.5f + 0.5f, 1.0f );
				boneToColor[iCluster] = boneColor;

			//	kbLog( "%s bone color is %f %f %f", curJointName.c_str(), boneColor.x, boneColor.y, boneColor.z, boneColor.w );

				FbxAMatrix xformMat;
				FbxAMatrix xformLinkMat;
				FbxAMatrix globalBindPoseInverseMatrix;

				pCurCluster->GetTransformMatrix( xformMat );
				pCurCluster->GetTransformLinkMatrix( xformLinkMat );
				globalBindPoseInverseMatrix = xformLinkMat.Inverse() * xformMat * geomXForm;
				//kbLog( "Yay!");

				unsigned int numOfIndices = pCurCluster->GetControlPointIndicesCount();
				int * pCtrlPtList = pCurCluster->GetControlPointIndices();
				for (unsigned int i = 0; i < numOfIndices; ++i)
				{
				//	kbLog( "	Adding vertex %d", pCtrlPtList[i]);
					vertToBone[pCtrlPtList[i]] = iCluster;
				}

			}
		}

		for ( int iTri = 0; iTri < (int)newMesh.m_NumTriangles; iTri++ ) {

			int iCurVertex = vertexCount + 2;
			for ( int iTriVert = 2; iTriVert >= 0; iTriVert--, vertexCount++, iCurVertex-- ) {
				vertexLayout triVert;
				memset( &triVert, 0, sizeof( triVert ) );

				const int iCtrlPt = pFBXMesh->GetPolygonVertex( iTri, iTriVert );
				const FbxVector4 ctrlPt = pFBXMesh->GetControlPointAt(iCtrlPt);

				triVert.position.Set( (float)ctrlPt[1], (float)ctrlPt[2], -(float)ctrlPt[0] );
				newMesh.m_Bounds.AddPoint( triVert.position );
		
				FbxGeometryElementNormal *const pFBXVertNormal = pFBXMesh->GetElementNormal(0);
				if ( pFBXVertNormal != nullptr ) {
					auto mappingMode = pFBXVertNormal->GetMappingMode();
					kbErrorCheck( mappingMode == FbxGeometryElement::eByPolygonVertex, "kbModel::LoadFBX() - Invalid vertex normal mapping mode" );

					auto refMode = pFBXVertNormal->GetReferenceMode();
					kbErrorCheck( refMode == FbxGeometryElement::eDirect, "kbModel::LoadFBX() - Invalid vertex normal reference mode" );

					const auto fbxNormal = pFBXVertNormal->GetDirectArray().GetAt(iCurVertex).mData;
					kbVec4 normal( (float)fbxNormal[1], (float)fbxNormal[2], -(float)fbxNormal[0], 0.0f );
					normal.w = 0;
					triVert.SetNormal( normal );
				}

				FbxGeometryElementTangent *const pFBXVertTangent = pFBXMesh->GetElementTangent(0);
				if ( pFBXVertTangent != nullptr ) {

					auto mappingMode = pFBXVertTangent->GetMappingMode();
					kbErrorCheck( mappingMode == FbxGeometryElement::eByPolygonVertex, "kbModel::LoadFBX() - Invalid vertex tangent mapping mode" );

					auto refMode = pFBXVertTangent->GetReferenceMode();
					kbErrorCheck( refMode == FbxGeometryElement::eDirect, "kbModel::LoadFBX() - Invalid vertex tangent reference mode" );

					const auto fbxTangent = pFBXVertTangent->GetDirectArray().GetAt(iCurVertex).mData;
					kbVec4 tangent( (float)fbxTangent[1], (float)fbxTangent[2], -(float)fbxTangent[0], 0.0f );
					triVert.SetTangent( tangent );
				}

				FbxGeometryElementBinormal *const pFBXVertBinormal = pFBXMesh->GetElementBinormal(0);
				if ( pFBXVertBinormal != nullptr ) {

					auto mappingMode = pFBXVertBinormal->GetMappingMode();
					kbErrorCheck( mappingMode == FbxGeometryElement::eByPolygonVertex, "kbModel::LoadFBX() - Invalid vertex binormal mapping mode" );

					auto refMode = pFBXVertBinormal->GetReferenceMode();
					kbErrorCheck( refMode == FbxGeometryElement::eDirect, "kbModel::LoadFBX() - Invalid vertex binormal reference mode" );

					const auto fbxBinormal = pFBXVertBinormal->GetDirectArray().GetAt(iCurVertex).mData;
					kbVec4 binormal( (float)fbxBinormal[1], (float)fbxBinormal[2], -(float)fbxBinormal[0], 0.0f );
					triVert.SetBitangent( binormal );
				}

				FbxGeometryElementUV *const pFBXVertUV = pFBXMesh->GetElementUV(0);
				if ( pFBXVertUV != nullptr ) {

					auto uvMapMode = pFBXVertUV->GetMappingMode();
					kbErrorCheck( uvMapMode == FbxGeometryElement::eByPolygonVertex, "kbModel::LoadFBX() - Invalid uvs mapping mode" );

					auto uvRefMode = pFBXVertUV->GetReferenceMode();
					kbErrorCheck( uvRefMode == FbxGeometryElement::eIndexToDirect, "kbModel::LoadFBX() - Invalid uvs reference mode" );

					const int uvIndex = pFBXVertUV->GetIndexArray().GetAt(iCurVertex);
					const auto fbxUV = pFBXVertUV->GetDirectArray().GetAt(uvIndex).mData;
					triVert.uv.Set((float)fbxUV[0], 1.0f - (float)fbxUV[1]);
				}

				FbxGeometryElementVertexColor *const pFBXVertColor = pFBXMesh->GetElementVertexColor(0);
				if (pFBXVertColor != nullptr) {

					auto mappingMode = pFBXVertColor->GetMappingMode();
					kbErrorCheck(mappingMode == FbxGeometryElement::eByPolygonVertex, "kbModel::LoadFBX() - Invalid vertex color mapping mode");

					auto refMode = pFBXVertColor->GetReferenceMode();
					kbErrorCheck(refMode == FbxGeometryElement::eIndexToDirect, "kbModel::LoadFBX() - Invalid vertex color reference mode");

					const int colorIndex = pFBXVertColor->GetIndexArray().GetAt(iCurVertex);
					const auto fbxColor = pFBXVertColor->GetDirectArray().GetAt(colorIndex);
					kbVec4 color( (float)fbxColor.mRed, (float)fbxColor.mGreen, (float)fbxColor.mBlue, (float)fbxColor.mAlpha );
					triVert.SetColor(color);
				}

				int boneIdx = vertToBone[iCtrlPt];
				boneToBounds[boneIdx].AddPoint( triVert.position );
				triVert.color[0] = (byte) boneIdx;
				triVert.color[1] = (byte) boneIdx;
				triVert.color[2] = (byte) boneIdx;
				triVert.color[3] = (byte) boneIdx;

				/*
									newVert.color[0] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[1] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[2] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]];
					newVert.color[3] = (byte)boneIndices[currentTriangle.m_VertexIndices[j]]; 
				*/
				//triVert.SetColor( boneToColor[boneIdx] );
				auto vertIt = vertexMap.find( triVert );
				if ( vertIt == vertexMap.end() ) {
					const int vertIdx = (int)vertexList.size();
					vertexMap.insert( std::pair<vertexLayout, int>( triVert, vertIdx ) );
					vertexList.push_back( triVert );
					indexList.push_back( vertIdx );
				} else {
					indexList.push_back( vertIt->second );
				}
			}
		}
	}
/*
	for ( int i = 0; i < pRootNode->GetChildCount(); i++ ) {
		FbxNode * pCurNode = pRootNode->GetChild(i);
		kbLog( "Processing parent node %s", pCurNode->GetName() );

		for ( int j = 0; j < pCurNode->GetChildCount(); j++ ) {
			FbxNode * pRootBone = pCurNode->GetChild(j);
			if ( pRootBone->GetNodeAttribute() == nullptr || pRootBone->GetNodeAttribute()->GetAttributeType() != FbxNodeAttribute::eSkeleton ) {
				continue;
			}
			kbLog( "	Processing Root bone %s", pRootBone->GetName() );
					
			for ( int l = 0; l < pRootBone->GetChildCount(); l++ ) {
				FbxNode * pBoneNode = pRootBone->GetChild(l);
				if ( pBoneNode->GetNodeAttribute() == nullptr || pBoneNode->GetNodeAttribute()->GetAttributeType() != FbxNodeAttribute::eSkeleton ) {
					continue;
				}
				kbLog( "		Processing child bones %s", pBoneNode->GetName() );
			}
		}
	}*/

	m_VertexBuffer.CreateVertexBuffer( vertexList );
	m_IndexBuffer.CreateIndexBuffer( indexList );

	kbMaterial newMaterial;
	newMaterial.m_pShader = nullptr;//(kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	m_Materials.push_back( newMaterial );

	m_Bones.resize( boneToBounds.size() );
	for ( int i = 0; i < boneToBounds.size(); i++ ) {
		kbBounds & boneBounds = boneToBounds[i];
		m_Bones[i].m_RelativePosition = boneBounds.Center();
		m_Bones[i].m_RelativeRotation = kbQuat( 0.0f, 0.0f, 0.0f, 1.0f );

		kbBoneMatrix_t invRef;
		invRef.SetIdentity();
		invRef.SetAxis( 3, -m_Bones[i].m_RelativePosition );
		m_InvRefPose.push_back( invRef );

		kbBoneMatrix_t ref;
		ref.SetIdentity();
		ref.SetAxis( 3, m_Bones[i].m_RelativePosition );
		m_RefPose.push_back( ref );
	}
	return true;
}

/**
 *	kbModel::LoadDiablo3
 */
bool kbModel::LoadDiablo3() {

	struct FileReader {
		FileReader() { }
		const std::string delimiters = "\n,";

		int GetInt() {
			std::string::size_type endPos = m_ModelText.find_first_of( delimiters, m_CurPos );
			int retInt = 0;
			if ( endPos != std::string::npos ) {
				const std::string intstr = m_ModelText.substr( m_CurPos, endPos - m_CurPos );
				retInt = std::atoi( intstr.c_str() );
				m_CurPos = endPos + 1;
			} else {
				m_CurPos = m_ModelText.size();
			}

			return retInt;
		}

		float GetFloat() {
			std::string::size_type endPos = m_ModelText.find_first_of( delimiters, m_CurPos );
			float retFloat = 0;
			if ( endPos != std::string::npos ) {
				const std::string floatstr  = m_ModelText.substr( m_CurPos, endPos - m_CurPos );
				retFloat = (float)std::atof( floatstr.c_str() );
				m_CurPos = endPos + 1;
			} else {
				m_CurPos = m_ModelText.size();
			}

			return retFloat;
		}

		kbVec2 GetVec2() {
			return kbVec2( GetFloat(), GetFloat() );
		}

		kbVec3 GetVec3() {
			return kbVec3( GetFloat(), GetFloat(), GetFloat() );
		}

		kbVec4 GetVec4() {
			return kbVec4( GetFloat(), GetFloat(), GetFloat(), GetFloat() );
		}

		std::string			m_ModelText;
		size_t				m_CurPos = 0;

	} fileReader;


	std::ifstream modelFile;
	modelFile.open( m_FullFileName, std::ifstream::in );
	kbErrorCheck( modelFile.good(), "kbModel::LoadDiablo3() - Failed to load model %s", m_FullFileName.c_str() );
	fileReader.m_ModelText = std::string( ( std::istreambuf_iterator<char>(modelFile) ), std::istreambuf_iterator<char>() );

	std::vector<vertexLayout> vertexList;
	std::vector<ushort> indexList;

	while( fileReader.m_CurPos < fileReader.m_ModelText.size() ) {
		// Vertex,Index,POSITION 0,POSITION 1,POSITION 2,NORMAL 0,NORMAL 1,NORMAL 2,NORMAL 3,COLOR0 0,COLOR0 1,COLOR0 2,COLOR0 3,COLOR1 0,COLOR1 1,COLOR1 2,COLOR1 3,TEXCOORD0 0,TEXCOORD0 1,TEXCOORD0 2,TEXCOORD0 3,TEXCOORD1 0,TEXCOORD1 1,TEXCOORD1 2,TEXCOORD1 3,BLENDINDICES 0,BLENDINDICES 1,BLENDINDICES 2,BLENDINDICES 3,BLENDWEIGHT 0,BLENDWEIGHT 1,BLENDWEIGHT 2

		const int vertNum = fileReader.GetInt();
		const int vertIdx = fileReader.GetInt();
		const kbVec3 vertPos = fileReader.GetVec3();
		const kbVec4 vertNormal = fileReader.GetVec4();
		const kbVec4 vertColor1 = fileReader.GetVec4();
		const kbVec4 vertColor2 = fileReader.GetVec4();
		const kbVec4 vertUV1 = fileReader.GetVec4();
		const kbVec4 vertUV2 = fileReader.GetVec4();

		// Blend Indices
		fileReader.GetInt();
		fileReader.GetInt();
		fileReader.GetInt();
		fileReader.GetInt();

		// Blend Weights
		fileReader.GetVec3();

		vertexLayout newVert;
		newVert.position.Set( vertPos.y, vertPos.x, vertPos.z );
		newVert.normal[0] = (byte)vertNormal.x;
		newVert.normal[1] = (byte)vertNormal.y;
		newVert.normal[2] = (byte)vertNormal.z;
		newVert.normal[3] = (byte)vertNormal.w;

		if ( vertUV1.z == 128 ) {
			newVert.uv.x = vertUV1.w / 512.0f;
		} else {
			newVert.uv.x = 0.5f + vertUV1.w / 512.0f;
		}

		if ( vertUV1.x == 128 ) {
			newVert.uv.y = vertUV1.y / 512.0f;
		} else {
			newVert.uv.y = 0.5f + vertUV1.y / 512.0f;
		}

		vertexList.push_back( newVert );
		indexList.push_back( vertNum );
		//kbLog( "%d, %d, (%f %f %f), (%f %f %f %f), (%f %f)", vertNum, vertIdx, vertPos.x, vertPos.y, vertPos.z, vertNormal.x, vertNormal.y, vertNormal.z, vertNormal.w, vertUV1.x, vertUV1.y );
	}

	m_VertexBuffer.CreateVertexBuffer( vertexList );
	m_IndexBuffer.CreateIndexBuffer( indexList );

	m_Meshes.push_back( mesh_t() );
	mesh_t & newMesh = m_Meshes[m_Meshes.size() - 1];
	newMesh.m_IndexBufferIndex = 0;
	newMesh.m_MaterialIndex = 0;
	newMesh.m_NumTriangles = (uint)indexList.size() / 3;

	kbMaterial newMaterial;
	newMaterial.m_pShader = nullptr;//(kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	m_Materials.push_back( newMaterial );

	return true;
}

/**
 *	kbModel::CreateDynamicModel
 */
void kbModel::CreateDynamicModel( const UINT numVertices, const UINT numIndices, kbShader *const pShaderToUse, kbTexture *const pTextureToUse, const UINT vertexSizeInBytes ) {

	if ( m_NumVertices > 0 || m_Meshes.size() > 0 || m_Materials.size() > 0 || m_VertexBuffer.GetBufferPtr() != nullptr || m_IndexBuffer.GetBufferPtr() != nullptr ) {
		Release_Internal();
	}

	m_NumVertices = numVertices;
	m_bIsDynamicModel = true;
    m_bIsPointCloud = false;
	m_NumTriangles = numIndices / 3;
	m_Stride = vertexSizeInBytes;

	m_VertexBuffer.CreateVertexBuffer( numVertices, vertexSizeInBytes );
	m_IndexBuffer.CreateIndexBuffer( numIndices );

	mesh_t newMesh;
	newMesh.m_NumTriangles = m_NumTriangles;
	newMesh.m_IndexBufferIndex = 0;
	newMesh.m_MaterialIndex = 0;
	m_Meshes.push_back( newMesh );

	kbMaterial newMaterial;
	if ( pShaderToUse != nullptr ) {
		newMaterial.m_pShader = pShaderToUse;
	} else {
		newMaterial.m_pShader = nullptr;//(kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	}
	m_Materials.push_back( newMaterial );
}

/**
 *	kbModel::CreatePointCloud
 */
void kbModel::CreatePointCloud( const UINT numVertices, const std::string & shaderToUse, const ECullMode cullingMode,  const UINT vertexSizeInBytes ) {
	if ( m_NumVertices > 0 || m_Meshes.size() > 0 || m_Materials.size() > 0 || m_VertexBuffer.GetBufferPtr() != nullptr || m_IndexBuffer.GetBufferPtr() != nullptr ) {
		Release_Internal();
	}

	m_NumVertices = numVertices;
	m_bIsDynamicModel = false;
    m_bIsPointCloud = true;
	m_NumTriangles = 0;
	m_Stride = vertexSizeInBytes;

	m_VertexBuffer.CreateVertexBuffer( numVertices, m_Stride );

	mesh_t newMesh;
	newMesh.m_NumTriangles = m_NumTriangles;
	newMesh.m_IndexBufferIndex = 0;
	newMesh.m_MaterialIndex = 0;
	m_Meshes.push_back( newMesh );

	kbMaterial newMaterial;
	if ( shaderToUse.length() > 0 ) {
		newMaterial.m_pShader = nullptr;//(kbShader *) g_ResourceManager.GetResource( shaderToUse.c_str(), true );
	} else {
		newMaterial.m_pShader = nullptr;//(kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbshader", true );
	}
	newMaterial.SetCullingMode( cullingMode );
	m_Materials.push_back( newMaterial );
}

/**
 *	kbModel::MapVertexBuffer
 */
void * kbModel::MapVertexBuffer() {

	kbErrorCheck( m_bVBIsMapped == false, "kbModel::MapVertexBuffer() - Vertex buffer already mapped" );

	m_bVBIsMapped = true;
	return m_VertexBuffer.Map();
}

/**
 *	kbModel::UnmapVertexBuffer
 */
void kbModel::UnmapVertexBuffer( const INT NumIndices ) {

   if ( m_bVBIsMapped == false ) {
      return;
   }

	m_bVBIsMapped = false;
	if ( m_bIsDynamicModel && NumIndices > -1 ) {
		const INT NumTriangles = NumIndices / 3;
		if ( NumTriangles > m_NumTriangles ) {
			kbError( "kbModel Overflow" );
		}

		m_Meshes[0].m_NumTriangles = NumTriangles;
	}

	m_VertexBuffer.Unmap();
}

/**
 *	kbModel::MapIndexBuffer
 */
void * kbModel::MapIndexBuffer() {
    kbErrorCheck( m_bIBIsMapped == false, "kbModel::MapIndexBuffer() - Index buffer is already mapped." );
    kbErrorCheck( m_bIsDynamicModel == true, "kbModel::MapIndexBuffer() - Not a dynamic model." );
    kbErrorCheck( m_bIsPointCloud == false, "kbModel::MapIndexBuffer() - Point clouds cannot be mapped." );

	m_bIBIsMapped = true;
	return m_IndexBuffer.Map();
}

/**
 *	kbModel::UnmapIndexBuffer
 */
void kbModel::UnmapIndexBuffer() {
    kbErrorCheck( m_bIBIsMapped == true, "kbModel::UnmapIndexBuffer() - Index buffer was not mapped." );
    kbErrorCheck( m_bIsDynamicModel == true, "kbModel::UnmapIndexBuffer() - Not a dynamic model." );
    kbErrorCheck( m_bIsPointCloud == false, "kbModel::UnmapIndexBuffer() - Point clouds cannot be mapped." );


	m_bIBIsMapped = false;
	m_IndexBuffer.Unmap();
}

/**
 *	kbMode::SwapTexture
 */
void kbModel::SwapTexture( const UINT meshIdx, const kbTexture * pTexture, const int textureIdx ) {
	
	if ( meshIdx < 0 || meshIdx >= m_Materials.size() ) {
		return;
	}

	kbMaterial & material = m_Materials[meshIdx];
	if ( textureIdx < 0 || textureIdx >= material.m_Textures.size() + 1 ) {
		return;
	}

	if ( textureIdx < material.m_Textures.size() ) {
		material.m_Textures[textureIdx] = pTexture;
	} else {
		material.m_Textures.push_back( pTexture );
	}
}

/**
 *	kbModel::RayIntersection
 */
kbModelIntersection_t kbModel::RayIntersection( const kbVec3 & inRayOrigin, const kbVec3 & inRayDirection, const kbVec3 & modelTranslation, const kbQuat & modelOrientation ) const {
	kbModelIntersection_t intersectionInfo;

	kbMat4 inverseModelRotation = modelOrientation.ToMat4();
	inverseModelRotation.TransposeSelf();
	const kbVec3 rayStart = ( inRayOrigin - modelTranslation ) * inverseModelRotation;
	const kbVec3 rayDir = inRayDirection.Normalized() * inverseModelRotation;
	float t = FLT_MAX;

	for ( int iMesh = 0; iMesh < m_Meshes.size(); iMesh++ ) {
		for ( int iVert = 0; iVert < m_Meshes[iMesh].m_Vertices.size(); iVert += 3 ) {
			
			const kbVec3 & v0 = m_Meshes[iMesh].m_Vertices[iVert+0];
			const kbVec3 & v1 = m_Meshes[iMesh].m_Vertices[iVert+1];
			const kbVec3 & v2 = m_Meshes[iMesh].m_Vertices[iVert+2];

			if ( kbRayTriIntersection( t, rayStart, rayDir, v0, v1, v2 ) ) {
				if ( t < intersectionInfo.t && t >= 0 ) {
					intersectionInfo.t = t;
					intersectionInfo.meshNum = iMesh;
				}
			}
		}
	}

	if ( intersectionInfo.meshNum >= 0 ) {
		intersectionInfo.hasIntersection = true;
		intersectionInfo.intersectionPoint = inRayOrigin + intersectionInfo.t * inRayDirection.Normalized(); 
	}

	return intersectionInfo;
}

/**
 *	kbModel::Release_Internal
 */
void kbModel::Release_Internal() {
	m_VertexBuffer.Release();
	m_IndexBuffer.Release();

	m_Materials.clear();

	for ( uint i = 0; i < m_Meshes.size(); i++ ) {
		delete[] m_Meshes[i].m_TriangleIndices;
	}
	m_Meshes.clear();

	m_CPUVertices.clear();
	m_CPUIndices.clear();
	m_Bounds.Reset();
}

/**
 *	kbModel::GetBoneIndex
 */
int	kbModel::GetBoneIndex( const kbString & BoneName ) const {
	for ( int i = 0; i < m_Bones.size(); i++ ) {
		if ( m_Bones[i].m_Name == BoneName ) {
			return i;
		}
	}

	return -1;
}

/**
 *	kbAnimation::SetBoneMatrices
 */
void kbModel::SetBoneMatrices( std::vector<AnimatedBone_t> & bones, const float time, const kbAnimation *const pAnimation, const bool bIsLooping ) {
	if ( m_Bones.size() == 0 ) {
		return;
	}

	if ( pAnimation == nullptr ) {
		return;
	}

	const kbAnimation & animationData = *pAnimation;
	const float maxLength = animationData.m_LengthInSeconds;
	const float animTime = ( bIsLooping && time > maxLength ) ? ( fmod( time, maxLength ) ) : ( time );

	bones.resize( m_Bones.size() );

	for ( int i = 0; i < m_Bones.size(); i++ ) {

		bones[i].m_JointSpacePosition = kbVec3::zero;
		bones[i].m_JointSpaceRotation = kbQuat::identity;

		const kbAnimation::kbBoneKeyFrames_t & jointData = animationData.m_JointKeyFrameData[i];
		for ( int nextKey = 0; nextKey < jointData.m_RotationKeyFrames.size(); nextKey++ ) {

			float nextTime = jointData.m_RotationKeyFrames[nextKey].m_Time;
			if ( animTime >= nextTime && nextKey != jointData.m_RotationKeyFrames.size() - 1 ) {
				continue;
			}

			int prevKey = nextKey;
			if ( animTime >= nextTime ) {
				if ( bIsLooping ) {
					// Looped
					prevKey = 0;
					nextKey = 1;
				} else {
					prevKey = nextKey = (int) jointData.m_RotationKeyFrames.size() - 1;
				}
			} else {
				prevKey = kbClamp( prevKey - 1, 0, (int)jointData.m_RotationKeyFrames.size() );
			}

			const kbVec3 prevBonePosition = jointData.m_TranslationKeyFrames[prevKey].m_Position;
			const kbQuat prevBoneRotation = jointData.m_RotationKeyFrames[prevKey].m_Rotation;

			const kbQuat nextRotation = jointData.m_RotationKeyFrames[nextKey].m_Rotation;
			const kbVec3 nextPosition = jointData.m_TranslationKeyFrames[nextKey].m_Position;

			const float prevTime = jointData.m_RotationKeyFrames[prevKey].m_Time;
			nextTime = jointData.m_RotationKeyFrames[nextKey].m_Time;

			const float timeBetweenKeys = nextTime - prevTime;
			const float timeSincePrevKey = animTime - prevTime;
			const float t = ( timeBetweenKeys > 0 ) ? ( timeSincePrevKey / timeBetweenKeys ) : ( 0.0f );
			bones[i].m_JointSpaceRotation = kbQuat::Slerp( prevBoneRotation, nextRotation, t );
			bones[i].m_JointSpacePosition = prevBonePosition + ( nextPosition - prevBonePosition ) * t;
			break;
		}
	}
}

/**
 *	kbModel::Animate
 */
void kbModel::Animate( std::vector<kbBoneMatrix_t> & outMatrices, const float time, const kbAnimation *const pAnimation, const bool bLoopAnim ) {
	std::vector<AnimatedBone_t> tempBones;
	SetBoneMatrices( tempBones, time, pAnimation, bLoopAnim );

	for ( int i = 0; i < tempBones.size(); i++ ) {

		const int parent = m_Bones[i].m_ParentIndex;

		kbBoneMatrix_t matLocalSkel( m_Bones[i].m_RelativeRotation, m_Bones[i].m_RelativePosition );
		kbBoneMatrix_t matAnimate( tempBones[i].m_JointSpaceRotation, tempBones[i].m_JointSpacePosition );

		kbBoneMatrix_t matLocal = matAnimate * matLocalSkel;
		if ( parent != 65535 ) {
			tempBones[i].m_LocalSpaceMatrix =  matLocal * tempBones[parent].m_LocalSpaceMatrix;
		} else {
			tempBones[i].m_LocalSpaceMatrix = matLocal;
		}

		const kbBoneMatrix_t & invRef = GetInvRefBoneMatrix(i);
		outMatrices[i] = invRef * tempBones[i].m_LocalSpaceMatrix;
	}
}

/**
 *	kbModel::BlendAnimations
 */
void kbModel::BlendAnimations( std::vector<kbBoneMatrix_t> & outMatrices, const kbAnimation *const pFromAnim, const float FromAnimTime, const bool bFromAnimLoops, const kbAnimation *const pToAnim, const float ToAnimTime, const bool bToAnimLoops, const float normalizedBlendTime ) {

	/*std::vector<AnimatedBone_t> fromTempBones;
	SetBoneMatrices( FromAnimTime, pFromAnim, bFromAnimLoops, fromTempBones );

	std::vector<AnimatedBone_t> toTempBones;
	SetBoneMatrices( ToAnimTime, pToAnim, bToAnimLoops, toTempBones );

	for ( int i = 0; i < fromTempBones.size(); i++ ) {

		toTempBones[i].position = kbLerp( fromTempBones[i].position, toTempBones[i].position, normalizedBlendTime );
		toTempBones[i].rotation = kbQuat::Slerp( fromTempBones[i].rotation, toTempBones[i].rotation, normalizedBlendTime );

		const int parent = m_Bones[i].m_ParentIndex;
		if ( parent == 65535 ) {
			toTempBones[i].worldRotation = toTempBones[i].rotation * m_Bones[i].m_RelativeRotation;
		} else {
			toTempBones[i].worldRotation = toTempBones[i].rotation * m_Bones[i].m_RelativeRotation * toTempBones[parent].worldRotation;
		}
		toTempBones[i].worldRotation.Normalize();

		const kbMat4 boneWorldMatrix = toTempBones[i].worldRotation.ToMat4();
		toTempBones[i].worldMatrix.SetAxis( 0, boneWorldMatrix[0].ToVec3() );
		toTempBones[i].worldMatrix.SetAxis( 1, boneWorldMatrix[1].ToVec3() );
		toTempBones[i].worldMatrix.SetAxis( 2, boneWorldMatrix[2].ToVec3() );

		kbVec3 finalBonePosition = m_Bones[i].m_RelativePosition + toTempBones[i].position;
		if ( parent != 65535 ) {
			finalBonePosition = finalBonePosition * toTempBones[parent].worldMatrix;
		}
		toTempBones[i].worldMatrix.SetAxis( 3, finalBonePosition );

		const kbBoneMatrix_t & invRef = GetInvRefBoneMatrix(i);
		boneMatrices[i] = invRef * toTempBones[i].worldMatrix;
	}*/
}

/**
 *	kbAnimation::kbAnimation
 */
kbAnimation::kbAnimation() :
	m_LengthInSeconds( 0 ) {
}

/**
 *	kbAnimation::Load_Internal
 */
bool kbAnimation::Load_Internal() {

	std::ifstream modelFile;
	modelFile.open( m_FullFileName, std::ifstream::in | std::ifstream::binary );

	if ( modelFile.fail() ) {
		int numTries = 5;
		while( numTries < 2 && modelFile.fail() ) {
			modelFile.close();
			Sleep( 2 );
			modelFile.open( m_FullFileName, std::ifstream::in | std::ifstream::binary );
			numTries++;
		}

		if ( modelFile.fail() ) {
			modelFile.close();
			kbWarning( "kbModel::LoadResource_Internal - Failed to load model %s", m_FullFileName.c_str() );
			return false;
		}
	}
	
	// Find the file size
	modelFile.seekg( 0, std::ifstream::end );
	std::streamoff fileSize = modelFile.tellg();
	modelFile.seekg( 0, std::ifstream::beg );

	// Load file into memory
	char *const pMemoryFileBuffer = new char[fileSize];
	modelFile.read( pMemoryFileBuffer, fileSize );
	modelFile.close();

	const char * pPtr = pMemoryFileBuffer;

	// Header
	const ms3dHeader_t * pHeader = ( const ms3dHeader_t * ) pPtr;
	pPtr += sizeof( ms3dHeader_t );

	if ( strncmp( pHeader->m_ID, "MS3D000000", 10 ) != 0 ) {
		kbError( "Error: kbModel::LoadResource_Internal - Invalid model header %s", pHeader->m_ID );
	}

	ushort numVertices = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );

	pPtr += sizeof( ms3dVertex_t ) * numVertices;

	// Triangles
	const uint numTriangles = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );
	pPtr += sizeof( ms3dTriangle_t ) * numTriangles;

	// Groups
	const uint numGroups = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );

	for ( uint i = 0; i < numGroups; i++ ) {

		pPtr += sizeof( byte );	// flags
		pPtr += 32;							// name

		const ushort numTriangles = *(ushort *)pPtr;
		pPtr += sizeof( ushort );

		pPtr += sizeof( ushort ) * numTriangles;
		pPtr += sizeof( char );
	}

	const uint numMaterials = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );
	pPtr += numMaterials * sizeof( ms3dMaterial_t );
   
	// Joints
	const float AnimationFPS = *( float * ) pPtr;
	pPtr += sizeof( float );

	const float CurrentTime = *( float * ) pPtr;
	pPtr += sizeof( float );

	const int TotalFrames = *( int * ) pPtr;
	pPtr += sizeof( int );

	const ushort numJoints = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );

	//m_Bones.resize( numJoints );

	//m_Animations.resize( 1 );
	//kbAnimation & baseAnim = m_Animations[0];
	m_JointKeyFrameData.resize( numJoints );

	kbLog( "Anim %s", m_FullFileName.c_str() );

	for ( unsigned i = 0; i < numJoints; i++ ) {
		const ms3dBone_t * pJoint = ( ms3dBone_t * ) pPtr;
		pPtr += sizeof( ms3dBone_t );

	//	m_Bones[i].m_Name = pJoint->m_Name;
	// m_Bones[i].m_ParentIndex = -1;

		// Find this bone's parent index
	/*	for ( uint parentIdx = 0; parentIdx < i; parentIdx++ ) {
			if ( m_Bones[parentIdx].m_Name == pJoint->m_ParentName ) {
				m_Bones[i].m_ParentIndex = parentIdx;
				break;
			}
		}

		if ( i > 0 && m_Bones[i].m_ParentIndex == -1 ) {
			kbWarning( "kbModel::Load_Internal() - Missing parent in model %s at index %d", GetName().c_str(), i );
		}

		m_Bones[i].m_Position.Set( pJoint->m_Position[0], pJoint->m_Position[1], -pJoint->m_Position[2] );
*
		// Convert rotation from euler angles to quaternions
		kbQuat rotationX( kbVec3::right, pJoint->m_Rotation[0] ); 
		kbQuat rotationY( kbVec3::up, pJoint->m_Rotation[1] );
		kbQuat rotationZ( kbVec3::forward, -pJoint->m_Rotation[2] );
		m_Bones[i].m_Rotation = rotationX * rotationY * rotationZ;
*/
		// Load Animation here
		const ushort NumTranslationKeyFrames = pJoint->m_NumPositionKeyFrames;
		const ushort NumRotationKeyFrames = pJoint->m_NumRotationKeyFrames;

		const ms3dRotationKeyFrame_t * rotationKeyFrames = ( ms3dRotationKeyFrame_t * ) pPtr;
		pPtr += sizeof( ms3dPositionKeyFrame_t ) * NumRotationKeyFrames;

		const ms3dPositionKeyFrame_t * positionKeyFrames = ( ms3dPositionKeyFrame_t * ) pPtr;
		pPtr += sizeof( ms3dPositionKeyFrame_t ) * NumTranslationKeyFrames;

		kbAnimation::kbBoneKeyFrames_t & jointData = m_JointKeyFrameData[i];
		jointData.m_RotationKeyFrames.resize( NumRotationKeyFrames );
		jointData.m_TranslationKeyFrames.resize( NumTranslationKeyFrames );

		for ( int iKey = 0; iKey < NumRotationKeyFrames; iKey++ ) {
			const kbQuat rotationX( kbVec3::right, rotationKeyFrames[iKey].m_Rotation[0] );
			const kbQuat rotationY( kbVec3::up, rotationKeyFrames[iKey].m_Rotation[1] );
			const kbQuat rotationZ( kbVec3::forward, -rotationKeyFrames[iKey].m_Rotation[2] );
		
		
			jointData.m_RotationKeyFrames[iKey].m_Rotation = rotationX * rotationY * rotationZ;
			jointData.m_RotationKeyFrames[iKey].m_Time = rotationKeyFrames[iKey].m_Time;
	
			kbLog( "[%s] Euler = %f %f %f", pJoint->m_Name, rotationKeyFrames[iKey].m_Rotation[0], rotationKeyFrames[iKey].m_Rotation[1], rotationKeyFrames[iKey].m_Rotation[2] );
			kbMat4 rotMat = jointData.m_RotationKeyFrames[iKey].m_Rotation.ToMat4();
			kbLog( "		Vector = (%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f %.2f)",  rotMat[0].x, rotMat[0].y, rotMat[0].z, rotMat[1].x, rotMat[1].y, rotMat[1].z, rotMat[2].x, rotMat[2].y, rotMat[2].z );

			if ( jointData.m_RotationKeyFrames[iKey].m_Time > m_LengthInSeconds )
			{
				m_LengthInSeconds = jointData.m_RotationKeyFrames[iKey].m_Time;
			}
		}

		for ( int iKey = 0; iKey < NumTranslationKeyFrames; iKey++ ) {
			jointData.m_TranslationKeyFrames[iKey].m_Position.Set( positionKeyFrames[iKey].m_Position[0], positionKeyFrames[iKey].m_Position[1], -positionKeyFrames[iKey].m_Position[2] );
			jointData.m_TranslationKeyFrames[iKey].m_Time = positionKeyFrames[iKey].m_Time;
			kbLog( "		Trans = (%.2f %.2f %.2f)", jointData.m_TranslationKeyFrames[iKey].m_Position.x, jointData.m_TranslationKeyFrames[iKey].m_Position.y, jointData.m_TranslationKeyFrames[iKey].m_Position.z );

			if ( jointData.m_TranslationKeyFrames[iKey].m_Time > m_LengthInSeconds )
			{
				m_LengthInSeconds = jointData.m_TranslationKeyFrames[iKey].m_Time;
			}
		
		}
	}

	delete[] pMemoryFileBuffer;

	return true;
}

void kbAnimation::Release_Internal() {

}

kbBoneMatrix_t operator *( const kbBoneMatrix_t & op1, const kbBoneMatrix_t & op2 ) {
	kbBoneMatrix_t returnMatrix;

	returnMatrix.m_Axis[0].x = op1.m_Axis[0].x*op2.m_Axis[0].x + op1.m_Axis[0].y*op2.m_Axis[1].x + op1.m_Axis[0].z*op2.m_Axis[2].x;
	returnMatrix.m_Axis[1].x = op1.m_Axis[1].x*op2.m_Axis[0].x + op1.m_Axis[1].y*op2.m_Axis[1].x + op1.m_Axis[1].z*op2.m_Axis[2].x;
	returnMatrix.m_Axis[2].x = op1.m_Axis[2].x*op2.m_Axis[0].x + op1.m_Axis[2].y*op2.m_Axis[1].x + op1.m_Axis[2].z*op2.m_Axis[2].x;
	returnMatrix.m_Axis[3].x = op1.m_Axis[3].x*op2.m_Axis[0].x + op1.m_Axis[3].y*op2.m_Axis[1].x + op1.m_Axis[3].z*op2.m_Axis[2].x + op2.m_Axis[3].x;

	returnMatrix.m_Axis[0].y = op1.m_Axis[0].x*op2.m_Axis[0].y + op1.m_Axis[0].y*op2.m_Axis[1].y + op1.m_Axis[0].z*op2.m_Axis[2].y;
	returnMatrix.m_Axis[1].y = op1.m_Axis[1].x*op2.m_Axis[0].y + op1.m_Axis[1].y*op2.m_Axis[1].y + op1.m_Axis[1].z*op2.m_Axis[2].y;
	returnMatrix.m_Axis[2].y = op1.m_Axis[2].x*op2.m_Axis[0].y + op1.m_Axis[2].y*op2.m_Axis[1].y + op1.m_Axis[2].z*op2.m_Axis[2].y;
	returnMatrix.m_Axis[3].y = op1.m_Axis[3].x*op2.m_Axis[0].y + op1.m_Axis[3].y*op2.m_Axis[1].y + op1.m_Axis[3].z*op2.m_Axis[2].y + op2.m_Axis[3].y;

	returnMatrix.m_Axis[0].z = op1.m_Axis[0].x*op2.m_Axis[0].z + op1.m_Axis[0].y*op2.m_Axis[1].z + op1.m_Axis[0].z*op2.m_Axis[2].z;
	returnMatrix.m_Axis[1].z = op1.m_Axis[1].x*op2.m_Axis[0].z + op1.m_Axis[1].y*op2.m_Axis[1].z + op1.m_Axis[1].z*op2.m_Axis[2].z;
	returnMatrix.m_Axis[2].z = op1.m_Axis[2].x*op2.m_Axis[0].z + op1.m_Axis[2].y*op2.m_Axis[1].z + op1.m_Axis[2].z*op2.m_Axis[2].z;
	returnMatrix.m_Axis[3].z = op1.m_Axis[3].x*op2.m_Axis[0].z + op1.m_Axis[3].y*op2.m_Axis[1].z + op1.m_Axis[3].z*op2.m_Axis[2].z + op2.m_Axis[3].z;
	return returnMatrix;
}

void kbModel::DrawDebugTBN( const kbVec3 & modelTranslation, const kbQuat & modelOrientation, const kbVec3 & scale ) {

	kbMat4 modelMatrix;
	modelMatrix.MakeScale( scale );
	modelMatrix *= modelOrientation.ToMat4();
	modelMatrix[3].Set( modelTranslation.x, modelTranslation.y, modelTranslation.z, 1.0f );

	for ( int i = 0; i < m_DebugPositions.size(); i++ ) {
		const kbVec3 worldPos = modelMatrix.TransformPoint( m_DebugPositions[i] );
		const kbVec3 worldNormal = m_DebugNormals[i] * modelMatrix;
		const kbVec3 worldTangent = m_DebugTangents[i] * modelMatrix;
		const kbVec3 worldBitangent = worldNormal.Cross( worldTangent ).Normalized();

		g_pRenderer->DrawLine( worldPos, worldPos + worldTangent * 3.0f, kbColor::red );
		g_pRenderer->DrawLine( worldPos, worldPos + worldBitangent * 3.0f, kbColor::green );
		g_pRenderer->DrawLine( worldPos, worldPos + worldNormal * 3.0f, kbColor::blue );
	}
}
