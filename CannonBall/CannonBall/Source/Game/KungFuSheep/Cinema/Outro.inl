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
		Dance
	} m_State;

	kbGameEntityPtr m_p3000TonTitleEntity;
	KungFuSnolafComponent * m_pSnolafGuards[3];
	float m_StateStartTime;

	KungFuGame_OutroState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) {

	}

	void ChangeState( const eOutroState_States newState ) {

		m_State = newState;
		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void OnAnimEvent( const kbAnimEventInfo_t & animEvent ) {

		static kbString sPounce_1( "Pounce_1" );
		static kbString sPounce_1_Smear( "Pounce_1_Smear" );
		const kbString eventName = animEvent.m_AnimEvent.GetEventName();
		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		if ( eventName == sPounce_1 ) {
			m_pSnolafGuards[0]->RequestStateChange( KungFuSnolafState::Dead );
			m_pSnolafGuards[1]->RequestStateChange( KungFuSnolafState::Dead );
			m_pSnolafGuards[2]->RequestStateChange( KungFuSnolafState::Dead );

			kbGameEntityPtr pEnt = g_pGame->GetEntityByName( kbString( "3000 Ton Smash Snolaf Pos" ) );
			pSheep->PlayCannonBallFX( pEnt.GetEntity()->GetPosition() );
	
			// Give sheep big eyes
			pSheep->SetOverrideFXMaskParameters( kbVec4( 0.0f, 0.0f, 1.0f, 0.0f ) );

		} else if ( eventName == sPounce_1_Smear ) {
			auto pTreyTon = KungFuLevelComponent::Get()->Get3000Ton()->GetComponent<CannonActorComponent>();
			pTreyTon->ApplyAnimSmear( kbVec3( 0.0f, 5.0f, 0.0f ), 0.067f );
		}
	}

	float GetStateTime() const {
		return g_GlobalTimer.TimeElapsedSeconds() - m_StateStartTime;
	}

	virtual void BeginState_Internal( KungFuGame::eKungFuGame_State previousState ) override {
		
		static kbString sOutroCam1 = kbString( "Outro Camera 1" );
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

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		pSheep->ExternalRequestStateChange( KungFuSheepState::Cinema );

		static const kbString Run_Anim( "Run_Basic" );
		pSheep->PlayAnimation( Run_Anim, 0.15f );
		pSheep->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, -1.0f ) );

		m_pSnolafGuards[0] = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pSnolafGuards[0]->SetOwnerPosition( KungFuGame::kFoxPos + kbVec3( 0.0f, 0.0f, 1.0f ) );
		m_pSnolafGuards[0]->RequestStateChange( KungFuSnolafState::Cinema );
		m_pSnolafGuards[0]->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );

		m_pSnolafGuards[1] = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pSnolafGuards[1]->SetOwnerPosition( KungFuGame::kFoxPos + kbVec3( 0.0f, 0.0f, -0.6f ) );
		m_pSnolafGuards[1]->RequestStateChange( KungFuSnolafState::Cinema );
		m_pSnolafGuards[1]->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );

		m_pSnolafGuards[2] = KungFuLevelComponent::Get()->GetSnolafFromPool();
		m_pSnolafGuards[2]->SetOwnerPosition( KungFuGame::kFoxPos + kbVec3( 0.0f, 0.0f, -1.0f ) );
		m_pSnolafGuards[2]->RequestStateChange( KungFuSnolafState::Cinema );
		m_pSnolafGuards[2]->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, -1.0f ) );

		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		p3000Ton->SetOwnerPosition( KungFuGame::kTreyTonStartPos );

		auto p3000TonSkel = p3000Ton->GetComponent<kbSkeletalModelComponent>();
		p3000TonSkel->RegisterAnimEventListener( this );
		p3000TonSkel->Enable( false );
		p3000TonSkel->Enable( true );

		auto p3000TonActor = p3000Ton->GetComponent<CannonActorComponent>();
		p3000TonActor->SetTargetFacingDirection( kbVec3( 0.0f, 0.0f, 1.0f ) );
		p3000TonActor->PlayAnimation( kbString( "Idle" ), 0.0f );

		static const kbString s3000TonTitle( "3000 Ton Title" );
		m_p3000TonTitleEntity = g_pGame->GetEntityByName( s3000TonTitle );
	}

	virtual void UpdateState_Internal() override {
		
		if ( g_SkipCheat == KungFuGame::Skip_ToEnd ) {
			if ( g_pCannonGame->GetPlayer()->IsDead() ) {
				RequestStateChange( KungFuGame::PlayerDead );
			}
		}

		static const kbString sPounce_1( "Pounce_1" );
		static const kbString sPounce_2( "Pounce_2" );

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		auto pCamera = g_pCannonGame->GetMainCamera();

		const float SheepPullUpZ = KungFuGame::kOutroStartZ + 7.0f;

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
				ChangeState( TreyTonPounce );
				p3000Ton->PlayAnimation( sPounce_1, -1.0f );

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
				const kbVec3 TreyTonLandSpot( 76.992683f, -52.626686f, -235.728653f );


				if ( GetStateTime() > DisplayTitleTime ) {
					if ( m_p3000TonTitleEntity.GetEntity() != nullptr ) {
						m_p3000TonTitleEntity.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( true );
					}
				}

				if ( GetStateTime() > HideTitleTime ) {


					auto pCamera = g_pCannonGame->GetMainCamera();
					pCamera->SetLookAtOffset( kbVec3( 0.000000f, 2.500000f, 0.000000f ), ZoomOutSpeed );
					pCamera->SetPositionOffset( kbVec3( -10.933998f, 3.224068f, 0.000000f ), ZoomOutSpeed );

					kbGameEntityPtr pOutroCamEnt = g_pGame->GetEntityByName( kbString( "Outro Camera 1" ) );
					if ( pOutroCamEnt.GetEntity() != nullptr ) {
						pCamera->SetTarget( pOutroCamEnt.GetEntity(), ZoomOutSpeed );
					}
					if ( m_p3000TonTitleEntity.GetEntity() != nullptr ) {
						m_p3000TonTitleEntity.GetEntity()->GetComponent<kbUIWidgetComponent>()->Enable( false );
					}

					p3000Ton->SetOwnerPosition( TreyTonLandSpot );
					p3000Ton->PlayAnimation( kbString( "KaratePose" ), 0.0f );

					ChangeState( TreyTonPounce2 );
				}
				break;
			}

			case TreyTonPounce2 : {
				if ( GetStateTime() > 0.15f ) {
					p3000Ton->PlayAnimation( sPounce_2, 0.15f, false );
				}
				if ( GetStateTime() > 0.85f ) {
					pSheep->PlayAnimation( kbString( "Watch3000Ton" ), 0.5f, false );
				}

				if ( GetStateTime() > 1.6f ) {
					pSheep->PlayBaa( 2 );
					ChangeState( SheepDodge );
					pSheep->SetOverrideFXMaskParameters( kbVec4( 1.0f, 0.0f, 0.0f, 0.0f ) );
					pSheep->SetOverrideFXMaskParameters( kbVec4( 1.0f, 0.0f, 0.0f, 0.0f ) );
				}
				break;
			}

			case SheepDodge:
			{
				const float sheepHopRate = 3.1f;
				const float sheepTargetZ = -241.8f;
				 
				if ( GetStateTime() > 0.25f ) {
					kbVec3 sheepPos = pSheep->GetOwnerPosition();
					sheepPos.z += sheepHopRate * g_pGame->GetFrameDT();
					if ( sheepPos.z >= sheepTargetZ ) {
						sheepPos.z = sheepTargetZ;
						ChangeState( TreyTonExit );
					}
					pSheep->SetOwnerPosition( sheepPos );
				}
			}

			case TreyTonExit :
			{
				if ( GetStateTime() > 2.0f ) {
					pSheep->PlayAnimation( kbString( "Run_Basic" ), 0.15f );
					kbVec3 sheepPos = pSheep->GetOwnerPosition();
					sheepPos.z += pSheep->GetMaxRunSpeed() * g_pGame->GetFrameDT();
					if ( sheepPos.z >= KungFuGame::kSheepFinalPos.z ) {
						sheepPos.z = KungFuGame::kSheepFinalPos.z;
						pSheep->PlayAnimation( kbString( "IdleLeft_Basic" ), 0.0f );
						ChangeState( Dance );
					}
					pSheep->SetOwnerPosition( sheepPos );
				}
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
	}

private:

};