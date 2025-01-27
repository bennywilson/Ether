//===================================================================================================
// DQuadSkelModel.cpp
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#include "kbNetworkingManager.h"
#include "DQuadGame.h"
#include "DQuadSkelModel.h"

KB_DEFINE_COMPONENT(kbDQuadAnimComponent)
KB_DEFINE_COMPONENT(kbDQuadSkelModelComponent)

/**
 *	kbDQuadAnimComponent::Constructor()
 */
void kbDQuadAnimComponent::Constructor() {
	m_pAnimation = NULL;
	m_TimeScale = 1.0f;
	m_bIsLooping = false;
	m_CurrentAnimationTime = -1.0f;
}

/**
 *	kbDQuadSkelModelComponent::Constructor(
 */
void kbDQuadSkelModelComponent::Constructor() {
	m_DebugAnimIdx = -1;
	m_bFirstPersonModel = false;
}

/**
 *	kbDQuadSkelModelComponent::PlayAnimation
 */
void kbDQuadSkelModelComponent::PlayAnimation( const kbString & AnimationName, const bool bStopAllOtherAnimations ) {
	const std::string & animName = AnimationName.stl_str();
	for ( int i = 0; i < m_Animations.size(); i++ ) {
		const std::string & CurName = m_Animations[i].m_AnimationName.stl_str();
		if ( m_Animations[i].m_AnimationName == AnimationName ) {
			for ( int iAnims = (int)m_CurrentlyPlayingAnimations.size() - 1; iAnims >= 0; iAnims-- ) {
				if ( m_CurrentlyPlayingAnimations[iAnims] == i ) {
					m_Animations[m_CurrentlyPlayingAnimations[iAnims]].m_CurrentAnimationTime = 0.0f;
				} else if ( bStopAllOtherAnimations ) {

					std::swap( m_CurrentlyPlayingAnimations[iAnims], m_CurrentlyPlayingAnimations.back() );
					m_CurrentlyPlayingAnimations.pop_back();
				}
			}

			m_CurrentlyPlayingAnimations.push_back( i );
			m_Animations[i].m_CurrentAnimationTime = 0.0f;
			break;
		}
	}
}

/**
 *	kbDQuadSkelModelComponent::IsPlaying
 */
bool kbDQuadSkelModelComponent::IsPlaying( const kbString & AnimationName ) const {
	for ( int i = 0; i < m_CurrentlyPlayingAnimations.size(); i++ ) {
		const int animIdx = m_CurrentlyPlayingAnimations[i];
		if ( m_Animations[animIdx].m_AnimationName == AnimationName && m_Animations[animIdx].m_CurrentAnimationTime >= 0.0f ) {
			return true;
		}
	}

	return false;
}


/**
 *	kbDQuadSkelModelComponent::SetEnable_Internal
 */
void kbDQuadSkelModelComponent::SetEnable_Internal( const bool isEnabled ) {
	Super::SetEnable_Internal( isEnabled );
}

/**
 *	kbDQuadSkelModelComponent::SetModel
 */
void kbDQuadSkelModelComponent::SetModel( kbModel *const pModel, bool bIsFirstPersonModel ) {
	m_bFirstPersonModel = bIsFirstPersonModel;
	Super::SetModel( pModel );
}

/**
 *	kbDQuadSkelModelComponent::Update
 */
