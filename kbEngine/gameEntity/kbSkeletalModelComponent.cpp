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
 *	kbAnimComponent::Constructor()
 */
void kbAnimComponent::Constructor() {
	m_pAnimation = nullptr;
	m_TimeScale = 1.0f;
	m_bIsLooping = false;
	m_CurrentAnimationTime = -1.0f;
}

/**
 *	kbSkeletalMeshComponent
 */
void kbSkeletalModelComponent::Constructor() {
	m_pModel = nullptr;
	m_RenderObject.m_bIsSkinnedModel = true;

	m_CurrentAnimation = -1;
	m_NextAnimation = -1;

	m_BlendStartTime = 0.0f;
	m_BlendLength = 1.0f;

	m_DebugAnimIdx = -1;
	m_DebugAnimTime = 0.0f;
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
		RefreshMaterials( true );
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
		RefreshMaterials( false );

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

	if ( m_pModel != nullptr ) {
		if ( m_BindToLocalSpaceMatrices.size() != m_pModel->NumBones() ) {
			m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
		}

		// Debug Animation
		if ( m_DebugAnimIdx >= 0 && m_DebugAnimIdx < m_Animations.size() && m_Animations[m_DebugAnimIdx].m_pAnimation != nullptr ) {
			if ( m_pModel != nullptr ) {

				static bool pause = false;
				if ( pause == false ) {
					const float AnimTimeScale = m_Animations[m_DebugAnimIdx].m_TimeScale;
					m_DebugAnimTime += DeltaTime * AnimTimeScale;

					if ( m_Animations[m_DebugAnimIdx].m_bIsLooping == false ) {
						m_DebugAnimTime = kbClamp( m_DebugAnimTime, 0.0f, m_Animations[m_DebugAnimIdx].m_pAnimation->GetLengthInSeconds() );
					}
				}

				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}
				m_pModel->Animate( m_BindToLocalSpaceMatrices, m_DebugAnimTime, m_Animations[m_DebugAnimIdx].m_pAnimation, m_Animations[m_DebugAnimIdx].m_bIsLooping );
			}
		} else {
			for ( int i = 0; i < m_pModel->NumBones(); i++ ) {
				m_BindToLocalSpaceMatrices[i].SetIdentity();
			}
		}

		if ( m_CurrentAnimation != -1 ) {
#if DEBUG_ANIMS
			kbLog( "Updating current anim %s", m_Animations[m_CurrentAnimation].GetAnimationName().c_str() );
#endif
			// Check if the blend is finished
			if ( m_NextAnimation != -1 ) {
				const float blendTime = ( g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime ) / m_BlendLength;

#if DEBUG_ANIMS
				kbLog( "	Checking if blend is finished.  Blend time is %f", blendTime );
#endif
				if ( blendTime >= 1.0f ) {
					m_CurrentAnimation = m_NextAnimation;
					m_NextAnimation = -1;

#if DEBUG_ANIMS
					kbLog( "	%s Transition to Next Animation", GetOwner()->GetName().c_str() );
#endif
				}
			}

			kbAnimComponent & CurAnim = m_Animations[m_CurrentAnimation];

			bool bAnimIsFinished = false;

			if ( CurAnim.m_bIsLooping == false ) {
				if ( CurAnim.m_CurrentAnimationTime >= CurAnim.m_pAnimation->GetLengthInSeconds() ) {

#if DEBUG_ANIMS
				kbLog( "	Cur anim is finished!" );
#endif
					CurAnim.m_CurrentAnimationTime = CurAnim.m_pAnimation->GetLengthInSeconds();
					bAnimIsFinished = true;
				}
			}

			if ( m_NextAnimation == -1 ) {
				const float prevAnimTime = CurAnim.m_CurrentAnimationTime;
	
				CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;

				for ( int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++ ) {
					auto & curEvent = CurAnim.m_AnimEvents[iAnimEvent];
					if ( curEvent.GetEventTime() > prevAnimTime && curEvent.GetEventTime() <= CurAnim.m_CurrentAnimationTime  ) {
						kbLog( "AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime() );
					}
				}

				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}

#if DEBUG_ANIMS
				kbLog( "	Not blending anim %s. anim time = %f", CurAnim.m_AnimationName.c_str(), CurAnim.m_CurrentAnimationTime );
#endif

				m_pModel->Animate( m_BindToLocalSpaceMatrices, CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, CurAnim.m_bIsLooping );

				if ( bAnimIsFinished && CurAnim.m_DesiredNextAnimation.IsEmptyString() == false ) {

#if DEBUG_ANIMS
					kbLog( "	Cur Animation Done, going to %s - %f", CurAnim.m_DesiredNextAnimation.c_str(), CurAnim.m_DesiredNextAnimBlendLength );
#endif

					PlayAnimation( CurAnim.m_DesiredNextAnimation, CurAnim.m_DesiredNextAnimBlendLength, true );
				}
			} else {
				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}

				if ( bAnimIsFinished == false ) {
					const float prevAnimTime = CurAnim.m_CurrentAnimationTime;
	
					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;

					for ( int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++ ) {
						auto & curEvent = CurAnim.m_AnimEvents[iAnimEvent];
						if ( curEvent.GetEventTime() > prevAnimTime && curEvent.GetEventTime() <= CurAnim.m_CurrentAnimationTime  ) {
							//kbLog( "AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime() );
						}
					}
					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;
				}

				kbAnimComponent & NextAnim = m_Animations[m_NextAnimation];
				const float prevNextAnimTime = NextAnim.m_CurrentAnimationTime;
				if ( CurAnim.m_bIsLooping && NextAnim.m_bIsLooping ) {
					// Sync the anims if they're both looping
					NextAnim.m_CurrentAnimationTime = CurAnim.m_CurrentAnimationTime;
				} else {
					NextAnim.m_CurrentAnimationTime += DeltaTime * NextAnim.m_TimeScale;
				}

				for ( int iAnimEvent = 0; iAnimEvent < NextAnim.m_AnimEvents.size(); iAnimEvent++ ) {
					auto & curEvent = NextAnim.m_AnimEvents[iAnimEvent];
					if ( curEvent.GetEventTime() > prevNextAnimTime && curEvent.GetEventTime() <= NextAnim.m_CurrentAnimationTime  ) {
						//kbLog( "AnimEvent %s - %f", curEvent.GetEventName().c_str(), curEvent.GetEventTime() );
					}
				}

				const float blendTime = kbClamp( ( g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime ) / m_BlendLength, 0.0f, 1.0f );
				m_pModel->BlendAnimations( m_BindToLocalSpaceMatrices, CurAnim.m_pAnimation, CurAnim.m_CurrentAnimationTime, CurAnim.m_bIsLooping, NextAnim.m_pAnimation, NextAnim.m_CurrentAnimationTime, NextAnim.m_bIsLooping, blendTime ); 

#if DEBUG_ANIMS
				kbLog( "	Blending anims %f.  %s cur time = %f. %s cur time is %f", blendTime, CurAnim.GetAnimationName().c_str(), CurAnim.m_CurrentAnimationTime, NextAnim.GetAnimationName().c_str(), NextAnim.m_CurrentAnimationTime );
#endif
			}
		}
	}

	m_RenderObject.m_pComponent = this;
	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Scale = GetOwner()->GetScale();
	m_RenderObject.m_pModel = m_pModel;
	m_RenderObject.m_RenderPass = m_RenderPass;
	g_pRenderer->UpdateRenderObject( m_RenderObject );
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

	kbMat4 worldMatrix;
	GetOwner()->CalculateWorldMatrix( worldMatrix );

	const kbVec3 localPos = m_pModel->GetRefBoneMatrix( boneIdx ).GetOrigin() * m_BindToLocalSpaceMatrices[boneIdx];
	outWorldPosition = worldMatrix.TransformPoint( localPos );
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

