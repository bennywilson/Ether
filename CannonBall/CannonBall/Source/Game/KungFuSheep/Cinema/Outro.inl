/**
 *	KungFuGame_OutroState
 */
class KungFuGame_OutroState : public KungFuGame_BaseState, public IAnimEventListener {

//---------------------------------------------------------------------------------------------------
public:

	enum eOutroState_States {
		SheepInitialWalkUp,
		SheepSnolafFaceOff,
		TreyTonPounce,
		TreyTonTitle,
		TreyTonPounce2,
		SheepDodge,
		TreyTonExit,
		Dance,
		Snolaf_Reenter,
		Disrespect,
		DanceStare,
		Title_Snolaf,
		Title_Sheep,
		Title_Fox,
		Painted_Title,
		FadeOut
	} m_State;

	kbGameEntityPtr m_p3000TonTitleEntity;
	kbGameEntityPtr m_pTitle;

	KungFuSnolafComponent * m_pSnolafGuards[2];
	KungFuSnolafComponent * m_pLastSnolaf;
	float m_StateStartTime;

	float m_MusicStartTime;

	KungFuGame_OutroState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) {
		m_pSnolafGuards[0] = m_pSnolafGuards[1] = nullptr;
		m_pLastSnolaf = nullptr;
		m_MusicStartTime = 0.0f;
	}

	void ChangeState( const eOutroState_States newState ) {

		m_State = newState;
		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void OnAnimEvent( const kbAnimEventInfo_t & animEvent ) {

		static kbString sPounce_1( "Pounce_1" );
		static kbString sPounce_1_Smear( "Pounce_1_Smear" );
		static kbString sPounce_2_Impact_1( "Pounce_2_Impact_1" );
		static kbString sSlapSound( "SlapSound" );
		static kbString sHitCamera( "HitCamera" );

		const kbString eventName = animEvent.m_AnimEvent.GetEventName();
		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		if ( eventName == sPounce_1 ) {
			m_pSnolafGuards[0]->RequestStateChange( KungFuSnolafState::ForcePoofDeath );
			m_pSnolafGuards[1]->RequestStateChange( KungFuSnolafState::ForcePoofDeath );

			kbGameEntityPtr pEnt = g_pGame->GetEntityByName( kbString( "3000 Ton Smash Snolaf Pos" ) );
			pSheep->PlayCannonBallFX( pEnt.GetEntity()->GetPosition() );
	
			// Give sheep big eyes
			pSheep->SetOverrideFXMaskParameters( kbVec4( 0.0f, 0.0f, 1.0f, 0.0f ) );

			KungFuLevelComponent::Get()->GetPresent(0).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->Enable( true );
			KungFuLevelComponent::Get()->GetPresent(1).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->Enable( true );

		} else if ( eventName == sPounce_1_Smear ) {
			auto pTreyTon = KungFuLevelComponent::Get()->Get3000Ton()->GetComponent<CannonActorComponent>();
			pTreyTon->ApplyAnimSmear( kbVec3( 0.0f, 10.0f, 0.0f ), 0.067f );
		} else if ( eventName == sPounce_2_Impact_1 ) {
			kbVec3 fxPos = pSheep->GetOwnerPosition();
			fxPos.z =  KungFuGame::kOutroStartZ + 7.0f;

			pSheep->PlayCannonBallFX( fxPos );

			// Smash bridge
			KungFuLevelComponent::Get()->DoBreakBridgeEffect( true );

			auto pTreyTon = KungFuLevelComponent::Get()->Get3000Ton()->GetComponent<CannonActorComponent>();
			pTreyTon->ApplyAnimSmear( kbVec3( 0.0f, 10.0f, 0.0f ), 0.067f );
		} else if ( eventName == sSlapSound ) {
			pSheep->PlayImpactSound();
		} else if ( eventName == sHitCamera ) {
			pSheep->PlayCameraShake();
		}
	}

	float GetStateTime() const {
		return g_GlobalTimer.TimeElapsedSeconds() - m_StateStartTime;
	}

	virtual void BeginState_Internal( KungFuGame::eKungFuGame_State previousState ) override {

		static kbString sOutroCam1( "Outro - Camera 1" );
		static kbString sCry( "Cry" );

		for ( int i = 0; i < g_pCannonGame->GetGameEntities().size(); i++ ) {

			kbGameEntity *const pTargetEnt = g_pCannonGame->GetGameEntities()[i];
			if ( pTargetEnt->GetName() == sOutroCam1 ) {
				g_pCannonGame->GetMainCamera()->SetTarget( pTargetEnt, 0.6f );
				continue;
			}

			KungFuSnolafComponent *const pSnolaf = pTargetEnt->GetComponent<KungFuSnolafComponent>();
			if ( pSnolaf == nullptr || pSnolaf->IsEnabled() == false || pSnolaf->IsDead() ) {
				continue;
			}

			pSnolaf->RequestStateChange( KungFuSnolafState::RunAway );
		}

		ChangeState( SheepInitialWalkUp );

		auto pLevelComp = KungFuLevelComponent::Get();
		auto pSheep = pLevelComp->GetSheep();
		pSheep->ExternalRequestStateChange( KungFuSheepState::Cinema );

		static const kbString Run_Anim( "Run_Basic" );
		static const kbString sOfferPresent_1( "OfferPresent_1" );
		static const kbString sOfferPresent_2( "OfferPresent_2" );

		pSheep->PlayAnimation( Run_Anim, 0.15f );
		pSheep->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, -1.0f ) );

		m_pSnolafGuards[0] = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pSnolafGuards[0]->SetOwnerPosition( KungFuGame::kFoxPos + kbVec3( 0.0f, 0.0f, -1.6f ) );
		m_pSnolafGuards[0]->RequestStateChange( KungFuSnolafState::Cinema );
		m_pSnolafGuards[0]->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, -1.0f ) );
		m_pSnolafGuards[0]->PlayAnimation( sOfferPresent_1, 0.0f );

		m_pSnolafGuards[1] = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pSnolafGuards[1]->SetOwnerPosition( KungFuGame::kFoxPos + kbVec3( 0.0f, 0.0f, -2.9f ) );
		m_pSnolafGuards[1]->RequestStateChange( KungFuSnolafState::Cinema );
		m_pSnolafGuards[1]->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
		m_pSnolafGuards[1]->PlayAnimation( sOfferPresent_1, 0.0f );

		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		p3000Ton->SetOwnerPosition( KungFuGame::kTreyTonOffScreenPos );

		auto p3000TonSkel = p3000Ton->GetComponent<kbSkeletalModelComponent>();
		p3000TonSkel->RegisterAnimEventListener( this );
		p3000TonSkel->Enable( false );
		p3000TonSkel->Enable( true );

		auto p3000TonActor = p3000Ton->GetComponent<CannonActorComponent>();
		p3000TonActor->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
		p3000TonActor->PlayAnimation( kbString( "Idle" ), 0.0f );

		static const kbString s3000TonTitle( "3000 Ton Title" );
		m_p3000TonTitleEntity = g_pGame->GetEntityByName( s3000TonTitle );

		static const kbString sTitle( "Title" );
		m_pTitle = g_pGame->GetEntityByName( sTitle );

		// Fox
		auto pFox = pLevelComp->GetFox();
		pFox->PlayAnimation( sCry, 0.0f );
		pFox->SetTargetFacingDirection( kbVec3( -1.0f, 0.0f, 0.0f ) );
		pFox->GetComponent<kbParticleComponent>()->Enable( true );
		pFox->GetComponent<kbSkeletalModelComponent>()->RegisterAnimEventListener( this );

		pLevelComp->GetPresent(0).GetEntity()->GetComponent<kbSkeletalModelComponent>()->Enable( true );
		pLevelComp->GetPresent(1).GetEntity()->GetComponent<kbSkeletalModelComponent>()->Enable( true );

		m_pLastSnolaf = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pLastSnolaf->GetComponent<kbSkeletalModelComponent>()->RegisterAnimEventListener( this );
		m_pLastSnolaf->GetComponent<KungFuSnolafComponent>()->RequestStateChange( KungFuSnolafState::Cinema );

		m_MusicStartTime = g_GlobalTimer.TimeElapsedSeconds() + 0.75f;
	}

	virtual void UpdateState_Internal() override {
		
		if ( g_SkipCheat == KungFuGame::Skip_ToEnd ) {
			if ( g_pCannonGame->GetPlayer()->IsDead() ) {
				RequestStateChange( KungFuGame::PlayerDead );
			}
		}

		if ( m_MusicStartTime > 0 && g_GlobalTimer.TimeElapsedSeconds() > m_MusicStartTime ) {
			m_MusicStartTime = -1.0f;
			KungFuLevelComponent::Get()->SetPlayLevelMusic( 1, false );
			KungFuLevelComponent::Get()->SetPlayLevelMusic( 1, true );
		}
		static const kbString sPounce_1( "Pounce_1" );
		static const kbString sPounce_2( "Pounce_2" );
		static const kbString sFoxStare( "Stare" );
		static const kbString sDance( "Dance" );
		static const kbString sDisrespect( "Disrespect" );

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		auto pCamera = g_pCannonGame->GetMainCamera();
		auto pFox = KungFuLevelComponent::Get()->GetFox()->GetComponent<CannonActorComponent>();

		const float SheepPullUpZ = KungFuGame::kOutroStartZ + 8.8f;

		switch( m_State ) {

			case SheepInitialWalkUp : {
				auto frameDt = g_pGame->GetFrameDT();
				kbVec3 targetPos = pSheep->GetOwnerPosition() + kbVec3( 0.0f, 0.0f, 1.0f ) * frameDt * pSheep->GetMaxRunSpeed();
				if ( targetPos.z >= SheepPullUpZ ) {
					targetPos.z = SheepPullUpZ;
					ChangeState( SheepSnolafFaceOff );

					static const kbString IdleL_Anim( "IdleLeft_Basic" );
					pSheep->PlayAnimation( IdleL_Anim, 0.2f );
				}
				pSheep->SetOwnerPosition( targetPos );
				break;
			}

			case SheepSnolafFaceOff : {

				if ( GetStateTime() > 1.25f ) {
					ChangeState( TreyTonPounce );
					p3000Ton->SetOwnerPosition( KungFuGame::kTreyTonJump1StartPos );
					p3000Ton->PlayAnimation( sPounce_1, -1.0f );
				}

				break;
			}

			case TreyTonPounce : {

				if ( GetStateTime() > 1.0f ) {

					const kbVec3 posOffset = kbVec3( 0.0f, 0.0f, -13.0f ) + kbVec3( -10.933998f, 3.224068f,  10.000000f ) * 0.3f;

					const float lerpSpeed = 3.0f;
					pCamera->SetTarget( p3000Ton->GetOwner(), lerpSpeed );
					pCamera->SetPositionOffset( posOffset, lerpSpeed );
					pCamera->SetLookAtOffset( kbVec3( 0.0f, 0.0f, -8.0f ), lerpSpeed );
					ChangeState( TreyTonTitle );
					pFox->GetComponent<kbParticleComponent>()->Enable( false );

				}
				/*if ( p3000Ton->HasFinishedAnim( sSquashSnolafs ) ) {
					ChangeState( TreyTonTitle );
				}*/
				break;
			}

			case TreyTonTitle : {

				const float DisplayTitleTime = 0.5f;
				const float HideTitleTime = 3.5f;
				const float ZoomOutSpeed = 3.0f;
				static const kbString sOutroCam1("Outro - Camera 1");

				if ( GetStateTime() > DisplayTitleTime ) {
					if ( m_p3000TonTitleEntity.GetEntity() != nullptr ) {
						g_pGame->SetDeltaTimeScale( 0.0f );
						m_p3000TonTitleEntity.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( true );
					}
				}

				if ( GetStateTime() > HideTitleTime ) {

					auto pCamera = g_pCannonGame->GetMainCamera();
					pCamera->SetLookAtOffset( kbVec3( 0.000000f, 2.500000f, 0.000000f ), ZoomOutSpeed );
					pCamera->SetPositionOffset( kbVec3( -10.933998f, 3.224068f, 0.000000f ), ZoomOutSpeed );

					kbGameEntityPtr pOutroCamEnt = g_pGame->GetEntityByName( sOutroCam1 );
					if ( pOutroCamEnt.GetEntity() != nullptr ) {
						pCamera->SetTarget( pOutroCamEnt.GetEntity(), ZoomOutSpeed );
					}

					if ( m_p3000TonTitleEntity.GetEntity() != nullptr ) {
						g_pGame->SetDeltaTimeScale( 1.0f );
						m_p3000TonTitleEntity.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( false );
					}

					p3000Ton->SetOwnerPosition( KungFuGame::kTreyTonJump2StartPos );
					p3000Ton->PlayAnimation( kbString( "KaratePose" ), 0.0f );

					ChangeState( TreyTonPounce2 );
				}
				break;
			}

			case TreyTonPounce2 : {

				// Start Pounce2
				if ( GetStateTime() > 0.15f ) {
					p3000Ton->PlayAnimation( sPounce_2, 0.15f, false );
				}

				// Sheep starts watching 3000 Tons' pounce
				if ( GetStateTime() > 0.85f ) {
					pSheep->PlayAnimation( kbString( "Watch3000Ton" ), 0.5f, false );
				}

				// Play baa
				if ( GetStateTime() > 1.25f ) {
					pSheep->PlayBaa( 2 );
					ChangeState( SheepDodge );
				}
				break;
			}

			case SheepDodge: {
				const float sheepHopRate = 4.0f;
				const float sheepTargetZ = -286.2f;
				 
				if ( GetStateTime() > 0.5f ) {
					kbVec3 sheepPos = pSheep->GetOwnerPosition();
					sheepPos.z += sheepHopRate * g_pGame->GetFrameDT();
					if ( sheepPos.z >= sheepTargetZ ) {
						sheepPos.z = sheepTargetZ;
						pSheep->SetOverrideFXMaskParameters( kbVec4( 1.0f, 0.0f, 0.0f, 0.0f ) );
						pSheep->SetOverrideFXMaskParameters( kbVec4( 1.0f, 0.0f, 0.0f, 0.0f ) );
					}
					pSheep->SetOwnerPosition( sheepPos );

					if ( GetStateTime() > 1.2f ) {
						pFox->GetComponent<CannonActorComponent>()->PlayAnimation( sFoxStare, 0.15f, false );
					}

					if ( GetStateTime() > 1.65f ) {
						ChangeState( TreyTonExit );
					}
				}
				break;
			}

			case TreyTonExit : {
				bool bFin = true;
				const float StareAtCamLen = 1.0f;
				if ( GetStateTime() > StareAtCamLen ) {
					pSheep->PlayAnimation( kbString( "Run_Basic" ), 0.15f );
					kbVec3 sheepPos = pSheep->GetOwnerPosition();
					sheepPos.z += pSheep->GetMaxRunSpeed() * g_pGame->GetFrameDT();
					if ( sheepPos.z >= KungFuGame::kSheepFinalPos.z ) {
						sheepPos.z = KungFuGame::kSheepFinalPos.z;
						pSheep->PlayAnimation( kbString( "IdleLeft_Basic" ), 0.0f );
					} else {
						bFin = false;
					}
					pSheep->SetOwnerPosition( sheepPos );
				} else {
					bFin = false;
				}

				if ( GetStateTime() > StareAtCamLen + 0.15f ) {
					pFox->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
					pFox->PlayAnimation( kbString( "Walk" ), 0.15f );

					kbVec3 foxPos = pFox->GetOwnerPosition();
					foxPos.z -= pFox->GetMaxRunSpeed() * g_pGame->GetFrameDT();
					if ( foxPos.z <= KungFuGame::kFoxFinalPos.z ) {
						foxPos.z = KungFuGame::kFoxFinalPos.z;
						pFox->PlayAnimation( kbString( "Idle" ), 0.15f );

					} else {
						bFin = false;
					}
					pFox->SetOwnerPosition( foxPos );
				}

				if ( bFin ) {
					ChangeState( Dance );
					pFox->PlayAnimation( sDance, 0.15f );
					pFox->SetTargetFacingDirection( kbVec3( -1.f, 0.0f, 0.0f ) );

					pSheep->PlayAnimation( sDance, 0.15f );
					pSheep->SetTargetFacingDirection( kbVec3( -1.f, 0.0f, 0.0f ) );
				} else {
					bFin = false;
				}
				break;
			}

			case Dance : {

				if ( GetStateTime() > 1.0f ) {

					m_pLastSnolaf->SetOwnerPosition( KungFuGame::kFinalSnolafEntryPos + kbVec3( 0.0f, 0.0f, 2.0f ) );	// Spawn Snolaf offscreen
					m_pLastSnolaf->RequestStateChange( KungFuSnolafState::Cinema );
					m_pLastSnolaf->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
					m_pLastSnolaf->PlayAnimation( kbString( "Reenter" ), 0.0f );
					ChangeState( Snolaf_Reenter );
				}
				break;
			}

			case Snolaf_Reenter : {

				const float SnolafPeerLen = 2.0f;
				const float SnolafRunMultiplier = 2.0f;

				if ( GetStateTime() > SnolafPeerLen ) {
					const kbVec3 snolafTargetPos = KungFuGame::kSheepFinalPos + kbVec3( 0.0f, 0.0f, 1.1f );
					kbVec3 snolafCurPos = m_pLastSnolaf->GetOwnerPosition();
					if ( snolafCurPos.z > snolafTargetPos.z ) {
						m_pLastSnolaf->PlayAnimation( kbString( "Run" ), 0.15f );
						snolafCurPos.z -= m_pLastSnolaf->GetMaxRunSpeed() * g_pGame->GetFrameDT() * SnolafRunMultiplier;
					} else {
						m_pLastSnolaf->PlayAnimation( kbString( "Dance" ), 0.15f );
						m_pLastSnolaf->SetTargetFacingDirection( kbVec3( -1.0f, 0.0f, 0.0f ) );
						ChangeState( DanceStare );
					}
					m_pLastSnolaf->SetOwnerPosition( snolafCurPos );
				} else {
					// Move Snolaf to edge of screen
					if ( GetStateTime() > 0.5f ) {
						m_pLastSnolaf->SetOwnerPosition( KungFuGame::kFinalSnolafEntryPos );
					}
				}
				break;
			}

			case DanceStare : {

				const float FoxStareTime = 1.5f;
				const float SheepStareTime = 1.25f;

				if ( GetStateTime() > SheepStareTime ) {
					pSheep->PlayAnimation( kbString( "Dance_Stare" ), 0.15f );
				}

				if ( GetStateTime() > FoxStareTime ) {
					pFox->PlayAnimation( kbString( "Dance_Stare" ), 0.15f );
					ChangeState( Disrespect );
				}	
				break;
			}

			case Disrespect : {
				if ( GetStateTime() > 1.2f ) {
					pSheep->PlayAnimation( kbString( "Disrespect" ), 0.15f );
				}

				if ( GetStateTime() > 1.5f ) {
					pFox->PlayAnimation( kbString( "Disrespect" ), 0.15f );
					pFox->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
				}
				if ( GetStateTime() > 3.8f ) {
					pFox->PlayAnimation( kbString( "Slap Snolaf" ), 0.15f, false );
					pSheep->PlayAnimation( kbString( "Slap Snolaf" ), 0.15f, false );
					ChangeState( Title_Snolaf );
				}
				break;
			}

			case Title_Snolaf : {
				if ( GetStateTime() > 0.25f ) {
					m_pLastSnolaf->PlayAnimation( kbString( "Title" ), 0.15f );
					m_pLastSnolaf->SetOverrideFXMaskParameters( kbVec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
					ChangeState( Title_Sheep );
				}

				break;
			}

			case Title_Sheep : {
				if ( GetStateTime() > 1.0f ) {

					ChangeState( Title_Fox );
				} else if ( GetStateTime() > 0.6f ) {
					pFox->PlayAnimation( kbString( "Water_Dive" ), 0.15f );
				} else if ( GetStateTime() > 0.3f ) {
					pSheep->PlayAnimation( kbString( "Water_Dive" ), 0.15f );
				}
				break;
			}

			case Title_Fox : {
				if ( GetStateTime() > 1.0f ) {
					pFox->PlayAnimation( kbString( "Title" ), 0.15f );
					ChangeState( Painted_Title );
				} else if ( GetStateTime() > 0.5f ) {
					pSheep->PlayAnimation( kbString( "Title" ), 0.15f );
					pSheep->EnableHeadBand( false );
				}
				break;
			}

			case Painted_Title : {
				if ( GetStateTime() > 1.0f && m_pTitle.GetEntity() != nullptr ) {
					m_pTitle.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( true );
				}
				break;
			}
		}
	}

	virtual void EndState_Internal( KungFuGame::eKungFuGame_State nextstate ) {
		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		auto p3000TonSkel = p3000Ton->GetComponent<kbSkeletalModelComponent>();
		p3000TonSkel->UnregisterAnimEventListener( this );

		auto pCamera = g_pCannonGame->GetMainCamera();
		pCamera->SetLookAtOffset( kbVec3( 0.000000f, 2.500000f, 0.000000f ), -1.0f );
		pCamera->SetPositionOffset( kbVec3( -10.933998f, 3.224068f, 0.000000f ), -1.0f );

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		pSheep->EnableHeadBand( true );

		KungFuLevelComponent::Get()->GetPresent(0).GetEntity()->GetComponent<kbSkeletalModelComponent>()->Enable( false );
		KungFuLevelComponent::Get()->GetPresent(1).GetEntity()->GetComponent<kbSkeletalModelComponent>()->Enable( false );
		KungFuLevelComponent::Get()->GetPresent(0).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->Enable( false );
		KungFuLevelComponent::Get()->GetPresent(1).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->Enable( false );
		KungFuLevelComponent::Get()->GetPresent(0).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->ResetToStartPos();
		KungFuLevelComponent::Get()->GetPresent(1).GetEntity()->GetComponent<kbFlingPhysicsComponent>()->ResetToStartPos();
		KungFuLevelComponent::Get()->DoBreakBridgeEffect( false );

		if ( m_pTitle.GetEntity() != nullptr ) {
			m_pTitle.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( false );
		}
		
		auto pFox = KungFuLevelComponent::Get()->GetFox();
		pFox->GetComponent<kbParticleComponent>()->Enable( false );
		pFox->GetComponent<kbSkeletalModelComponent>()->UnregisterAnimEventListener( this );

		m_pLastSnolaf->SetOverrideFXMaskParameters( kbVec4( -1.0f, -1.0f, -1.0f, -1.0f ) );
		m_pLastSnolaf->GetComponent<kbSkeletalModelComponent>()->UnregisterAnimEventListener( this );
		KungFuLevelComponent::Get()->ReturnSnolafToPool( m_pLastSnolaf );
		m_pLastSnolaf = nullptr;

		KungFuLevelComponent::Get()->ReturnSnolafToPool( m_pSnolafGuards[0] );
		KungFuLevelComponent::Get()->ReturnSnolafToPool( m_pSnolafGuards[1] );
	}

private:

};