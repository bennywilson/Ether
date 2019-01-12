//===================================================================================================
// EtherSkelModel.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "EtherGame.h"
#include "EtherSkelModel.h"
#include "kbRenderer.h"

KB_DEFINE_COMPONENT(EtherAnimComponent)
KB_DEFINE_COMPONENT(EtherSkelModelComponent)

#define DEBUG_ANIMS 0

/**
 *	EtherAnimComponent::Constructor()
 */
void EtherAnimComponent::Constructor() {
	m_pAnimation = nullptr;
	m_TimeScale = 1.0f;
	m_bIsLooping = false;
	m_CurrentAnimationTime = -1.0f;
}

/**
 *	EtherSkelModelComponent::Constructor(
 */
void EtherSkelModelComponent::Constructor() {
	m_DebugAnimIdx = -1;
	m_bFirstPersonModel = false;
	m_CurrentAnimation = -1;
	m_NextAnimation = -1;
}

/**
 *	EtherSkelModelComponent::PlayAnimation
 */
void EtherSkelModelComponent::PlayAnimation( const kbString & AnimationName, const float BlendLength, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength ) {

#if DEBUG_ANIMS
	kbLog( "Attempting to play Animation %s ===================================================================", AnimationName.c_str() );
#endif

	if ( IsPlaying( AnimationName ) ) {
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
					kbLog( "	Anim all set up.  Next anim = %s.  Desired blend = %f", desiredNextAnimation.c_str() );
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
 *	EtherSkelModelComponent::IsPlaying
 */
bool EtherSkelModelComponent::IsPlaying( const kbString & AnimationName ) const {
	if ( m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_AnimationName == AnimationName ) {
		return true;
	}

	if ( m_CurrentAnimation != -1 && m_Animations[m_CurrentAnimation].m_AnimationName == AnimationName ) {
		return true;
	}

	return false;
}


/**
 *	EtherSkelModelComponent::SetEnable_Internal
 */
void EtherSkelModelComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );
}

/**
 *	EtherSkelModelComponent::SetModel
 */
void EtherSkelModelComponent::SetModel( kbModel *const pModel, bool bIsFirstPersonModel ) {
	m_bFirstPersonModel = bIsFirstPersonModel;
	Super::SetModel( pModel );
}

/**
 *	EtherSkelModelComponent::Update_Internal
 */
void EtherSkelModelComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_pModel != nullptr ) {
		if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
			m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
		}

		// Debug Animation
		if ( m_DebugAnimIdx >= 0 && m_DebugAnimIdx < m_Animations.size() && m_Animations[m_DebugAnimIdx].m_pAnimation != nullptr ) {
			if ( m_pModel != nullptr ) {
				static float time = 0.0f;
				static bool pause = false;

				if ( pause == false ) {
					const float AnimTimeScale = m_Animations[m_DebugAnimIdx].m_TimeScale;
					time += DeltaTime * AnimTimeScale;

					if ( m_Animations[m_DebugAnimIdx].m_bIsLooping == false ) {
						time = kbClamp( time, 0.0f, m_Animations[m_DebugAnimIdx].m_pAnimation->GetLengthInSeconds() );
					}
				}

				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}
				m_pModel->Animate( time, m_Animations[m_DebugAnimIdx].m_pAnimation, m_Animations[m_DebugAnimIdx].m_bIsLooping, m_BindToLocalSpaceMatrices );
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

			EtherAnimComponent & CurAnim = m_Animations[m_CurrentAnimation];

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
				CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;


				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}

#if DEBUG_ANIMS
				kbLog( "	Not blending anim %s. anim time = %f", CurAnim.m_AnimationName.c_str(), CurAnim.m_CurrentAnimationTime );
#endif

				m_pModel->Animate( CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, CurAnim.m_bIsLooping, m_BindToLocalSpaceMatrices );

				if ( bAnimIsFinished && CurAnim.m_DesiredNextAnimation.IsEmptyString() == false ) {

#if DEBUG_ANIMS
					kbLog( "	Cur Animation Done, going to %s - %f", CurAnim.m_DesiredNextAnimation.c_str(), CurAnim.m_DesiredNextAnimBlendLength );
#endif

					PlayAnimation( CurAnim.m_DesiredNextAnimation, CurAnim.m_DesiredNextAnimBlendLength );
				}
			} else {
				if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
					m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
				}

				if ( bAnimIsFinished == false ) {
					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;
				}

				EtherAnimComponent & NextAnim = m_Animations[m_NextAnimation];

				if ( CurAnim.m_bIsLooping && NextAnim.m_bIsLooping ) {
					// Sync the anims if they're both looping
					NextAnim.m_CurrentAnimationTime = CurAnim.m_CurrentAnimationTime;
				} else {
					NextAnim.m_CurrentAnimationTime += DeltaTime * NextAnim.m_TimeScale;
				}

				const float blendTime = kbClamp( ( g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime ) / m_BlendLength, 0.0f, 1.0f );
				m_pModel->BlendAnimations( CurAnim.m_pAnimation, CurAnim.m_CurrentAnimationTime, CurAnim.m_bIsLooping, NextAnim.m_pAnimation, NextAnim.m_CurrentAnimationTime, NextAnim.m_bIsLooping, blendTime, m_BindToLocalSpaceMatrices ); 

