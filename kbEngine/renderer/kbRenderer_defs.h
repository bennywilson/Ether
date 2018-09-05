//===================================================================================================
// kbRenderer_defs.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBRENDERERDEFS_H_
#define _KBRENDERERDEFS_H_

#include "kbJobManager.h"
#include "kbVector.h"
#include "kbQuaternion.h"

enum ERenderPass {
	RP_FirstPerson,
	RP_Lighting,
	RP_Translucent,
	RP_PostLighting,
	RP_InWorldUI,
	RP_Debug,
	RP_MousePicker,
	NUM_RENDER_PASSES
};

struct vertexColorLayout {

	kbVec3 position;
	byte color[4];

	void SetColor( const kbVec4 & inColor ) {
		color[0] = ( byte ) ( inColor.z * 255.0f );
		color[1] = ( byte ) ( inColor.y * 255.0f );
		color[2] = ( byte ) ( inColor.x * 255.0f );
		color[3] = ( byte ) ( inColor.w * 255.0f );
	}

	kbVec4 GetColor() const {
		kbVec4 outColor( ( float ) color[2], ( float ) color[1], ( float ) color[0], ( float ) color[3] );
		outColor.x = outColor.x / 255.0f;
		outColor.y = outColor.y / 255.0f;
		outColor.z = outColor.z / 255.0f;
		outColor.w = outColor.w / 255.0f;

		return outColor;
	}
};

struct kbParticleVertex {

	void SetColor( const kbVec4 & inColor ) {
		color[0] = (byte) ( inColor.x * 255.0f );
		color[1] = (byte) ( inColor.y * 255.0f );
		color[2] = (byte) ( inColor.z * 255.0f );
		color[3] = (byte) ( inColor.w * 255.0f );
	}

	kbVec3	position;
	kbVec2  uv;
	byte	color[4];
	kbVec2	size;
	kbVec3	direction;
	float	rotation;
	byte	billboardType[4];
};

struct vertexLayout {

	kbVec3 position;
	kbVec2 uv;
	byte color[4];
	byte normal[4];
	byte tangent[4];

	void SetColor( const kbVec4 & inColor ) {
		color[0] = ( byte ) ( inColor.x * 255.0f );
		color[1] = ( byte ) ( inColor.y * 255.0f );
		color[2] = ( byte ) ( inColor.z * 255.0f );
		color[3] = ( byte ) ( inColor.w * 255.0f );
	}

	void SetNormal( const kbVec4 & inNormal ) {
		normal[0] = ( byte ) ( ( ( inNormal.x * 0.5f ) + 0.5f ) * 255.0f );
		normal[1] = ( byte ) ( ( ( inNormal.y * 0.5f ) + 0.5f ) * 255.0f );
		normal[2] = ( byte ) ( ( ( inNormal.z * 0.5f ) + 0.5f ) * 255.0f );
		normal[3] = ( byte ) ( ( ( inNormal.w * 0.5f ) + 0.5f ) * 255.0f );
	}
	
	void SetTangent( const kbVec4 & inTangent ) {
		tangent[0] = ( byte ) ( ( ( inTangent.x * 0.5f ) + 0.5f ) * 255.0f );
		tangent[1] = ( byte ) ( ( ( inTangent.y * 0.5f ) + 0.5f ) * 255.0f );
		tangent[2] = ( byte ) ( ( ( inTangent.z * 0.5f ) + 0.5f ) * 255.0f );
		tangent[3] = ( byte ) ( ( ( inTangent.w * 0.5f ) + 0.5f ) * 255.0f );
	}

	void SetBitangent( const kbVec4 & inBitangent ) {
		color[0] = ( byte ) ( ( ( inBitangent.x * 0.5f ) + 0.5f ) * 255.0f );
		color[1] = ( byte ) ( ( ( inBitangent.y * 0.5f ) + 0.5f ) * 255.0f );
		color[2] = ( byte ) ( ( ( inBitangent.z * 0.5f ) + 0.5f ) * 255.0f );
		color[3] = ( byte ) ( ( ( inBitangent.w * 0.5f ) + 0.5f ) * 255.0f );
	}

	kbVec3 GetNormal() const {
		kbVec3 outNormal( ( float ) normal[0], ( float ) normal[1], ( float ) normal[2] );
		outNormal.x = ( ( outNormal.x / 255.0f ) * 2.0f ) - 1.0f;
		outNormal.y = ( ( outNormal.y / 255.0f ) * 2.0f ) - 1.0f;
		outNormal.z = ( ( outNormal.z / 255.0f ) * 2.0f ) - 1.0f;

		outNormal.Normalize();
		return outNormal;
	}

