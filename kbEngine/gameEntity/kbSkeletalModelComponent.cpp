//==============================================================================
// kbSkeletalModelComponent.cpp
//
//
// 2016-2018 kbEngine 2.0
//==============================================================================
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(kbSkeletalModelComponent)

/**
 *	kbSkeletalMeshComponent
 */
void kbSkeletalModelComponent::Constructor() {
	m_pModel = nullptr;
	m_RenderObject.m_bIsSkinnedModel = true;
}

/**
 *	~kbSkeletalMeshComponent
 */
kbSkeletalModelComponent::~kbSkeletalModelComponent() {
}

/**
 *	kbSkeletalMeshComponent::EditorChange
 */
void kbSkeletalModelComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "Model" || propertyName == "ShaderOverride" ) {
		SetEnable_Internal( false );
		SetEnable_Internal( true );
	}
}

/**
 *	kbSkeletalModelComponent::SetEnable_Internal
 */
void kbSkeletalModelComponent::SetEnable_Internal( const bool isEnabled ) {
	
	if ( m_pModel == nullptr || g_pRenderer == nullptr ) {
		return;
	}

	m_RenderObject.m_pComponent = this;
	if ( isEnabled ) {

		m_RenderObject.m_bCastsShadow = this->GetCastsShadow();
		m_RenderObject.m_EntityId = GetOwner()->GetEntityId();
		m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
		m_RenderObject.m_pModel = m_pModel;
		m_RenderObject.m_Position = GetOwner()->GetPosition();
		m_RenderObject.m_RenderPass = m_RenderPass;
		m_RenderObject.m_Scale = GetOwner()->GetScale();
		m_RenderObject.m_OverrideShaderList = m_pOverrideShaderList;

		g_pRenderer->AddRenderObject( m_RenderObject );
	} else {
		g_pRenderer->RemoveRenderObject( m_RenderObject );
	}
}

/**
 *	kbSkeletalModelComponent:Update_Internal
 */
void kbSkeletalModelComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );
}

/**
 *	kbSkeletalModelComponent::GetBoneIndex
 */
int kbSkeletalModelComponent::GetBoneIndex( const kbString & boneName ) {
	if ( m_pModel == nullptr ) {
		return -1;
	}
	return m_pModel->GetBoneIndex( boneName );
}

/**
 *	kbSkeletalModelComponent::GetBoneRefMatrix
 */
kbBoneMatrix_t kbSkeletalModelComponent::GetBoneRefMatrix( int index ) {
	return m_pModel->GetRefBoneMatrix( index );
}

/**
 *	kbSkeletalModelComponent::GetBoneWorldPosition
 */
bool kbSkeletalModelComponent::GetBoneWorldPosition( const kbString & boneName, kbVec3 & outWorldPosition ) {
	const int boneIdx = GetBoneIndex( boneName );
	if ( boneIdx == -1 || boneIdx >= m_BindToLocalSpaceMatrices.size() ) {
		return false;
	}

	kbMat4 WeaponMatrix;
	GetOwner()->CalculateWorldMatrix( WeaponMatrix );

	const kbVec3 localPos = m_pModel->GetRefBoneMatrix( boneIdx ).GetOrigin() * m_BindToLocalSpaceMatrices[boneIdx];
	outWorldPosition = WeaponMatrix.TransformPoint( localPos );
	return true;
}

/**
 *	kbSkeletalModelComponent::GetBoneWorldMatrix
 */
bool kbSkeletalModelComponent::GetBoneWorldMatrix( const kbString & boneName, kbBoneMatrix_t & boneMatrix ) {
	const int boneIdx = GetBoneIndex( boneName );
	if ( boneIdx == -1 || boneIdx >= m_BindToLocalSpaceMatrices.size() ) {
		return false;
	}

	kbMat4 WeaponMatrix;
	GetOwner()->CalculateWorldMatrix( WeaponMatrix );

	boneMatrix = m_pModel->GetRefBoneMatrix( boneIdx ) * m_BindToLocalSpaceMatrices[boneIdx];
	boneMatrix *= WeaponMatrix;
	return true;
}