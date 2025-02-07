//===================================================================================================
// kbRenderer_defs.h
//
//
// 2025 blk 1.0
//===================================================================================================
#pragma once

#include "kbJobManager.h"
#include "Matrix.h"
#include "Quaternion.h"

enum ERenderPass {
	RP_FirstPerson,
	RP_Lighting,
	RP_Translucent,
	RP_TranslucentWithDepth,
	RP_PostLighting,
	RP_InWorldUI,
	RP_Distortion,
	RP_PostProcess,
	RP_UI,
	RP_Debug,
	RP_MousePicker,
	NUM_RENDER_PASSES
};

enum ECullMode {
	CullMode_ShaderDefault,
	CullMode_None,
	CullMode_FrontFaces,
	CullMode_BackFaces,
};

struct vertexColorLayout {

	Vec3 position;
	byte color[4];

	void SetColor( const Vec4 & inColor ) {
		color[0] = (byte) ( inColor.z * 255.0f );
		color[1] = (byte) ( inColor.y * 255.0f );
		color[2] = (byte) ( inColor.x * 255.0f );
		color[3] = (byte) ( inColor.w * 255.0f );
	}

	Vec4 GetColor() const {
		Vec4 outColor( ( float ) color[2], ( float ) color[1], ( float ) color[0], ( float ) color[3] );
		outColor.x = outColor.x / 255.0f;
		outColor.y = outColor.y / 255.0f;
		outColor.z = outColor.z / 255.0f;
		outColor.w = outColor.w / 255.0f;

		return outColor;
	}
};

struct kbParticleVertex {

	void SetColor( const Vec4 & inColor ) {
		color[0] = (byte) ( inColor.x * 255.0f );
		color[1] = (byte) ( inColor.y * 255.0f );
		color[2] = (byte) ( inColor.z * 255.0f );
		color[3] = (byte) ( inColor.w * 255.0f );
	}

	Vec3	position;
	Vec2  uv;
	byte	color[4];
	Vec2	size;
	Vec3	direction;
	float	rotation;
	byte	billboardType[4];
};


/// kbBoneMatrix_t
struct kbBoneMatrix_t {

	kbBoneMatrix_t() { }

	explicit kbBoneMatrix_t( const Quat4 & quat, const Vec3 & pos ) {
		SetFromQuat( quat );
		m_Axis[3] = pos;
	}

	void SetIdentity() {
		m_Axis[0].set( 1.0f, 0.0f, 0.0f );
		m_Axis[1].set( 0.0f, 1.0f, 0.0f );
		m_Axis[2].set( 0.0f, 0.0f, 1.0f );
		m_Axis[3].set( 0.0f, 0.0f, 0.0f );
	}

	const Vec3& operator[]( const int index ) const { return GetAxis( index ); }
	const Vec3 & GetAxis( const int axisIndex ) const { if ( axisIndex < 0 || axisIndex > 3 ) { blk::error("Doh!"); } return m_Axis[axisIndex]; }
	const Vec3 & GetOrigin() const { return m_Axis[3]; }
	void SetAxis( const int axisIndex, const Vec3 & inVec ) { if ( axisIndex < 0 || axisIndex > 3 ) { blk::error("Doh!"); } m_Axis[axisIndex] = inVec; }
	void SetFromQuat( const Quat4 & srcQuat );

	void TransposeUpper();

	void Invert();

	void operator*=( const kbBoneMatrix_t & op2 );
	void operator*=( const Mat4 & op2 );

	Vec3 m_Axis[4];
};

/// kbRenderJob
class kbRenderJob : public kbJob {

//---------------------------------------------------------------------------------------------------
public:
												kbRenderJob() : m_bRequestShutdown( false ) { }

	void										Run();

	void										RequestShutdown() { m_bRequestShutdown = true; }

private:
	bool										m_bRequestShutdown;
};

/// kbShaderParamOverrides_t
struct kbShaderParamOverrides_t {

    struct kbShaderParam_t {