void kbDQuadSkelModelComponent::Update( const float DeltaTime ) {
	Super::Update( DeltaTime );

	if ( kbNetworkingManager::GetHostType() == HOST_SERVER ) {
		return;
	}

	if ( m_pModel != NULL ) {
		if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
			m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
		}

		if ( m_DebugAnimIdx >= 0 && m_DebugAnimIdx < m_Animations.size() && m_Animations[m_DebugAnimIdx].m_pAnimation != NULL ) {
			if ( m_pModel != NULL ) {
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
				m_pModel->SetBoneMatrices( time, m_Animations[m_DebugAnimIdx].m_pAnimation, m_BindToLocalSpaceMatrices );
			}
		} else {
			for ( int i = 0; i < m_pModel->NumBones(); i++ ) {
				m_BindToLocalSpaceMatrices[i].SetIdentity();
			}
		}

		for ( int i = (int)m_CurrentlyPlayingAnimations.size() - 1; i >= 0; i-- ) {
			const int currentAnimIdx = m_CurrentlyPlayingAnimations[i];
			kbDQuadAnimComponent & CurAnim = m_Animations[currentAnimIdx];
			CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale;
			bool bAnimIsFinished = false;

			if ( CurAnim.m_bIsLooping == false ) {
				if ( CurAnim.m_CurrentAnimationTime >= CurAnim.m_pAnimation->GetLengthInSeconds() ) {
					CurAnim.m_CurrentAnimationTime = CurAnim.m_pAnimation->GetLengthInSeconds();
					bAnimIsFinished = true;
				}
			}

			if ( m_BindToLocalSpaceMatrices.size() == 0 ) {
				m_BindToLocalSpaceMatrices.resize( m_pModel->NumBones() );
			}
			m_pModel->SetBoneMatrices( CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, m_BindToLocalSpaceMatrices );

			if ( bAnimIsFinished ) {
				CurAnim.m_CurrentAnimationTime = -1.0f;
				m_CurrentlyPlayingAnimations.erase( m_CurrentlyPlayingAnimations.begin() + i );
			}
		}

		if ( g_pRenderer->UsingHMD() && IsFirstPersonModel() ) {
			// Glue first person models to the hmd
			DQuadGame *const pGame = static_cast<DQuadGame*>( g_pGame );

			kbMat4 tempMat = pGame->GetCamera().m_EyeMats[0];
			tempMat.InvertFast();

			const kbQuat rot = kbQuatFromMatrix( tempMat );
			m_pParent->SetPosition( tempMat[3].ToVec3() );
			m_pParent->SetOrientation( rot );

			g_pRenderer->UpdateRenderObject( this, tempMat[3].ToVec3(), rot, m_pParent->GetScale() );
		} else {
			g_pRenderer->UpdateRenderObject( this, m_pParent->GetPosition(), m_pParent->GetOrientation(), m_pParent->GetScale() );
		}
	}


	// Temp: Search for "Additional Cloth Bones" and draw the hair locks for this AI using axial bill boards
	kbClothComponent * pClothComponent = NULL;
	for ( int i = 0; i < m_pParent->NumComponents(); i++ ) {
		if ( m_pParent->GetComponent( i )->IsA( kbClothComponent::GetType() ) ) {
			 pClothComponent = static_cast<kbClothComponent*>( m_pParent->GetComponent( i ) );
			break;
		}
	}

	if ( pClothComponent != NULL && pClothComponent->GetMasses().size() > 0 ) {
		
		const int startAdditionalBonedIdx = (int)pClothComponent->GetBoneInfo().size();
		const std::vector<kbClothMass_t> & ClothMasses = pClothComponent->GetMasses();

		for ( int iMass = startAdditionalBonedIdx; iMass < ClothMasses.size() - 1; iMass++ ) {
			if ( ClothMasses[iMass + 1].m_bAnchored ) {
				continue;
			}

			const kbVec3 midPt = ( ClothMasses[iMass].GetPosition() + ClothMasses[iMass+1].GetPosition() ) * 0.5f;
			const float distance = ( ClothMasses[iMass].GetPosition() - ClothMasses[iMass+1].GetPosition() ).Length();
			const kbVec3 direction = ( ClothMasses[iMass+1].GetPosition() - ClothMasses[iMass].GetPosition() ) / distance; 

			kbParticleManager::CustomParticleInfo_t ParticleInfo;
			ParticleInfo.m_Position = midPt;
			ParticleInfo.m_Direction = direction;
			ParticleInfo.m_Width = distance;
			ParticleInfo.m_Height = 4.0f;
			ParticleInfo.m_Color.Set( 1.0f, 1.0f, 1.0f );
			ParticleInfo.m_UVs[0].Set( 0.25f, 0.0f );
			ParticleInfo.m_UVs[1].Set( 0.25f + 0.125f, 0.125f );
			ParticleInfo.m_Type = kbParticleManager::CPT_AxialBillboard;

			g_pGame->GetParticleManager()->AddQuad( ParticleInfo );
		}
	}
}