/**
 *	kbSkeletalModelComponent::PlayAnimation
 */
void kbSkeletalModelComponent::PlayAnimation( const kbString & AnimationName, const float BlendLength, const bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength ) {

#if DEBUG_ANIMS
	kbLog( "Attempting to play Animation %s ===================================================================", AnimationName.c_str() );
#endif

	if ( bRestartIfAlreadyPlaying == false && IsPlaying( AnimationName ) ) {
		return;
	}

	const std::string & animName = AnimationName.stl_str();
	for ( int i = 0; i < m_Animations.size(); i++ ) {
		const std::string & CurName = m_Animations[i].m_AnimationName.stl_str();
		if ( m_Animations[i].m_AnimationName == AnimationName ) {
			#if DEBUG_ANIMS
				kbLog( "	Found desired animation" );
			#endif

			if ( BlendLength <= 0.0f || m_CurrentAnimation == -1 ) {

				#if DEBUG_ANIMS
						kbLog( "	Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation );
				#endif

				// Stop previous animation
				if ( m_CurrentAnimation != -1 && m_CurrentAnimation != i ) {
					#if DEBUG_ANIMS
						kbLog( "	Stopping Animation %s", m_Animations[m_CurrentAnimation].GetAnimationName().c_str() );
					#endif

					m_Animations[m_CurrentAnimation].m_CurrentAnimationTime = -1;
				}

				if ( m_NextAnimation != -1 && m_NextAnimation != i ) {

					#if DEBUG_ANIMS
						kbLog( "	Canceling next animation %s", m_Animations[m_NextAnimation].GetAnimationName().c_str() );
					#endif

					m_Animations[m_NextAnimation].m_CurrentAnimationTime = -1;
				}
				m_NextAnimation = -1;

				// Start current animation
				if ( m_CurrentAnimation != i ) {
					m_Animations[i].m_CurrentAnimationTime = 0.0f;
				}
				m_CurrentAnimation = i;

				#if DEBUG_ANIMS
					kbLog( "	Anim all set up.  Next anim = %s.  Desired blend length = %f", desiredNextAnimation.c_str() ,desiredNextAnimationBlendLength );
				#endif

				m_Animations[m_CurrentAnimation].m_DesiredNextAnimation = desiredNextAnimation;
				m_Animations[m_CurrentAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
			} else {
				m_BlendStartTime = g_GlobalTimer.TimeElapsedSeconds();
				m_BlendLength = BlendLength;
				m_NextAnimation = i;

				m_Animations[m_NextAnimation].m_DesiredNextAnimation = desiredNextAnimation;
				m_Animations[m_NextAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
				m_Animations[m_NextAnimation].m_CurrentAnimationTime = 0.0f;


				#if DEBUG_ANIMS
					kbLog( "	Blending this animation in %f %d.  Desired next = %s, desired len = %f ", m_BlendLength, m_NextAnimation, desiredNextAnimation.c_str(), desiredNextAnimationBlendLength );
				#endif
			}

			break;
		}
	}
}

/**
 *	kbSkeletalModelComponent::IsPlaying
 */
bool kbSkeletalModelComponent::IsPlaying( const kbString & AnimationName ) const {
	if ( m_Animations.size() == 0 ) {
		return false;
	}

	if ( m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_AnimationName == AnimationName ) {
		return true;
	}

	if ( m_CurrentAnimation != -1 && m_Animations[m_CurrentAnimation].m_AnimationName == AnimationName ) {
		return true;
	}

	return false;
}

/**
 *	kbSkeletalModelComponent::SetModel
 */
void kbSkeletalModelComponent::SetModel( kbModel *const pModel ) {

	m_BindToLocalSpaceMatrices.clear();
	m_RenderObject.m_pModel = pModel;
	g_pRenderer->UpdateRenderObject( m_RenderObject );
}


/**
 *	kbSkeletalModelComponent::GetCurAnimationName
 */
const kbString * kbSkeletalModelComponent::GetCurAnimationName() const {
	if ( m_CurrentAnimation >= 0 && m_CurrentAnimation < m_Animations.size() ) {
		return &m_Animations[m_CurrentAnimation].GetAnimationName();
	}

	return nullptr;
}