        enum type {
            SHADER_MAT4,
            SHADER_VEC4,
			SHADER_MAT4_LIST,
			SHADER_VEC4_LIST,
            SHADER_TEX,
        } m_Type;

		std::vector<Mat4> m_Mat4List;
		std::vector<Vec4> m_Vec4List;

        kbShaderParam_t() : m_pTexture( nullptr ), m_pRenderTexture( nullptr ) { }
        const class kbTexture *			m_pTexture;
		const class kbRenderTexture *	m_pRenderTexture;
        std::string						m_VarName;
        size_t							m_VarSizeBytes;
    };

    kbShaderParamOverrides_t() : m_pShader( nullptr ), m_CullModeOverride( CullMode_ShaderDefault ) { }

    std::vector<kbShaderParam_t>		m_ParamOverrides;
	const class kbShader *				m_pShader;
	ECullMode							m_CullModeOverride;

    kbShaderParam_t & AllocateParam( const std::string & varName ) {
		for ( int i = 0; i < m_ParamOverrides.size(); i++ ) {
			if ( m_ParamOverrides[i].m_VarName == varName ) {
				return m_ParamOverrides[i];
			}
		}

        kbShaderParam_t newParam;
        m_ParamOverrides.push_back( newParam );
        return m_ParamOverrides[m_ParamOverrides.size() - 1];
    }

    void SetMat4( const std::string & varName, const Mat4 & newMat ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
		newParam.m_Mat4List.clear();
        newParam.m_Mat4List.push_back( newMat );
        newParam.m_Type = kbShaderParam_t::SHADER_MAT4;
        newParam.m_VarSizeBytes = sizeof(Mat4);
    }

	void SetMat4List( const std::string & varName, const std::vector<Mat4> & list ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_Mat4List = list;
        newParam.m_Type = kbShaderParam_t::SHADER_MAT4_LIST;
        newParam.m_VarSizeBytes = sizeof(Vec4);
	}

    void SetVec4( const std::string & varName, const Vec4 & newVec ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
		newParam.m_Vec4List.clear();
        newParam.m_Vec4List.push_back( newVec );
        newParam.m_Type = kbShaderParam_t::SHADER_VEC4;
        newParam.m_VarSizeBytes = sizeof(Vec4);
    }

	void SetVec4List( const std::string & varName, const std::vector<Vec4> & list ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_Vec4List = list;
        newParam.m_Type = kbShaderParam_t::SHADER_VEC4_LIST;
        newParam.m_VarSizeBytes = sizeof(Vec4);
	}

    void SetTexture( const std::string & varName, const kbTexture *const pTexture ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_pTexture = pTexture;
        newParam.m_Type = kbShaderParam_t::SHADER_TEX;
        newParam.m_VarSizeBytes = sizeof(kbTexture*);
    }

	void SetTexture( const std::string & varName, const kbRenderTexture *const pRenderTexture ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_pRenderTexture = pRenderTexture;
        newParam.m_Type = kbShaderParam_t::SHADER_TEX;
        newParam.m_VarSizeBytes = sizeof(kbRenderTexture*);
	}
};

/// kbRenderObject
class kbRenderObject {

//---------------------------------------------------------------------------------------------------
public:
												kbRenderObject() : 
													m_pComponent( nullptr ),
													m_pModel( nullptr ),
													m_RenderPass( RP_Lighting ),
													m_CullMode( CullMode_ShaderDefault ),
													m_RenderOrderBias( 0.0f ),
													m_EntityId( 0 ),
													m_VertBufferStartIndex( -1 ),
													m_VertBufferIndexCount( -1 ),
													m_CullDistance( -1.0f ),
													m_bCastsShadow( false ),
													m_bIsSkinnedModel( false ),
													m_bIsFirstAdd( true ),
													m_bIsRemove( false ) { }

	const class kbGameComponent *				m_pComponent;
	const class kbModel *						m_pModel;
	std::vector<kbShaderParamOverrides_t>		m_Materials;
	ERenderPass									m_RenderPass;
	ECullMode									m_CullMode;
	float										m_RenderOrderBias;
	Vec3										m_Position;
	Quat4										m_Orientation;
	Vec3										m_Scale;
	uint										m_EntityId;

