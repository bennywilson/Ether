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
		TreyTonTitle
	} m_State;

	KungFuSnolafComponent * m_pSnolafGuards[3];
	int m_StateStartTime;

	KungFuGame_OutroState( KungFuLevelComponent *const pLevelComponent ) : KungFuGame_BaseState( pLevelComponent ) {

	}

	void ChangeState( const eOutroState_States newState ) {

		m_State = newState;
		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
	}

	virtual void OnAnimEvent( const kbAnimEventInfo_t & animEvent ) {

		if ( animEvent.m_AnimEvent.GetEventName() == kbString( "Squash Snolafs" ) ) {
			m_pSnolafGuards[0]->RequestStateChange( KungFuSnolafState::Dead );
			m_pSnolafGuards[1]->RequestStateChange( KungFuSnolafState::Dead );
			m_pSnolafGuards[2]->RequestStateChange( KungFuSnolafState::Dead );
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
		auto p3000TonSkel = p3000Ton->GetComponent<kbSkeletalModelComponent>();
		p3000TonSkel->RegisterAnimEventListener( this );
	}

	virtual void UpdateState_Internal() override {
		
		if ( g_SkipCheat == KungFuGame::Skip_ToEnd ) {
			if ( g_pCannonGame->GetPlayer()->IsDead() ) {
				RequestStateChange( KungFuGame::PlayerDead );
			}
		}

		static const kbString sSquashSnolafs( "Squash Snolafs");

		auto pSheep = KungFuLevelComponent::Get()->GetSheep();
		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		
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
				p3000Ton->PlayAnimation( sSquashSnolafs, 0.15f );

				break;
			}

			case TreyTonPounce : {

				if ( p3000Ton->HasFinishedAnim( sSquashSnolafs ) ) {
					ChangeState( TreyTonTitle );
				}
				break;
			}

			case TreyTonTitle : {

				break;
			}
		}
	}

	virtual void EndState_Internal( KungFuGame::eKungFuGame_State nextstate ) {
		auto p3000Ton = KungFuLevelComponent::Get()->Get3000Ton();
		auto p3000TonSkel = p3000Ton->GetComponent<kbSkeletalModelComponent>();
		p3000TonSkel->UnregisterAnimEventListener( this );
	}

private:

};