	kbVec3 GetTangent() const {
		kbVec3 outTangent( ( float ) tangent[0], ( float ) tangent[1], ( float ) tangent[2] );
		outTangent.x = ( ( outTangent.x / 255.0f ) * 2.0f ) - 1.0f;
		outTangent.y = ( ( outTangent.y / 255.0f ) * 2.0f ) - 1.0f;
		outTangent.z = ( ( outTangent.z / 255.0f ) * 2.0f ) - 1.0f;

		outTangent.Normalize();
		return outTangent;
	}

	kbVec4 GetColor() const {
		kbVec4 outColor( ( float ) color[2], ( float ) color[1], ( float ) color[0], ( float ) color[3] );
		outColor.x = outColor.x / 255.0f;
		outColor.y = outColor.y / 255.0f;
		outColor.z = outColor.z / 255.0f;
		outColor.w = outColor.w / 255.0f;

		return outColor;
	}

	void Clear() {
		memset( this, 0, sizeof( vertexLayout ) );
	}

	bool operator == ( const vertexLayout & op2 ) const {
		const float epsilon = 0.0000001f;
		return position.Compare( op2.position, epsilon ) && uv.Compare( op2.uv, epsilon ) && 
			normal[0] == op2.normal[0] && normal[1] == op2.normal[1] && normal[2] == op2.normal[2];
	}

	bool operator < ( const vertexLayout & op2 ) const {

		if ( position.x < op2.position.x ) {
			return true;
		} else if ( position.x > op2.position.x ) {
			return false;
		} else if ( position.y < op2.position.y ) {
			return true;
		} else if ( position.y > op2.position.y ) {
			return false;
		} else if ( position.z < op2.position.z ) {
			return true;
		} else if ( position.z > op2.position.z ) {
			return false;
		} else if ( normal[0] < op2.normal[0] ) {
			return true;
		} else if ( normal[0] > op2.normal[0] ) {
			return false;
		} else if ( normal[1] < op2.normal[1] ) {
			return true;
		} else if ( normal[1] > op2.normal[1] ) {
			return false;
		} else if ( normal[2] < op2.normal[2] ) {
			return true;
		} else if ( normal[2] > op2.normal[2] ) {
			return false;
		} else if ( normal[3] < op2.normal[3] ) {
			return true;
		} else if ( normal[3] > op2.normal[3] ) {
			return false;
		} else if ( uv.x < op2.uv.x ) {
			return true;
		} else if ( uv.x > op2.uv.x ) {
			return false;
		} else if ( uv.y < op2.uv.y ) {
			return true;
		} else if ( uv.y > op2.uv.y ) {
			return false;
		} 

		return false;
	}
};

/**
 *	kbBoneMatrix_t
 */
struct kbBoneMatrix_t {

	void SetIdentity() {
		m_Axis[0].Set( 1.0f, 0.0f, 0.0f );
		m_Axis[1].Set( 0.0f, 1.0f, 0.0f );
		m_Axis[2].Set( 0.0f, 0.0f, 1.0f );
		m_Axis[3].Set( 0.0f, 0.0f, 0.0f );
	}

	const kbVec3 & GetAxis( const int axisIndex ) const { if ( axisIndex < 0 || axisIndex > 3 ) { kbError("Doh!"); } return m_Axis[axisIndex]; }
	const kbVec3 & GetOrigin() const { return m_Axis[3]; }
	void SetAxis( const int axisIndex, const kbVec3 & inVec ) { if ( axisIndex < 0 || axisIndex > 3 ) { kbError("Doh!"); } m_Axis[axisIndex] = inVec; }

	void TransposeUpper();

	void Invert();

	void operator*=( const kbBoneMatrix_t & op2 );
	void operator*=( const kbMat4 & op2 );

	kbVec3 m_Axis[4];
};

// platform switch here
#include <D3D11.h>

typedef ID3D11Buffer kbHWBuffer;
typedef ID3D11ShaderResourceView kbHWTexture;
typedef ID3D11VertexShader kbHWVertexShader;
typedef ID3D11GeometryShader kbHWGeometryShader;
typedef ID3D11PixelShader kbHWPixelShader;
typedef ID3D11InputLayout kbHWVertexLayout;

/**
 *	kbRenderJob
 */
class kbRenderJob : public kbJob {

//---------------------------------------------------------------------------------------------------
public:
												kbRenderJob() : m_bRequestShutdown( false ) { }

	void										Run();

	void										RequestShutdown() { m_bRequestShutdown = true; }

private:
	bool										m_bRequestShutdown;
};

/**
 *	kbShaderParamOverrides_t
 */
struct kbShaderParamOverrides_t {

    struct kbShaderParam_t {

        enum type {
            SHADER_MAT4,
            SHADER_VEC4,
			SHADER_MAT4_LIST,
			SHADER_VEC4_LIST,
            SHADER_TEX,
        } m_Type;

		std::vector<kbMat4> m_Mat4List;
		std::vector<kbVec4> m_Vec4List;

