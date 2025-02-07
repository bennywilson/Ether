/// kbModel.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "blk_core.h"
#include "kbBounds.h"
#include "kbRenderBuffer.h"
#include "Matrix.h"
#include "kbRenderer_defs.h"
#include "kbMaterial.h"

/// kbModelIntersection_t
struct kbModelIntersection_t {
	kbModelIntersection_t() : t( FLT_MAX ), meshNum( -1 ), intersectionPoint( Vec3::zero ), hasIntersection( false ) { }

	float	t;
	int		meshNum;
	Vec3  intersectionPoint;
	bool	hasIntersection;
};

/// kbAnimation
class kbAnimation : public kbResource {
	friend class kbModel;

//---------------------------------------------------------------------------------------------------
public:
												kbAnimation();
	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_ANIMATION; }

	float										GetLengthInSeconds() const { return m_LengthInSeconds; }

private:

	virtual bool								Load_Internal();
	virtual void								Release_Internal();

private:
	struct kbRotationKeyFrame_t {
		float									m_Time;
		Quat4									m_Rotation;
	};

	struct kbTranslationKeyFrame_t {
		float									m_Time;
		Vec3									m_Position;
	};

	struct kbBoneKeyFrames_t {
		std::vector<kbRotationKeyFrame_t>		m_RotationKeyFrames;
		std::vector<kbTranslationKeyFrame_t>	m_TranslationKeyFrames;
	};

	std::vector<kbBoneKeyFrames_t>				m_JointKeyFrameData;
	float										m_LengthInSeconds;
};


Vec3 operator*( const Vec3 & op1, const kbBoneMatrix_t & op2 );
kbBoneMatrix_t operator *( const kbBoneMatrix_t & op1, const kbBoneMatrix_t & op2 );

struct AnimatedBone_t {
	Quat4										m_JointSpaceRotation;
	Vec3										m_JointSpacePosition;

	kbBoneMatrix_t								m_LocalSpaceMatrix;		
};


/// kbModel
class kbModel : public kbResource {
	friend	class kbRenderer_DX11;

//---------------------------------------------------------------------------------------------------
public:
												kbModel();
												~kbModel();

	struct mesh_t {
												mesh_t() : m_TriangleIndices( nullptr ) { m_Bounds.Reset(); }
		kbBounds								m_Bounds;
		unsigned short *						m_TriangleIndices;			// <- don't need to save this
		unsigned int							m_NumTriangles;
		unsigned int							m_IndexBufferIndex;
		unsigned char							m_MaterialIndex;

		std::vector< Vec3 >					m_Vertices;
	};


	struct bone_t {
		kbString								m_Name;
		unsigned short							m_ParentIndex;
		Quat4									m_RelativeRotation;
		Vec3									m_RelativePosition;
	};

	void										CreateDynamicModel( const UINT numVertices, const UINT numIndices, kbShader *const pShaderToUse = nullptr, kbTexture *const pTextureToUse = nullptr, const UINT VertexSizeInBytes = sizeof( vertexLayout ) );
    void                                        CreatePointCloud( const UINT numVertices, const std::string & ShaderToUse = "", const ECullMode cullingMode = CullMode_BackFaces, const UINT VertexSizeInBytes = sizeof( vertexLayout ) );

	void *										MapVertexBuffer();
	void										UnmapVertexBuffer( const INT numVerticesWritten = -1 );
	void *										MapIndexBuffer();
	void										UnmapIndexBuffer();
	bool										IsVertexBufferMapped() const { return m_bVBIsMapped; }
	bool										IsIndexBufferMapped() const { return m_bIBIsMapped; }
    bool                                        IsPointCloud() const { return m_bIsPointCloud; }

	// CPU Access
	void										SetCPUAccessOnly( const bool bCPUAccessOnly ) { m_bCPUAccessOnly = bCPUAccessOnly; }
	const std::vector<vertexLayout>	&			GetCPUVertices() const { return m_CPUVertices; }
	const std::vector<ushort> &					GetCPUIndices() const { return m_CPUIndices; }

	void										SwapTexture( const UINT MeshIdx, const kbTexture * pTexture, const int textureIdx );

	const std::vector< mesh_t >	&				GetMeshes() const { return m_Meshes; }
	const std::vector< kbMaterial > &			GetMaterials() const { return m_Materials; }

	const kbBounds &							GetBounds() const { return m_Bounds; }
	size_t										NumMeshes() const { return m_Meshes.size(); }
	size_t										NumMaterials() const { return m_Materials.size(); }
	size_t										NumVertices() const { return m_NumVertices; }
	UINT										VertexStride() const { return m_Stride; }

	kbModelIntersection_t						RayIntersection( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Vec3 & modelTranslation, const Quat4 & modelOrientation, const Vec3 & scale ) const;

	void										Animate( std::vector<kbBoneMatrix_t> & outMatrices, const float time, const kbAnimation *const pAnimation, const bool bLoopAnim );
	void										BlendAnimations( std::vector<kbBoneMatrix_t> & outMatrices, const kbAnimation *const pFromAnim, const float fromAnimTime, const bool bFromAnimLoops, const kbAnimation *const pToAnim, const float ToAnimTime, const bool bToAnimLoops, const float normalizedBlendTime );
	void										SetBoneMatrices( std::vector<AnimatedBone_t> & outMatrices, const float time, const kbAnimation *const pAnimation, const bool bLoopAnim );

	int											NumBones() const { return (int)m_Bones.size(); }
	int											GetBoneIndex( const kbString & BoneName ) const;
	const kbBoneMatrix_t &						GetRefBoneMatrix( const int index ) const { return m_RefPose[index]; }
	const kbBoneMatrix_t &						GetInvRefBoneMatrix( const int index ) const { return m_InvRefPose[index]; }

	// Debug
	void										DrawDebugTBN( const Vec3 & modelTranslation, const Quat4 & modelOrientation, const Vec3 & modelScale );

protected:

	virtual bool								Load_Internal();
	bool										LoadMS3D();
	bool										LoadFBX();
	bool										LoadDiablo3();

	virtual void								Release_Internal();


	kbRenderBuffer								m_VertexBuffer;
	kbRenderBuffer								m_IndexBuffer;

	kbBounds									m_Bounds;

	std::vector<ushort>							m_CPUIndices;
	std::vector<vertexLayout>					m_CPUVertices;

	int											m_NumTriangles;
	int											m_NumVertices;
	std::vector<mesh_t>							m_Meshes;
	std::vector<kbMaterial>						m_Materials;
	std::vector<bone_t>							m_Bones;
	std::vector<kbBoneMatrix_t>					m_RefPose;
	std::vector<kbBoneMatrix_t>					m_InvRefPose;

	UINT										m_Stride;

	bool										m_bIsDynamicModel       : 1;
    bool                                        m_bIsPointCloud         : 1;
	bool										m_bVBIsMapped           : 1;
	bool										m_bIBIsMapped           : 1;
	bool										m_bCPUAccessOnly        : 1;

private:

	virtual kbTypeInfoType_t					GetType() const { return KBTYPEINFO_STATICMODEL; }

	virtual void								Load( const std::string & fileName ) { };

	// Debug
	std::vector<Vec3>							m_DebugPositions;
	std::vector<Vec3>							m_DebugNormals;
	std::vector<Vec3>							m_DebugTangents;
};
