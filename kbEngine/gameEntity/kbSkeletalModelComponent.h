//==============================================================================
// kbSkeletalModelComponent.h
//
// 2016-2017 kbEngine 2.0
//==============================================================================
#ifndef _KBSKELETALMODELCOMPONENT_H_
#define _KBSKELETALMODELCOMPONENT_H_

#include "kbModel.h"

/**
 *	kbSkeletalModelComponent
 */
class kbSkeletalModelComponent : public kbModelComponent {

	KB_DECLARE_COMPONENT( kbSkeletalModelComponent, kbModelComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual										~kbSkeletalModelComponent();

	void										SetModel( class kbModel * pModel ) { m_pModel = pModel; }
	const kbModel *								GetModel() const { return m_pModel; }

	virtual void								EditorChange( const std::string & propertyName );

	int											GetBoneIndex( const kbString & boneName );
	kbBoneMatrix_t								GetBoneRefMatrix( const int index );

	bool										GetBoneWorldPosition( const kbString & boneName, kbVec3 & outWorldPosition );
	bool										GetBoneWorldMatrix( const kbString & boneName, kbBoneMatrix_t & boneMatrix );
	std::vector<kbBoneMatrix_t> &				GetFinalBoneMatrices() { return m_BindToLocalSpaceMatrices; }
	const std::vector<kbBoneMatrix_t> &			GetFinalBoneMatrices() const { return m_BindToLocalSpaceMatrices; }

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	class kbModel *								m_pModel;

	std::vector<kbBoneMatrix_t>					m_BindToLocalSpaceMatrices;
};

#endif