	int											m_VertBufferStartIndex;
	int											m_VertBufferIndexCount;
	
	std::vector<kbBoneMatrix_t>					m_MatrixList;

	float										m_CullDistance;

	bool										m_bCastsShadow			: 1;
	bool										m_bIsSkinnedModel		: 1;

	// Updated by renderer
	bool										m_bIsFirstAdd			: 1;
	bool										m_bIsRemove				: 1;
};

/// kbRenderLight
class kbRenderLight {

//---------------------------------------------------------------------------------------------------
public:
												kbRenderLight() :
													m_pLightComponent( nullptr ),
													m_bIsFirstAdd( false ),
													m_bIsRemove( false ) {
														memset( &m_CascadedShadowSplits, 0, sizeof( m_CascadedShadowSplits ) );
													}

	const class kbLightComponent *				m_pLightComponent;
	Vec3										m_Position;
	Quat4										m_Orientation;
	Vec4										m_Color;
	float										m_Radius;
	float										m_Length;
	float										m_CascadedShadowSplits[4];
	bool										m_bCastsShadow;
	bool										m_bIsFirstAdd;
	bool										m_bIsRemove;
};

/// eRenderObjectOp
enum eRenderObjectOp {
	ROO_Add,
	ROO_Remove,
	ROO_Update,
};

/// kbLightShafts
class kbLightShafts {

//---------------------------------------------------------------------------------------------------
public:
												kbLightShafts() :
													m_pLightShaftsComponent( nullptr ),
													m_pTexture( nullptr ),
													m_Color(0.0f, 0.0f, 0.0f, 1.0f ),
													m_Pos( Vec3::zero ),
													m_Rotation( Quat4::zero ),
													m_Width( 0.0f ),
													m_Height( 0.0f ),
													m_NumIterations( 0 ),
													m_IterationWidth( 0.0f ), 
													m_IterationHeight( 0.0f ),
													m_Operation( ROO_Add ),
													m_bIsDirectional( true ) {
														m_Rotation.set( 0.0f, 0.0f, 0.0f, 1.0f );
													}

	const class kbLightShaftsComponent *		m_pLightShaftsComponent;
	class kbTexture *							m_pTexture;
	kbColor										m_Color;
	Vec3										m_Pos;
	Quat4										m_Rotation;
	float										m_Width;
	float										m_Height;
	int											m_NumIterations;
	float										m_IterationWidth;
	float										m_IterationHeight;
	eRenderObjectOp								m_Operation;
	bool										m_bIsDirectional;
};

/// kbRenderTargetMap
struct kbRenderTargetMap {
	byte *										m_pData;
	uint										m_Width;
	uint										m_Height;
	uint										m_rowPitch;
};

enum kbBlend {
	Blend_Zero,
	Blend_One,
	Blend_SrcColor,
	Blend_InvSrcColor,
	Blend_SrcAlpha,
	Blend_InvSrcAlpha,
	Blend_DstAlpha,
	Blend_InvDstAlpha,
	Blend_DstColor,
	Blend_InvDstColor,
};

enum kbBlendOp {
	BlendOp_Add,
	BlendOp_Subtract,
	BlendOp_Max,
	BlendOp_Min
};

enum kbColorWriteEnable {
	ColorWriteEnable_Red	= 1,
	ColorWriteEnable_Green	= 2,
	ColorWriteEnable_Blue	= 4,
	ColorWriteEnable_Alpha	= 8,
	ColorWriteEnable_RGB	= ColorWriteEnable_Red | ColorWriteEnable_Green | ColorWriteEnable_Blue,
	ColorWriteEnable_All	= ColorWriteEnable_Red | ColorWriteEnable_Green | ColorWriteEnable_Blue | ColorWriteEnable_Alpha
};

kbColorWriteEnable operator |( const kbColorWriteEnable lhs, const kbColorWriteEnable rhs );
