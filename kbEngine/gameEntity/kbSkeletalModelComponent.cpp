//==============================================================================
// kbSkeletalModelComponent.cpp
//
//
// 2016-2019 kbEngine 2.0
//==============================================================================
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbRenderer.h"
#include "DX11/kbRenderer_DX11.h"			// HACK

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

	m_pSyncParent = nullptr;

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
		m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
		RefreshMaterials( false );

		g_pRenderer->AddRenderObject( m_RenderObject );

		m_AnimationTimeScaleMultipliers.resize( m_Animations.size() );
		for ( int i = 0; i < m_AnimationTimeScaleMultipliers.size(); i++ ) {
			m_AnimationTimeScaleMultipliers[i] = 1.0f;
		}
	} else {
		g_pRenderer->RemoveRenderObject( m_RenderObject );
	}
}

/**
 *	kbSkeletalModelComponent:Update_Internal
 */
void kbSkeletalModelComponent::Update_Internal( const float DeltaTime ) {
	Super::Update_Internal( DeltaTime );

	if ( m_pModel != nullptr && m_pSyncParent == nullptr ) {
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
			bool bOutput = true;
			if ( m_Animations[m_CurrentAnimation].GetAnimationName().stl_str().find( "Shoot" ) != std::string::npos ) {
				bOutput = true;
			}
			if ( bOutput ) kbLog( "Updating current anim %s with idx %d", m_Animations[m_CurrentAnimation].GetAnimationName().c_str(), m_CurrentAnimation );
#endif
			// Check if the blend is finished
			if ( m_NextAnimation != -1 ) {
				const float blendTime = ( g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime ) / m_BlendLength;

#if DEBUG_ANIMS
				if ( bOutput ) kbLog( "	Checking if blend is finished.  Blend time is %f", blendTime );
#endif
				if ( blendTime >= 1.0f ) {
					m_CurrentAnimation = m_NextAnimation;
					m_NextAnimation = -1;

#if DEBUG_ANIMS
					if ( bOutput ) kbLog( "	%s Transition to Next Animation", GetOwner()->GetName().c_str() );
#endif
				}
			}

			kbAnimComponent & CurAnim = m_Animations[m_CurrentAnimation];

			bool bAnimIsFinished = false;
			const float curAnimLenSec = CurAnim.m_pAnimation->GetLengthInSeconds();
			const float prevAnimTime = CurAnim.m_CurrentAnimationTime;

			bool bOutput = false;
			if ( CurAnim.m_bIsLooping && CurAnim.m_AnimEvents.size() > 0 ) {
				bOutput = true;
			}

			if ( CurAnim.m_bIsLooping == false ) {
				if ( CurAnim.m_CurrentAnimationTime >= curAnimLenSec ) {

#if DEBUG_ANIMS
				if ( bOutput ) kbLog( "	Cur anim is finished!" );
#endif
					//CurAnim.m_CurrentAnimationTime = curAnimLenSec;
					bAnimIsFinished = true;
				}
			}

			if ( m_NextAnimation == -1 ) {

				CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];

				if ( CurAnim.m_bIsLooping ) {
					CurAnim.m_CurrentAnimationTime = fmod( CurAnim.m_CurrentAnimationTime, curAnimLenSec );
				}

				const float curAnimTime = CurAnim.m_CurrentAnimationTime;
#if DEBUG_ANIMS
				if ( bOutput ) { kbLog( "		prevAnimTime = %f - Cur anim time = %f.  DeltaT and all that was %f", prevAnimTime, CurAnim.m_CurrentAnimationTime ); }
#endif

				for ( int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++ ) {
					auto & curEvent = CurAnim.m_AnimEvents[iAnimEvent];
					const float animEventTime = curEvent.GetEventTime();

					if ( ( animEventTime > prevAnimTime && animEventTime <= curAnimTime ) ||
						 ( prevAnimTime > curAnimTime && animEventTime < curAnimTime ) ) {

						if ( ( prevAnimTime > curAnimTime && animEventTime < curAnimTime ) ) {
						//	kbLog( "	pat = %f curAnimTime = %f animEventTime = %f --> %s. DT = %f", prevAnimTime, curAnimTime, animEventTime, curEvent.GetEventName().c_str(), DeltaTIme );
						}
						const kbAnimEventInfo_t animEventInfo( curEvent, this );
						for ( int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++ ) {
							IAnimEventListener *const pCurListener = m_AnimEventListeners[iListener];
							m_AnimEventListeners[iListener]->OnAnimEvent( animEventInfo );
						}
					}
				}

#if DEBUG_ANIMS
				if ( bOutput ) kbLog( "	Not blending anim %s. anim time = %f", CurAnim.m_AnimationName.c_str(), CurAnim.m_CurrentAnimationTime );
#endif

				m_pModel->Animate( m_BindToLocalSpaceMatrices, CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, CurAnim.m_bIsLooping );

				if ( bAnimIsFinished && CurAnim.m_DesiredNextAnimation.IsEmptyString() == false ) {

#if DEBUG_ANIMS
					if ( bOutput ) kbLog( "	Cur Animation Done, going to %s - %f", CurAnim.m_DesiredNextAnimation.c_str(), CurAnim.m_DesiredNextAnimBlendLength );
#endif

					PlayAnimation( CurAnim.m_DesiredNextAnimation, CurAnim.m_DesiredNextAnimBlendLength, true );
				}
			} else {

				if ( bAnimIsFinished == false ) {
					const float prevAnimTime = fmod( CurAnim.m_CurrentAnimationTime, curAnimLenSec );
	
					CurAnim.m_CurrentAnimationTime += DeltaTime * CurAnim.m_TimeScale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];
					if ( CurAnim.m_bIsLooping ) {
						CurAnim.m_CurrentAnimationTime = fmod( CurAnim.m_CurrentAnimationTime, curAnimLenSec );
					}

					for ( int iAnimEvent = 0; iAnimEvent < CurAnim.m_AnimEvents.size(); iAnimEvent++ ) {
						auto & curEvent = CurAnim.m_AnimEvents[iAnimEvent];
						const float animEventTime = curEvent.GetEventTime() * CurAnim.m_TimeScale * m_AnimationTimeScaleMultipliers[m_CurrentAnimation];

						if ( ( animEventTime > prevAnimTime && animEventTime <= CurAnim.m_CurrentAnimationTime ) ||
							 ( prevAnimTime > CurAnim.m_CurrentAnimationTime && animEventTime < CurAnim.m_CurrentAnimationTime  ) ) {

							const kbAnimEventInfo_t animEventInfo( curEvent, this );
							for ( int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++ ) {
								IAnimEventListener *const pCurListener = m_AnimEventListeners[iListener];	
								m_AnimEventListeners[iListener]->OnAnimEvent( animEventInfo );
							}
						}
					}
				}

				kbAnimComponent & NextAnim = m_Animations[m_NextAnimation];
				const float nextAnimLenSec = NextAnim.m_pAnimation->GetLengthInSeconds();
				const float prevNextAnimTime = NextAnim.m_CurrentAnimationTime;

#if DEBUG_ANIMS
				if ( bOutput ) { kbLog( "		Cur anim is %s.  time = %f.  DeltaT was %f.  Next anim is %s.  Next anim time is %f", CurAnim.m_AnimationName.c_str(), CurAnim.m_CurrentAnimationTime, DeltaTime * CurAnim.m_TimeScale, NextAnim.m_AnimationName.c_str(), NextAnim.m_CurrentAnimationTime ); }
#endif

				if ( CurAnim.m_bIsLooping && NextAnim.m_bIsLooping ) {
					// Sync the anims if they're both looping
					NextAnim.m_CurrentAnimationTime = CurAnim.m_CurrentAnimationTime;
				} else {
					NextAnim.m_CurrentAnimationTime += DeltaTime * NextAnim.m_TimeScale * m_AnimationTimeScaleMultipliers[m_NextAnimation];
				}

				if ( NextAnim.m_bIsLooping ) {
					NextAnim.m_CurrentAnimationTime = fmod( NextAnim.m_CurrentAnimationTime, nextAnimLenSec );
				}

				for ( int iAnimEvent = 0; iAnimEvent < NextAnim.m_AnimEvents.size(); iAnimEvent++ ) {
					auto & curEvent = NextAnim.m_AnimEvents[iAnimEvent];

					const float animEventTime = curEvent.GetEventTime() * m_AnimationTimeScaleMultipliers[m_NextAnimation];

					if ( ( animEventTime > prevNextAnimTime && animEventTime <= NextAnim.m_CurrentAnimationTime ) ||
						 ( prevNextAnimTime > NextAnim.m_CurrentAnimationTime && animEventTime < NextAnim.m_CurrentAnimationTime ) ) {

						const kbAnimEventInfo_t animEventInfo( curEvent, this );
						for ( int iListener = 0; iListener < m_AnimEventListeners.size(); iListener++ ) {
							IAnimEventListener *const pCurListener = m_AnimEventListeners[iListener];	
							m_AnimEventListeners[iListener]->OnAnimEvent( animEventInfo );
						}
					}
				}

				const float blendTime = kbClamp( ( g_GlobalTimer.TimeElapsedSeconds() - m_BlendStartTime ) / m_BlendLength, 0.0f, 1.0f );
				m_pModel->BlendAnimations( m_BindToLocalSpaceMatrices, CurAnim.m_pAnimation, CurAnim.m_CurrentAnimationTime, CurAnim.m_bIsLooping, NextAnim.m_pAnimation, NextAnim.m_CurrentAnimationTime, NextAnim.m_bIsLooping, blendTime ); 

#if DEBUG_ANIMS
				if ( bOutput ) kbLog( "	Blending anims %f.  %s cur time = %f. %s cur time is %f", blendTime, CurAnim.GetAnimationName().c_str(), CurAnim.m_CurrentAnimationTime, NextAnim.GetAnimationName().c_str(), NextAnim.m_CurrentAnimationTime );
#endif
			}
		}
	}

	m_RenderObject.m_pComponent = this;
	m_RenderObject.m_Position = GetOwner()->GetPosition();
	m_RenderObject.m_Orientation = GetOwner()->GetOrientation();
	m_RenderObject.m_Scale = GetOwner()->GetScale() * kbLevelComponent::GetGlobalModelScale();
	m_RenderObject.m_pModel = m_pModel;
	m_RenderObject.m_RenderPass = m_RenderPass;
	g_pRenderer->UpdateRenderObject( m_RenderObject );

	for ( int i = 0; i < m_SyncedSkelModels.size(); i++ ) {
		m_SyncedSkelModels[i]->m_BindToLocalSpaceMatrices = m_BindToLocalSpaceMatrices;
	}
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
 *	kbSkeletalModelComponent::SetAnimationTimeScaleMultiplier
 */