        kbShaderParam_t() : m_pTexture( nullptr ), m_pRenderTexture( nullptr ) { }
        const class kbTexture *			m_pTexture;
		const class kbRenderTexture *	m_pRenderTexture;
        std::string						m_VarName;
        size_t							m_VarSizeBytes;
    };

    kbShaderParamOverrides_t() { }

    std::vector<kbShaderParam_t> m_ParamOverrides;

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

    void SetMat4( const std::string & varName, const kbMat4 & newMat ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
		newParam.m_Mat4List.clear();
        newParam.m_Mat4List.push_back( newMat );
        newParam.m_Type = kbShaderParam_t::SHADER_MAT4;
        newParam.m_VarSizeBytes = sizeof(kbMat4);
    }

	void SetMat4List( const std::string & varName, const std::vector<kbMat4> & list ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_Mat4List = list;
        newParam.m_Type = kbShaderParam_t::SHADER_MAT4_LIST;
        newParam.m_VarSizeBytes = sizeof(kbVec4);
	}

    void SetVec4( const std::string & varName, const kbVec4 & newVec ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
		newParam.m_Vec4List.clear();
        newParam.m_Vec4List.push_back( newVec );
        newParam.m_Type = kbShaderParam_t::SHADER_VEC4;
        newParam.m_VarSizeBytes = sizeof(kbVec4);
    }

	void SetVec4List( const std::string & varName, const std::vector<kbVec4> & list ) {
        kbShaderParam_t & newParam = AllocateParam( varName );
        newParam.m_VarName = varName;
        newParam.m_Vec4List = list;
        newParam.m_Type = kbShaderParam_t::SHADER_VEC4_LIST;
        newParam.m_VarSizeBytes = sizeof(kbVec4);
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

/**
 *	kbRenderObject
 */
class kbRenderObject {

//---------------------------------------------------------------------------------------------------
public:
												kbRenderObject() : 
													m_pComponent( nullptr ),
													m_pModel( nullptr ),
													m_RenderPass( RP_Lighting ),
													m_RenderPassBucket( 0 ),
													m_EntityId( 0 ),
													m_CullDistance( -1.0f ),
													m_bCastsShadow( false ),
													m_bIsSkinnedModel( false ),
													m_bIsFirstAdd( false ),
													m_bIsRemove( false ) { }

	const class kbComponent *					m_pComponent;
	const class kbModel *						m_pModel;
	std::vector<class kbShader *>				m_OverrideShaderList;
	kbShaderParamOverrides_t                    m_ShaderParamOverrides;
	ERenderPass									m_RenderPass;
	int											m_RenderPassBucket;
	kbVec3										m_Position;
	kbQuat										m_Orientation;
	kbVec3										m_Scale;
	uint										m_EntityId;

	std::vector<kbBoneMatrix_t>					m_MatrixList;

	float										m_CullDistance;

	bool										m_bCastsShadow			: 1;
	bool										m_bIsSkinnedModel		: 1;

	// Updated by renderer
	bool										m_bIsFirstAdd			: 1;
	bool										m_bIsRemove				: 1;
};

/**
 *	kbRenderLight
 */
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
	kbVec3										m_Position;
	kbQuat										m_Orientation;
	kbVec4										m_Color;
	float										m_Radius;
	float										m_Length;
	float										m_CascadedShadowSplits[4];
	bool										m_bCastsShadow;
	bool										m_bIsFirstAdd;
	bool										m_bIsRemove;
};

/**
 *	eRenderObjectOp
 */
enum eRenderObjectOp {
	ROO_Add,
	ROO_Remove,
	ROO_Update,
};

/**
 *	kbLightShafts
 */
class kbLightShafts {

//---------------------------------------------------------------------------------------------------
public:
												kbLightShafts() :
													m_pLightShaftsComponent( nullptr ),
													m_pTexture( nullptr ),
													m_Color(0.0f, 0.0f, 0.0f, 1.0f ),
													m_Pos( kbVec3::zero ),
													m_Rotation( kbQuat::zero ),
													m_Width( 0.0f ),
													m_Height( 0.0f ),
													m_NumIterations( 0 ),
													m_IterationWidth( 0.0f ), 
													m_IterationHeight( 0.0f ),
													m_Operation( ROO_Add ),
													m_bIsDirectional( true ) {
														m_Rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
													}

	const class kbLightShaftsComponent *		m_pLightShaftsComponent;
	class kbTexture *							m_pTexture;
	kbColor										m_Color;
	kbVec3										m_Pos;
	kbQuat										m_Rotation;
	float										m_Width;
	float										m_Height;
	int											m_NumIterations;
	float										m_IterationWidth;
	float										m_IterationHeight;
	eRenderObjectOp								m_Operation;
	bool										m_bIsDirectional;
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

enum kbCullMode {
	CullMode_FrontFaces,
	CullMode_BackFaces,
	CullMode_None,
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

#endif