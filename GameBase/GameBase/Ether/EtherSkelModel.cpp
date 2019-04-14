//===================================================================================================
// EtherSkelModel.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "EtherGame.h"
#include "EtherSkelModel.h"
#include "kbRenderer.h"
#include "kbConsole.h"
#include "DX11/kbRenderer_DX11.h"			// HACK

KB_DEFINE_COMPONENT(EtherAnimComponent)
KB_DEFINE_COMPONENT(EtherSkelModelComponent)

#define DEBUG_ANIMS 0

// TODO: HACK!
static XMMATRIX & XMMATRIXFromkbMat4( kbMat4 & matrix ) { return (*(XMMATRIX*) &matrix); }
static kbMat4 & kbMat4FromXMMATRIX( FXMMATRIX & matrix ) { return (*(kbMat4*) & matrix); }

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
void EtherSkelModelComponent::PlayAnimation( const kbString & AnimationName, const float BlendLength, const bool bRestartIfAlreadyPlaying, const kbString desiredNextAnimation, const float desiredNextAnimationBlendLength ) {

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
 *	EtherSkelModelComponent::IsPlaying
 */
bool EtherSkelModelComponent::IsPlaying( const kbString & AnimationName ) const {
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

	m_BindToLocalSpaceMatrices.clear();
	m_RenderObject.m_pModel = pModel;
	g_pRenderer->UpdateRenderObject( m_RenderObject );
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

		EtherDestructibleComponent *const pDestructible = (EtherDestructibleComponent*)GetOwner()->GetComponentByType( EtherDestructibleComponent::GetType() );
		if ( pDestructible != nullptr && pDestructible->IsSimulating() ) {

			const std::vector<EtherDestructibleComponent::destructibleBone_t> & brokenBones = pDestructible->GetBonesList();
			const kbModel *const pModel = this->GetModel();
			for ( int i = 0; i < brokenBones.size(); i++ ) {
				const EtherDestructibleComponent::destructibleBone_t & destructibleBone = brokenBones[i];

				kbQuat rot;
				rot.FromAxisAngle( destructibleBone.m_RotationAxis, destructibleBone.m_CurRotationAngle );
				kbMat4 matRot = rot.ToMat4();
				m_BindToLocalSpaceMatrices[i].SetAxis( 0, matRot[0].ToVec3() );
				m_BindToLocalSpaceMatrices[i].SetAxis( 1, matRot[1].ToVec3() );
				m_BindToLocalSpaceMatrices[i].SetAxis( 2, matRot[2].ToVec3() );

				m_BindToLocalSpaceMatrices[i].SetAxis( 3, destructibleBone.m_Position );

				m_BindToLocalSpaceMatrices[i] = pModel->GetInvRefBoneMatrix(i) * m_BindToLocalSpaceMatrices[i];

				//kbVec3 worldPos = destructibleBone.m_Position * GetOwner()->GetOrientation().ToMat4() + GetOwner()->GetPosition();
				//g_pRenderer->DrawBox( kbBounds( worldPos - kbVec3::one * 0.1f, worldPos + kbVec3::one * 0.1f ), kbColor::red );
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

				m_pModel->Animate( CurAnim.m_CurrentAnimationTime, CurAnim.m_pAnimation, CurAnim.m_bIsLooping, m_BindToLocalSpaceMatrices );

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

				EtherAnimComponent & NextAnim = m_Animations[m_NextAnimation];
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

/**
 *	EtherDestructibleComponent::Constructor
 */
void EtherDestructibleComponent::Constructor() {
	m_DestructibleType = EDestructibleBehavior::PushFromImpactPoint;
	m_pNonDamagedModel = nullptr;
	m_pDamagedModel = nullptr;
	m_MaxLifeTime = 4.0f;
	m_Gravity.Set( 0.0f, 22.0f, 0.0f );
	m_MinLinearVelocity.Set( 20.0f, 20.0f, 20.0f );
	m_MaxLinearVelocity.Set( 25.0f, 25.0f, 25.0f );
	m_MinAngularVelocity = 5.0f;
	m_MaxAngularVelocity = 10.0f;
	m_StartingHealth = 6.0f;
	m_DestructionFXLocalOffset.Set( 0.0f, 0.0f, 0.0f );

	m_bDebugResetSim = false;
	m_Health = 6.0f;
	m_pSkelModel = nullptr;
	m_bIsSimulating = false;
	m_SimStartTime = 0.0f;
}

/**
 *	EtherDestructibleComponent::EditorChange
 */
void EtherDestructibleComponent::EditorChange( const std::string & propertyName ) {
	Super::EditorChange( propertyName );

	if ( propertyName == "ResetSim" ) {
		if ( m_bIsSimulating ) {
			m_bIsSimulating = false;
			m_Health = m_StartingHealth;
		} else {
			TakeDamage( 9999999.0f, GetOwner()->GetPosition() + kbVec3( kbfrand(), kbfrand(), kbfrand() ) * 5.0f, 10000000.0f );
		}
	}
}

/**
 *	EtherDestructibleComponent::TakeDamage
 */
void EtherDestructibleComponent::TakeDamage( const float damageAmt, const kbVec3 & explosionPosition, const float explosionRadius ) {
	m_Health -= damageAmt;
	if ( m_Health > 0.0f || m_bIsSimulating ) {
		return;
	}

	if ( m_pSkelModel == nullptr ) {
		m_pSkelModel = (EtherSkelModelComponent *) GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
	}

	kbErrorCheck( m_pSkelModel != nullptr, "EtherDestructibleComponent::TakeDamage() - Missing skeletal model" );

	kbCollisionComponent *const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );
	if ( pCollision != nullptr ) {
		pCollision->Enable( false );
	}


	if ( g_UseEditor == false ) {

		if ( m_pDamagedModel != nullptr ) {
			for ( int i = 0; i < m_DamagedModelMaterialParams.size(); i++ ) {
				const kbShaderParamComponent & shaderParam = m_DamagedModelMaterialParams[i];
				if ( shaderParam.GetTexture() != nullptr ) {
					m_pSkelModel->SetMaterialParamTexture( 0, shaderParam.GetParamName().stl_str(), const_cast<kbTexture*>( shaderParam.GetTexture() ) );
				} else {
					m_pSkelModel->SetMaterialParamVector( 0, shaderParam.GetParamName().stl_str(), shaderParam.GetVector() );
				}
			}
			m_pSkelModel->SetModel( m_pDamagedModel, false );
		}
	}

	m_LastHitLocation = explosionPosition;

	kbMat4 localMat;
	GetOwner()->CalculateWorldMatrix( localMat );
	const XMMATRIX inverseMat = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( localMat ) );
	localMat = kbMat4FromXMMATRIX( inverseMat );

	const kbVec3 localExplositionPos = localMat.TransformPoint( explosionPosition );
	const kbModel *const pModel = m_pSkelModel->GetModel();

	kbMat4 worldMat = localMat;
	worldMat.TransposeSelf();

	m_BonesList.resize( pModel->NumBones() );
	for ( int i = 0; i < pModel->NumBones(); i++ ) {
		m_BonesList[i].m_Position = pModel->GetRefBoneMatrix(i).GetOrigin();

		if ( m_DestructibleType == EDestructibleBehavior::UserVelocity ) {
			m_BonesList[i].m_Velocity = kbVec3Rand( m_MinLinearVelocity, m_MaxLinearVelocity );


			m_BonesList[i].m_Velocity = m_BonesList[i].m_Velocity * worldMat;

		} else {
			m_BonesList[i].m_Velocity = ( m_BonesList[i].m_Position - localExplositionPos ).Normalized() * ( kbfrand() * ( m_MaxLinearVelocity.x - m_MinLinearVelocity.x ) + m_MinLinearVelocity.x );
		}

		m_BonesList[i].m_Acceleration = kbVec3::zero;
		m_BonesList[i].m_RotationAxis = kbVec3( kbfrand(), kbfrand(), kbfrand() );
		if ( m_BonesList[i].m_RotationAxis.LengthSqr() < 0.01f ) {
			m_BonesList[i].m_RotationAxis.Set( 1.0f, 0.0f, 0.0f );
		} else {
			m_BonesList[i].m_RotationAxis.Normalize();
		}

		m_BonesList[i].m_RotationSpeed = kbfrand() * ( m_MaxAngularVelocity - m_MinAngularVelocity ) + m_MinAngularVelocity;
		m_BonesList[i].m_CurRotationAngle = 0.0f;
	}

//	if ( m_CompleteDestructionFX.size() > 0 ) {
	//	const int iFX = rand() % m_CompleteDestructionFX.size();
		if ( m_CompleteDestructionFX.GetEntity() != nullptr ) {
			kbGameEntity *const pExplosionFX = g_pGame->CreateEntity( m_CompleteDestructionFX.GetEntity() );

			kbVec3 worldOffset = worldMat.TransformPoint( m_DestructionFXLocalOffset );
//kbLog( "% f %f %f --- %f %f %f", m_DestructionFXLocalOffset.x, m_DestructionFXLocalOffset.y, m_DestructionFXLocalOffset.z, worldOffset.x, worldOffset.y, worldOffset.z );
			pExplosionFX->SetPosition( GetOwner()->GetPosition() + worldOffset );
			pExplosionFX->SetOrientation( GetOwner()->GetOrientation() );
			pExplosionFX->DeleteWhenComponentsAreInactive( true );
		}
	//}



	m_bIsSimulating = true;
	m_SimStartTime = g_GlobalTimer.TimeElapsedSeconds();
}

/**
 *	EtherDestructibleComponent::SetEnable_Internal
 */
void EtherDestructibleComponent::SetEnable_Internal( const bool bEnable ) {
	if ( bEnable == false ) {
		m_pSkelModel = nullptr;
	} else {
		m_pSkelModel = (EtherSkelModelComponent *) GetOwner()->GetComponentByType( EtherSkelModelComponent::GetType() );
		if ( m_pSkelModel == nullptr || m_pSkelModel->GetModel() == nullptr ) {
			kbWarning( "EtherDestructibleComponent::SetEnable_Internal() - No skeletal model found on entity %", GetOwner()->GetName().c_str() );
			this->Enable( false );
			return;
		}

		if ( g_UseEditor == false ) {
			if ( m_pNonDamagedModel != nullptr ) {
				m_pSkelModel->SetModel( m_pNonDamagedModel, false );
			}

			for ( int i = 0; i < m_NonDamagedModelMaterialParams.size(); i++ ) {
				const kbShaderParamComponent & shaderParam = m_NonDamagedModelMaterialParams[i];
				if ( shaderParam.GetTexture() != nullptr ) {
					m_pSkelModel->SetMaterialParamTexture( 0, shaderParam.GetParamName().stl_str(), const_cast<kbTexture*>( shaderParam.GetTexture() ) );
				} else {
					m_pSkelModel->SetMaterialParamVector( 0, shaderParam.GetParamName().stl_str(), shaderParam.GetVector() );
				}
			}
		}

		m_BonesList.resize( m_pSkelModel->GetModel()->NumBones() );

		m_Health = m_StartingHealth;
	}
}

/**
 *	EtherDestructibleComponent::Update_Internal
 */
extern kbConsoleVariable g_ShowCollision;

void EtherDestructibleComponent::Update_Internal( const float deltaTime ) {

	if ( GetAsyncKeyState( 'G' ) ) {
		m_bIsSimulating = false;
		m_Health = m_StartingHealth;

		if ( g_UseEditor == false ) {
			if ( m_pNonDamagedModel != nullptr ) {
				m_pSkelModel->SetModel( m_pNonDamagedModel, false );

				for ( int i = 0; i < m_NonDamagedModelMaterialParams.size(); i++ ) {
					const kbShaderParamComponent & shaderParam = m_NonDamagedModelMaterialParams[i];
					if ( shaderParam.GetTexture() != nullptr ) {
						m_pSkelModel->SetMaterialParamTexture( 0, shaderParam.GetParamName().stl_str(), const_cast<kbTexture*>( shaderParam.GetTexture() ) );
					} else {
						m_pSkelModel->SetMaterialParamVector( 0, shaderParam.GetParamName().stl_str(), shaderParam.GetVector() );
					}
				}
			}
		}

		kbCollisionComponent *const pCollision = (kbCollisionComponent*)GetOwner()->GetComponentByType( kbCollisionComponent::GetType() );
		if ( pCollision != nullptr ) {
			pCollision->Enable( true );
		}
	}

	if ( m_bIsSimulating ) {
		const float t = g_GlobalTimer.TimeElapsedSeconds() - m_SimStartTime;

		if ( t > m_MaxLifeTime ) {
			GetOwner()->DisableAllComponents();
			m_bIsSimulating = false;
		} else {
			const float tSqr = t * t;
			const kbModel *const pModel = m_pSkelModel->GetModel();

			for ( int i = 0; i < m_BonesList.size(); i++ ) {
				m_BonesList[i].m_Position.x += m_BonesList[i].m_Velocity.x * deltaTime;
				m_BonesList[i].m_Position.z += m_BonesList[i].m_Velocity.z * deltaTime;
			
				m_BonesList[i].m_Position.y = pModel->GetRefBoneMatrix(i).GetOrigin().y + ( m_BonesList[i].m_Velocity.y * t - ( 0.5f * m_Gravity.y * tSqr ) );
				m_BonesList[i].m_CurRotationAngle += m_BonesList[i].m_RotationSpeed * deltaTime;
			}
		}

		if ( g_ShowCollision.GetBool() ) {
			g_pRenderer->DrawBox( kbBounds( m_LastHitLocation - kbVec3::one, m_LastHitLocation + kbVec3::one ), kbColor::red );
		}
	}
}