void kbSkeletalModelComponent::SetAnimationTimeScaleMultiplier( const kbString & animName, const float factor ) {

	for ( int i = 0; i < m_Animations.size(); i++ ) {
		const kbAnimComponent & anim = m_Animations[i];
		if ( anim.m_AnimationName == animName ) {
			m_AnimationTimeScaleMultipliers[i] = factor;
			return;
		}
	}
}


/**
 *	kbSkeletalModelComponent::PlayAnimation
 */
void kbSkeletalModelComponent::PlayAnimation( const kbString & AnimationName, const float BlendLength, const bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength ) {

#if DEBUG_ANIMS
	bool bOutput = true;
	if ( bOutput ) kbLog( "Attempting to play Animation %s ===================================================================", AnimationName.c_str() );
#endif

	if ( bRestartIfAlreadyPlaying == false && IsPlaying( AnimationName ) ) {

		if ( m_NextAnimation != -1 && m_Animations[m_NextAnimation].m_AnimationName != AnimationName ) {
			m_Animations[m_NextAnimation].m_CurrentAnimationTime = -1;
			m_NextAnimation = -1;
		}
		#if DEBUG_ANIMS
			if ( bOutput ) kbLog( "		Returning as it's already playing" );
		#endif
		return;
	}

	const std::string & animName = AnimationName.stl_str();
	for ( int i = 0; i < m_Animations.size(); i++ ) {

		if ( m_Animations[i].m_AnimationName != AnimationName ) {
			continue;
		}

		#if DEBUG_ANIMS
			if ( bOutput ) kbLog( "		Found desired animation" );
		#endif

		if ( BlendLength <= 0.0f || m_CurrentAnimation == -1 ) {

			#if DEBUG_ANIMS
					if ( bOutput ) kbLog( "		Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation );
			#endif

			// Stop previous animation
			if ( m_CurrentAnimation != -1 && m_CurrentAnimation != i ) {
				#if DEBUG_ANIMS
					if ( bOutput ) kbLog( "		Stopping Animation %s", m_Animations[m_CurrentAnimation].GetAnimationName().c_str() );
				#endif

				m_Animations[m_CurrentAnimation].m_CurrentAnimationTime = -1;
			}

			if ( m_NextAnimation != -1 && m_NextAnimation != i ) {

				#if DEBUG_ANIMS
					if ( bOutput )kbLog( "		Canceling next animation %s", m_Animations[m_NextAnimation].GetAnimationName().c_str() );
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
				if ( bOutput )kbLog( "		Anim all set up.  Next anim = %s.  Desired blend length = %f.  Starting animation time is %f", desiredNextAnimation.c_str(), desiredNextAnimationBlendLength, m_Animations[i].m_CurrentAnimationTime );
			#endif

			m_Animations[m_CurrentAnimation].m_DesiredNextAnimation = desiredNextAnimation;
			m_Animations[m_CurrentAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
		} else {


			#if DEBUG_ANIMS
				if ( bOutput ) {
					if ( m_CurrentAnimation != -1 ) {
						kbLog( "		Cur anim is %s.  Anim time is %f", m_Animations[m_CurrentAnimation].GetAnimationName().c_str(), m_Animations[m_CurrentAnimation].m_CurrentAnimationTime );
					}
					
				}// kbLog( "	Starting this animation immediately.  Blend length is %f, currentanimation is %d", BlendLength, m_CurrentAnimation );
			#endif

			m_BlendStartTime = g_GlobalTimer.TimeElapsedSeconds();
			m_BlendLength = BlendLength;
			m_NextAnimation = i;

			m_Animations[m_NextAnimation].m_DesiredNextAnimation = desiredNextAnimation;
			m_Animations[m_NextAnimation].m_DesiredNextAnimBlendLength = desiredNextAnimationBlendLength;
			m_Animations[m_NextAnimation].m_CurrentAnimationTime = 0.0f;


			#if DEBUG_ANIMS
				if ( bOutput ) kbLog( "		Blend Len = %f.  Next Anim Idx = %d.  Next Anim Name = %s.  Next Anim Len = %f. ", m_BlendLength, m_NextAnimation, desiredNextAnimation.c_str(), desiredNextAnimationBlendLength );
			#endif
		}

		break;
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

		const kbAnimComponent & anim = m_Animations[m_CurrentAnimation];
		if ( anim.m_bIsLooping == true || anim.m_CurrentAnimationTime <= anim.m_pAnimation->GetLengthInSeconds() ) {
			return true;
		}
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

/**
 *	kbSkeletalModelComponent::GetNextAnimationName
 */
const kbString * kbSkeletalModelComponent::GetNextAnimationName() const {
	if ( m_NextAnimation >= 0 && m_NextAnimation < m_Animations.size() ) {
		return &m_Animations[m_NextAnimation].GetAnimationName();
	}

	return nullptr;
}

/**
 *	kbSkeletalModelComponent::RegisterAnimEventListener
 */
void kbSkeletalModelComponent::RegisterAnimEventListener( IAnimEventListener *const pListener ) {

	kbErrorCheck( VectorContains( m_AnimEventListeners, pListener ) == false, "RegisterAnimEventListener() - Duplicate entries");
	m_AnimEventListeners.push_back( pListener );
}

/**
 *	kbSkeletalModelComponent::UnregisterAnimEventListener
 */
void kbSkeletalModelComponent::UnregisterAnimEventListener( IAnimEventListener *const pListener ) {

	kbErrorCheck( VectorContains( m_AnimEventListeners, pListener ) == true, "UnregisterAnimEventListener() - Listener not previously registered");
	VectorRemoveFast( m_AnimEventListeners, pListener );
}

/**
 *	kbSkeletalModelComponent::RegisterSyncSkelModel
 */
void kbSkeletalModelComponent::RegisterSyncSkelModel( kbSkeletalModelComponent *const pSkelModel ) {

	if ( VectorFind( m_SyncedSkelModels, pSkelModel ) != m_SyncedSkelModels.end() ) {
		return;
	}
	m_SyncedSkelModels.push_back( pSkelModel );
	pSkelModel->m_pSyncParent = this;
}

/**
 *	kbSkeletalModelComponent::UnregisterSyncSkelModel
 */
void kbSkeletalModelComponent::UnregisterSyncSkelModel( kbSkeletalModelComponent *const pSkelModel ) {
	VectorRemoveFast( m_SyncedSkelModels, pSkelModel );
	pSkelModel->m_pSyncParent = nullptr;
}


/**
 *	kbFlingPhysicsComponent::Constructor
 */
void kbFlingPhysicsComponent::Constructor() {

	// Editor
	m_MinLinearVelocity.Set( -0.015f, 0.015f, 0.03f );
	m_MaxLinearVelocity.Set( 0.015f, 0.025f, 0.035f );
	m_MinAngularSpeed = 10.0f;
	m_MaxAngularSpeed = 15.0f;
	m_Gravity .Set( 0.0f, -20.0f, 0.0f );

	// Run time
	m_OwnerStartPos.Set( 0.0f, 0.0f, 0.0f );
	m_OwnerStartRotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );

	m_Velocity.Set( 0.0f, 0.0f, 0.0f );
	m_RotationAxis.Set( 1.0f, 0.0f, 0.0f );

	m_CurRotationAngle = 0.0f;
	m_RotationSpeed = 1.0f;

	m_FlingStartTime = 0.0f;
	
	m_bOwnerStartSet = false;
}

/**
 *	kbFlingPhysicsComponent::SetEnable_Internal
 */
void kbFlingPhysicsComponent::SetEnable_Internal( const bool bEnable ) {

	Super::SetEnable_Internal( bEnable );

	if ( bEnable ) {
		m_OwnerStartPos = GetOwnerPosition();
		m_OwnerStartRotation = GetOwnerRotation();
		m_bOwnerStartSet = true;

		m_Velocity = kbVec3Rand( m_MinLinearVelocity, m_MaxLinearVelocity );

		kbMat4 worldMatrix;
		GetOwner()->CalculateWorldMatrix( worldMatrix );
 		const XMMATRIX inverseMat = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( worldMatrix ) );
		worldMatrix = kbMat4FromXMMATRIX( inverseMat );
		worldMatrix.TransposeSelf();
		m_Velocity = m_Velocity * worldMatrix;

		m_RotationAxis = kbVec3( kbfrand(), kbfrand(), kbfrand() );
		if ( m_RotationAxis.LengthSqr() < 0.01f ) {
			m_RotationAxis.Set( 1.0f, 0.0f, 0.0f );
		} else {
			m_RotationAxis.Normalize();
		}
		m_RotationSpeed = kbfrand( m_MinAngularSpeed, m_MaxAngularSpeed );
		m_CurRotationAngle = 0;

		m_FlingStartTime = g_GlobalTimer.TimeElapsedSeconds();
	} else {
		if ( g_UseEditor ) {
			ResetToStartPos();
		}
	}
}

/**
 *	kbFlingPhysicsComponent::Update_Internal
 */
void kbFlingPhysicsComponent::Update_Internal( const float dt ) {
	Super::Update_Internal( dt );

	const float curTime = g_GlobalTimer.TimeElapsedSeconds();
	const float elapsedDeathTime = curTime - m_FlingStartTime;

	kbVec3 newPos = GetOwnerPosition();
	newPos.x += m_Velocity.x * dt;
	newPos.y = m_OwnerStartPos.y + m_Velocity.y * elapsedDeathTime - ( 0.5f * -m_Gravity.y * elapsedDeathTime * elapsedDeathTime );
	newPos.z += m_Velocity.z * dt;
	SetOwnerPosition( newPos );

	m_CurRotationAngle += m_RotationSpeed * dt;
	kbQuat rot;
	rot.FromAxisAngle( m_RotationAxis, m_CurRotationAngle );
	rot = m_OwnerStartRotation * rot;
	SetOwnerRotation( rot );
}