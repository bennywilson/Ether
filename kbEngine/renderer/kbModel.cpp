//==============================================================================
// kbModel.cpp
//
// General model format based off of the ms3d specs
//
// 2016-2017 kbEngine 2.0
//==============================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbIntersectionTests.h"
#include "kbModel.h"

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
 * kbModel::kbModel
 */
kbModel::kbModel() :
	m_NumVertices( 0 ),
	m_NumTriangles( 0 ),
	m_Stride( sizeof( vertexLayout ) ),
	m_bIsDynamicModel( false ),
	m_bVBIsMapped( false ),
	m_bIBIsMapped( false ),
	m_bCPUAccessOnly( false )  {
}

/**
 * kbModel::~kbModel
 */
kbModel::~kbModel() {
	Release_Internal();
}

/**
 *	kbModel::Load_Internal
 */
bool kbModel::Load_Internal() {

	std::ifstream modelFile;
	modelFile.open( m_FullFileName, std::ifstream::in | std::ifstream::binary );

	if ( modelFile.fail() ) {
		kbError( "Error: kbModel::LoadResource_Internal - Failed to load model %s", m_FullFileName.c_str() );
	}
	
	// Find the file size
	modelFile.seekg( 0, std::ifstream::end );
	std::streamoff fileSize = modelFile.tellg();
	modelFile.seekg( 0, std::ifstream::beg );

	// Load file into memory
	char * pMemoryFileBuffer = new char[ fileSize ];
	modelFile.read( pMemoryFileBuffer, fileSize );
	modelFile.close();

	const char * pPtr = pMemoryFileBuffer;

	// Header
	const ms3dHeader_t * pHeader = ( const ms3dHeader_t * ) pPtr;
	pPtr += sizeof( ms3dHeader_t );

	if ( strncmp( pHeader->m_ID, "MS3D000000", 10 ) != 0 ) {
		kbError( "Error: kbModel::LoadResource_Internal - Invalid model header %d", pHeader->m_ID );
	}

	// Vertices
	m_Bounds.Reset();

	ushort numVertices = *( ushort * ) pPtr;
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
	const uint numTriangles = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );
	ms3dTriangle_t * tempTriangles = new ms3dTriangle_t[ numTriangles ];

	for ( uint i = 0; i < numTriangles; i++ ) {
		const ms3dTriangle_t * pTriangles = ( ms3dTriangle_t * ) pPtr;
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
	uint numSrcGroups = *( ushort * ) pPtr;
	pPtr += sizeof( ushort );

	// Look for groups named "collision" and build collision geometry from them
	int collisionGroupIdx = -1;
	const char * pCollisionPtr = pPtr;
	for ( uint iGroup = 0; iGroup < numSrcGroups; iGroup++ ) {

		pCollisionPtr += sizeof( byte );
		const char * groupName = (char*)( pCollisionPtr );

		pCollisionPtr += 32;
		const uint numTris = *(ushort *)( pCollisionPtr );

		pCollisionPtr += sizeof( ushort );

		if ( strcmp( groupName, "collision" ) == 0 ) {
			kbErrorCheck( collisionGroupIdx == -1, "Too many collision groups in %s", m_FullFileName.c_str() );
			collisionGroupIdx = iGroup;

			// Use collision geometry to determine the kbModel's bounds
			m_Bounds.Reset();
			for ( uint iTris = 0; iTris < numTris; iTris++ ) {
				unsigned short curTriIndex = *( ushort *) pCollisionPtr;

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
			currentMesh.m_TriangleIndices[iTris] = *( ushort *) pPtr;
			pPtr += sizeof( ushort );
		}

		currentMesh.m_MaterialIndex = *( byte * ) pPtr;

		if ( currentMesh.m_MaterialIndex == 255 ) {
			kbError( "Mesh is missing a material" );
		}
		pPtr += sizeof( char );
	}

	const uint numMaterials = *( ushort * ) pPtr;
	m_Materials.resize( numMaterials );
	pPtr += sizeof( ushort );

	std::string filePath = m_FullFileName;
	size_t stringPos = filePath.rfind( "/" );
	filePath.erase( stringPos );
	filePath.append( "/" );

	// todo:don't load duplicate textures
	for ( uint i = 0; i < numMaterials; i++ ) {
		ms3dMaterial_t * pMat = ( ms3dMaterial_t * ) pPtr;
		pPtr += sizeof( ms3dMaterial_t );

		m_Materials[i].m_DiffuseColor.Set( pMat->m_Diffuse[0], pMat->m_Diffuse[1], pMat->m_Diffuse[2], 1.0f );

		// get the base texture name
		std::string textureName = pMat->m_Texture;
		if ( textureName.length() == 0 ) {
			continue;
		}

		stringPos = textureName.rfind( "." );
		std::string fileExt = textureName.substr( stringPos + 1 );
		textureName.erase( stringPos );

		// load the base diffuse textures
		std::string diffuseTextureName = filePath;
		diffuseTextureName.append( pMat->m_Texture );

		m_Materials[i].m_Texture = ( kbTexture * ) g_ResourceManager.GetResource( diffuseTextureName.c_str(), true );

		std::string shaderName = pMat->m_Name;

		if ( shaderName.find( "NoCull" ) != std::string::npos ) {
			m_Materials[i].m_CullingMode = kbMaterial::CM_None;
		}

		const size_t endOfShaderName = shaderName.find("_");
		if ( endOfShaderName != std::string::npos ) {
			shaderName.resize( endOfShaderName );
		}
		shaderName += ".kbShader";
		m_Materials[i].m_pShader = ( kbShader * ) g_ResourceManager.GetResource( shaderName );
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

	std::map<vertexLayout, int> vertHash;

	for ( uint i = 0; i < m_Meshes.size(); i++ ) {

		if ( ibIndex != m_Meshes[i].m_IndexBufferIndex ) {
			kbError( "Index buffer mismatch" );
		}

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

				std::map<vertexLayout, int>::iterator it = vertHash.find( newVert );


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
		const ms3dBone_t * pJoint = ( ms3dBone_t * ) pPtr;
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

		if ( i > 0 && m_Bones[i].m_ParentIndex == -1 ) {
			kbWarning( "kbModel::Load_Internal() - Missing parent in model %s at index %d", GetName().c_str(), i );
		}

		m_Bones[i].m_RelativePosition.Set( pJoint->m_Position[0], pJoint->m_Position[1], -pJoint->m_Position[2] );

		// Convert rotation from euler angles to quaternions
		kbQuat rotationX( kbVec3::right, pJoint->m_Rotation[0] ); 
		kbQuat rotationY( kbVec3::up, pJoint->m_Rotation[1] );
		kbQuat rotationZ( kbVec3::forward, -pJoint->m_Rotation[2] );
		m_Bones[i].m_RelativeRotation = rotationX * rotationY * rotationZ;

		// Load Animation here
		const ushort NumTranslationKeyFrames = pJoint->m_NumPositionKeyFrames;
		const ushort NumRotationKeyFrames = pJoint->m_NumRotationKeyFrames;

		const ms3dRotationKeyFrame_t * rotationKeyFrames = ( ms3dRotationKeyFrame_t * ) pPtr;
		pPtr += sizeof( ms3dPositionKeyFrame_t ) * NumRotationKeyFrames;

		const ms3dPositionKeyFrame_t * positionKeyFrames = ( ms3dPositionKeyFrame_t * ) pPtr;
		pPtr += sizeof( ms3dPositionKeyFrame_t ) * NumTranslationKeyFrames;
	}

	delete[] tempVertices;
	delete[] boneIndices;
	delete[] tempTriangles;
	delete[] pMemoryFileBuffer;

	// Build ref pose
	m_RefPose.insert( m_RefPose.begin(), m_Bones.size(), kbBoneMatrix_t() );
	m_InvRefPose.insert( m_InvRefPose.begin(), m_Bones.size(), kbBoneMatrix_t() );

	for ( int i = 0; i < m_Bones.size(); i++ ) {
		if ( i > 0 ) {
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
		} else {
			const kbMat4 rotationmat = m_Bones[i].m_RelativeRotation.ToMat4();
			m_RefPose[0].SetAxis( 0, rotationmat[0].ToVec3() );
			m_RefPose[0].SetAxis( 1, rotationmat[1].ToVec3() );
			m_RefPose[0].SetAxis( 2, rotationmat[2].ToVec3() );
			m_RefPose[0].SetAxis( 3,  m_Bones[0].m_RelativePosition );
		}

		m_InvRefPose[i] = m_RefPose[i];
		m_InvRefPose[i].Invert();
	}

	return true;
}

/**
 *	kbModel::CreateDynamicModel
 */
void kbModel::CreateDynamicModel( const UINT numVertices, const UINT numIndices, const std::string & ShaderToUse, const std::string & TextureToUse, const UINT vertexSizeInBytes ) {

	if ( m_NumVertices > 0 || m_Meshes.size() > 0 || m_Materials.size() > 0 || m_VertexBuffer.GetBufferPtr() != nullptr || m_IndexBuffer.GetBufferPtr() != nullptr ) {
		kbError( "kbModel::CreateDynamicModel() called on an already initialized kbModel" );
	}

	m_NumVertices = numVertices;
	m_bIsDynamicModel = true;
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
	if ( ShaderToUse.length() > 0 ) {
		newMaterial.m_pShader = ( kbShader * ) g_ResourceManager.GetResource( ShaderToUse.c_str(), true );
	} else {
		newMaterial.m_pShader = ( kbShader * ) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicShader.kbShader", true );
	}

	m_Materials.push_back( newMaterial );
}

/**
 *	kbModel::MapVertexBuffer
 */
void * kbModel::MapVertexBuffer() {
	if( m_bVBIsMapped ) {
		kbError( "Vertex buffer already mapped" );
	}

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
	if( m_bIBIsMapped ) {
		kbError( "Index buffer already mapped" );
	}

	m_bIBIsMapped = true;
	return m_IndexBuffer.Map();
}

/**
 *	kbModel::UnmapIndexBuffer
 */
void kbModel::UnmapIndexBuffer() {

   if ( m_bIBIsMapped == false ) {
      return;
   }

	m_bIBIsMapped = false;
	m_IndexBuffer.Unmap();
}

/**
 *	kbMode::SwapTexture
 */
void kbModel::SwapTexture( const UINT MeshIdx, const kbTexture * pTexture ) {
	
	if ( MeshIdx < 0 ||  MeshIdx >= m_Materials.size() )
	{
		return;
	}

	m_Materials[ MeshIdx ].m_Texture = pTexture;
}

/**
 *	kbModel::RayIntersection
 */
kbModelIntersection_t kbModel::RayIntersection( const kbVec3 & inRayOrigin, const kbVec3 & inRayDirection, const kbVec3 & modelTranslation, const kbQuat & modelOrientation ) {
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

			if ( kbRayTriIntersection( rayStart, rayDir, v0, v1, v2, t ) ) {
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
void kbModel::SetBoneMatrices( const float time, const kbAnimation *const theAnimation, const bool bIsLooping, std::vector<tempBone_t> & bones ) {
	if ( m_Bones.size() == 0 ) {
		return;
	}

	if ( theAnimation == nullptr ) {
		return;
	}

	const kbAnimation & baseAnim = *theAnimation;
	const float maxLength = baseAnim.m_LengthInSeconds;
	const float animTime = ( bIsLooping && time > maxLength ) ? ( fmod( time, maxLength ) ) : ( time );

	bones.resize( m_Bones.size() );

	for ( int i = 0; i < m_Bones.size(); i++ ) {

		kbQuat nextBoneRotation( 0.0f, 0.0f, 0.0f, 1.0f );
		kbVec3 nextBonePosition( 0.0f, 0.0f, 0.0f );

		const kbAnimation::kbBoneKeyFrames_t & jointData = baseAnim.m_JointKeyFrameData[i];
		for ( int nextKey = 0; nextKey < jointData.m_RotationKeyFrames.size(); nextKey++ ) {

			float nextTime = jointData.m_RotationKeyFrames[nextKey].m_Time;
			if ( animTime >= nextTime && nextKey != jointData.m_RotationKeyFrames.size() - 1 ) {
				continue;
			}

			nextBoneRotation = jointData.m_RotationKeyFrames[nextKey].m_Rotation;

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
			nextBoneRotation = kbQuat::Slerp( prevBoneRotation, nextRotation, t );
			nextBonePosition = prevBonePosition + ( nextPosition - prevBonePosition ) * t;
			break;
		}

		bones[i].position = nextBonePosition;
		bones[i].rotation = nextBoneRotation;
	}
}

/**
 *	kbModel::BlendAnimations
 */
void kbModel::Animate( const float time, const kbAnimation *const pAnimation, const bool bLoopAnim, std::vector<kbBoneMatrix_t> & boneMatrices ) {
	std::vector<tempBone_t> tempBones;
	SetBoneMatrices( time, pAnimation, bLoopAnim, tempBones );

	for ( int i = 0; i < tempBones.size(); i++ ) {

		const int parent = m_Bones[i].m_ParentIndex;
		if ( parent == 65535 ) {
			tempBones[i].worldRotation = tempBones[i].rotation * m_Bones[i].m_RelativeRotation;
		} else {
			tempBones[i].worldRotation = tempBones[i].rotation * m_Bones[i].m_RelativeRotation * tempBones[parent].worldRotation;
		}
		tempBones[i].worldRotation.Normalize();

		const kbMat4 boneWorldMatrix = tempBones[i].worldRotation.ToMat4();
		tempBones[i].worldMatrix.SetAxis( 0, boneWorldMatrix[0].ToVec3() );
		tempBones[i].worldMatrix.SetAxis( 1, boneWorldMatrix[1].ToVec3() );
		tempBones[i].worldMatrix.SetAxis( 2, boneWorldMatrix[2].ToVec3() );

		kbVec3 finalBonePosition = m_Bones[i].m_RelativePosition + tempBones[i].position;
		if ( parent != 65535 ) {
			finalBonePosition = finalBonePosition * tempBones[parent].worldMatrix;
		}
		tempBones[i].worldMatrix.SetAxis( 3, finalBonePosition );

		const kbBoneMatrix_t & invRef = GetInvRefBoneMatrix(i);
		boneMatrices[i] = invRef * tempBones[i].worldMatrix;
	}
}

/**
 *	kbModel::BlendAnimations
 */
void kbModel::BlendAnimations( const kbAnimation *const pFromAnim, const float FromAnimTime, const bool bFromAnimLoops, const kbAnimation *const pToAnim, const float ToAnimTime, const bool bToAnimLoops, const float normalizedBlendTime, std::vector<kbBoneMatrix_t> & boneMatrices ) {

	std::vector<tempBone_t> fromTempBones;
	SetBoneMatrices( FromAnimTime, pFromAnim, bFromAnimLoops, fromTempBones );

	std::vector<tempBone_t> toTempBones;
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
	}
}

/**
 *	kbAnimation::kbAnimation
 */
kbAnimation::kbAnimation() :
	m_LengthInSeconds( 0 ) {
}

/**
 *	kbAnimation::kbAnimation
 */
bool kbAnimation::Load_Internal() {

	std::ifstream modelFile;
	modelFile.open( m_FullFileName, std::ifstream::in | std::ifstream::binary );

	if ( modelFile.fail() ) {
		kbError( "Error: kbModel::LoadResource_Internal - Failed to load model %s", m_FullFileName.c_str() );
	}
	
	// Find the file size
	modelFile.seekg( 0, std::ifstream::end );
	std::streamoff fileSize = modelFile.tellg();
	modelFile.seekg( 0, std::ifstream::beg );

	// Load file into memory
	char * pMemoryFileBuffer = new char[ fileSize ];
	modelFile.read( pMemoryFileBuffer, fileSize );
	modelFile.close();

	const char * pPtr = pMemoryFileBuffer;

	// Header
	const ms3dHeader_t * pHeader = ( const ms3dHeader_t * ) pPtr;
	pPtr += sizeof( ms3dHeader_t );

	if ( strncmp( pHeader->m_ID, "MS3D000000", 10 ) != 0 ) {
		kbError( "Error: kbModel::LoadResource_Internal - Invalid model header %d", pHeader->m_ID );
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
		
			if ( jointData.m_RotationKeyFrames[iKey].m_Time > m_LengthInSeconds )
			{
				m_LengthInSeconds = jointData.m_RotationKeyFrames[iKey].m_Time;
			}
		}

		for ( int iKey = 0; iKey < NumTranslationKeyFrames; iKey++ ) {
			jointData.m_TranslationKeyFrames[iKey].m_Position.Set( positionKeyFrames[iKey].m_Position[0], positionKeyFrames[iKey].m_Position[1], -positionKeyFrames[iKey].m_Position[2] );
			jointData.m_TranslationKeyFrames[iKey].m_Time = positionKeyFrames[iKey].m_Time;
		
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