#if DEBUG_ANIMS
				kbLog( "	Blending anims %f.  %s cur time = %f. %s cur time is %f", blendTime, CurAnim.GetAnimationName().c_str(), CurAnim.m_CurrentAnimationTime, NextAnim.GetAnimationName().c_str(), NextAnim.m_CurrentAnimationTime );
#endif
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

	// Update collision component
	EtherSkelModelComponent *const pSkelModel = this;
	kbCollisionComponent *const pCollisionComponent = (kbCollisionComponent*)GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );

	if ( pCollisionComponent != nullptr && pSkelModel != nullptr ) {

		const std::vector<kbBoneCollisionSphere> & BindPoseCollisionSpheres = pCollisionComponent->GetLocalSpaceCollisionSpheres();

		// Update collision space world spheres
		if ( BindPoseCollisionSpheres.size() > 0 ) {

			for ( int iCollision = 0; iCollision < BindPoseCollisionSpheres.size(); iCollision++ ) {
				kbBoneMatrix_t jointMatrix;
				if ( pSkelModel->GetBoneWorldMatrix( BindPoseCollisionSpheres[iCollision].GetBoneName(), jointMatrix ) ) {
					kbVec4 newBoneWorldPos = BindPoseCollisionSpheres[iCollision].GetSphere().ToVec3() * jointMatrix;
					newBoneWorldPos.a = BindPoseCollisionSpheres[iCollision].GetSphere().a;
					pCollisionComponent->SetWorldSpaceCollisionSphere( iCollision, newBoneWorldPos );
				} else {
					kbError( "EtherAIComponent::Update_Internal() - Failed to get a world matrix for bone %s on model %s", BindPoseCollisionSpheres[iCollision].GetBoneName().c_str(), pSkelModel->GetModel()->GetName().c_str() );
				}
			}	
		}
	}
	// Temp: Search for "Additional Cloth Bones" and draw the hair locks for this AI using axial bill boards
	kbClothComponent * pClothComponent = nullptr;
	for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
		if ( GetOwner()->GetComponent( i )->IsA( kbClothComponent::GetType() ) ) {
			 pClothComponent = static_cast<kbClothComponent*>( GetOwner()->GetComponent( i ) );
			break;
		}
	}

	if ( pClothComponent != nullptr && pClothComponent->GetMasses().size() > 0 ) {
		
		const int startAdditionalBonedIdx = (int)pClothComponent->GetBoneInfo().size();
		const std::vector<kbClothMass_t> & ClothMasses = pClothComponent->GetMasses();

		for ( int iMass = startAdditionalBonedIdx; iMass < ClothMasses.size() - 1; iMass++ ) {
			if ( ClothMasses[iMass + 1].m_bAnchored ) {
				continue;
			}

			const kbVec3 midPt = ( ClothMasses[iMass].GetPosition() + ClothMasses[iMass+1].GetPosition() ) * 0.5f;
			const float distance = ( ClothMasses[iMass].GetPosition() - ClothMasses[iMass+1].GetPosition() ).Length();
			const kbVec3 direction = ( ClothMasses[iMass+1].GetPosition() - ClothMasses[iMass].GetPosition() ) / distance; 

			kbParticleManager::CustomParticleAtlasInfo_t ParticleInfo;
			ParticleInfo.m_Position = midPt;
			ParticleInfo.m_Direction = direction;
			ParticleInfo.m_Width = distance;
			ParticleInfo.m_Height = 4.0f;
			ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f, 1.0f );
			ParticleInfo.m_UVs[0].Set( 0.25f, 0.0f );
			ParticleInfo.m_UVs[1].Set( 0.25f + 0.125f, 0.125f );
			ParticleInfo.m_Type = BT_AxialBillboard;

			g_pGame->GetParticleManager()->AddQuad( 0, ParticleInfo );
		}
	}
}

/**
 *	EtherSkelModelComponent::GetCurAnimationName
 */
const kbString * EtherSkelModelComponent::GetCurAnimationName() const {
	if ( m_CurrentAnimation >= 0 && m_CurrentAnimation < m_Animations.size() ) {
		return &m_Animations[m_CurrentAnimation].GetAnimationName();
	}

	return nullptr;
}
