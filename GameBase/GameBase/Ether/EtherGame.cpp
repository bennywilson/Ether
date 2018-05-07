//===================================================================================================
// EtherGame.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbGame.h"
#include "kbTypeInfo.h"
#include "EtherGame.h"
#include "kbIntersectionTests.h"
#include "EtherSkelModel.h"
#include "EtherPlayer.h"
#include "EtherAI.h"
#include "EtherWeapon.h"

// oculus
#include "OVR_CAPI_D3D.h"
#include "OVR_Math.h"
using namespace OVR;

kbConsoleVariable g_NoEnemies( "noenemies", false, kbConsoleVariable::Console_Bool, "Remove enemies", "" );
kbConsoleVariable g_LockMouse( "lockmouse", 0, kbConsoleVariable::Console_Int, "Locks mouse", "" );

EtherGame * g_pEtherGame = nullptr;

// Stimpack
const float g_SlomoLength = 7.0f;

// Airstrike
const float g_AirstrikeDurationSec = 7.0f;
const float g_TimeBetweenBombers = 1.5f;
const float g_TimeBetweenBombs = 0.075f;

const kbVec3 g_CountUIScale( 0.5f, 0.5f, 0.5f );
static kbVec3 g_CountUIOffset( 14.96f, -12.0f, 10.0f );

/**
 *	EtherUIButton::EtherUIButton
 */
EtherUIButton::EtherUIButton() :
	m_pButtonEntity( nullptr ),
	m_pButtonModel( nullptr ),
	m_LocalPosition( kbVec3::zero ),
	m_pShader( nullptr ),
	m_Count( -1 ) {

	memset( m_pCountEntity, 0, sizeof( m_pCountEntity ) );
	memset( m_pCountModel, 0, sizeof( m_pCountModel ) );
}

/**
 *	EtherUIButton::LoadButton
 */
void EtherUIButton::LoadButton( const std::string & ModelName, const kbVec3 & localPosition ) {
	kbErrorCheck( m_pButtonEntity == nullptr, "EtherUIButton::LoadButton() - Called on an already initialized button" );

	m_pButtonEntity = new kbGameEntity();

	m_pButtonModel = (kbModel*)g_ResourceManager.GetResource( ModelName.c_str(), true );
	kbErrorCheck( m_pButtonModel != nullptr, "EtherUIButton::LoadButton() - Failed to load model %s", ModelName.c_str() );

	m_pShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbShader", true );
	kbErrorCheck( m_pShader != nullptr, "EtherUIButton::LoadButton() - Failed to shader" );
	
	g_pRenderer->AddRenderObject( m_pButtonEntity->GetComponent( 0 ), m_pButtonModel, kbVec3::zero, kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3( 1.0f, 1.0f, 1.0f ), RP_InWorldUI );

	m_LocalPosition = localPosition;


	for ( int i = 0; i < 4; i++ ) {
		m_pCountEntity[i] = new kbGameEntity();

		const std::string countModel = "./assets/FX/" + std::to_string( i ) + ".ms3d";
		m_pCountModel[i] = (kbModel*)g_ResourceManager.GetResource( countModel.c_str(), true );
		kbErrorCheck( m_pCountModel[i] != nullptr, "EtherUIButton::LoadButton() - Failed to load model %s", countModel.c_str() );
	}

	SetCount( 0 );
}

/**
 *	EtherUIButton::ShutdownButton
 */
void EtherUIButton::ShutdownButton() {

	if ( m_pButtonEntity != nullptr ) {
		g_pRenderer->RemoveRenderObject( m_pButtonEntity->GetComponent( 0 ) );
	}
	delete m_pButtonEntity;
	m_pButtonEntity = nullptr;
	m_pButtonModel = nullptr;

	for ( int i = 0; i < 4; i++ ) {
		if ( i == m_Count && m_pCountEntity[i] != nullptr ) {
			g_pRenderer->RemoveRenderObject( m_pCountEntity[i]->GetComponent( 0 ) );
		}

		delete m_pCountEntity[i];
		m_pCountEntity[i] = nullptr;
		m_pCountModel[i] = nullptr;
	}
}

/**
 *	EtherUIButton::Update
 */
void EtherUIButton::Update( const float deltaTimeSec ) {

	if ( m_pButtonEntity == nullptr ) {
		return;
	}

	static float xAxisLength = 400.0f;
	static float yAxisLength = xAxisLength * ( 900.0f / 1600.0f );		// Hack - Hardcoded numbers
	static float zAxisLength = 275.0f;

	kbVec3 centerEyePos = g_pEtherGame->GetCamera().m_Position;
	if ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) {
		const ovrPosef *const hmdEyePos = g_pRenderer->GetOvrEyePose();
		centerEyePos += g_pEtherGame->GetHMDWorldOffset() + ( ovrVecTokbVec3( hmdEyePos[0].Position ) + ovrVecTokbVec3( hmdEyePos[1].Position ) ) * 0.5f;
	}

	kbMat4 eyeMatrix;
	if ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) {
		eyeMatrix = g_pRenderer->GetEyeMatrices()[0];
		eyeMatrix.InvertFast();
	} else {
		eyeMatrix = g_pEtherGame->GetCamera().m_Rotation.ToMat4();
		eyeMatrix.InvertFast();
	}

	const kbVec3 rightVec = eyeMatrix[0].ToVec3();
	const kbVec3 upVec = eyeMatrix[1].ToVec3();

	kbVec3 button3DPos = centerEyePos + zAxisLength * eyeMatrix[2].ToVec3();
	button3DPos += rightVec * m_LocalPosition.x;
	button3DPos += upVec * m_LocalPosition.y;


	kbVec3 scale( 1.0f, 1.0f, 1.0f );

	if ( g_pRenderer->IsRenderingToHMD() ) {
		scale.Set( 1.5f, 1.5f, 1.5f );
	}

	if ( IsHighlighted() ) {
		scale *= 1.5f;
	}

	kbMat4 invCam = ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) ? ( g_pRenderer->GetEyeMatrices()[0] ) : ( g_pEtherGame->GetCamera().m_Rotation.ToMat4() );
	invCam.InvertFast();
	kbMat4 stimPackMatrix = kbMat4::identity;
	stimPackMatrix[0].Set( invCam[0].x, invCam[0].y, invCam[0].z, 0.0f );
	stimPackMatrix[1].Set( invCam[1].x, invCam[1].y, invCam[1].z, 0.0f );
	stimPackMatrix[2].Set( invCam[2].x, invCam[2].y, invCam[2].z, 1.0f );
	m_pButtonEntity->SetPosition( button3DPos );
	m_pButtonEntity->SetOrientation( kbQuatFromMatrix( stimPackMatrix ) );

	std::vector<kbShader *> ShaderOverrideList;
	ShaderOverrideList.push_back( m_pShader );
	g_pRenderer->UpdateRenderObject( m_pButtonEntity->GetComponent( 0 ), m_pButtonModel, button3DPos, kbQuatFromMatrix( stimPackMatrix ), scale, RP_InWorldUI, &ShaderOverrideList );
	
	if ( m_Count >= 0 && m_Count < 4 ) {
		g_pRenderer->UpdateRenderObject( m_pCountEntity[m_Count]->GetComponent( 0 ), m_pCountModel[m_Count], button3DPos + rightVec * g_CountUIOffset.x + upVec * g_CountUIOffset.y, kbQuatFromMatrix( stimPackMatrix ), g_CountUIScale * scale.x, RP_InWorldUI, &ShaderOverrideList );
	}
}

/**
 *	EtherUIButton::SetCount
 */
void EtherUIButton::SetCount( const int count ) {

	if ( m_Count >= 0 && m_Count < 4 ) {
		g_pRenderer->RemoveRenderObject( m_pCountEntity[m_Count]->GetComponent( 0 ) );
	}

	m_Count = count;
	if ( m_Count >= 0 && m_Count < 4 ) {
		g_pRenderer->AddRenderObject( m_pCountEntity[m_Count]->GetComponent( 0 ), m_pCountModel[m_Count], kbVec3::zero, kbQuat( 0.0f, 0.0f, 0.0f, 1.0f ), kbVec3( 1.0f, 1.0f, 1.0f ), RP_InWorldUI );
	}
}

/**
 *	EtherUIButton::IsHighlighted
 */
bool EtherUIButton::IsHighlighted() const {

	kbGameEntity *const pLocalPlayer =  g_pEtherGame->GetLocalPlayer();
	if ( pLocalPlayer == nullptr ) {
		return false;
	}

	EtherActorComponent *const pPlayer = (EtherActorComponent*)pLocalPlayer->GetActorComponent();
	if ( pPlayer == nullptr ) {
		return false;
	}

	kbGameEntity *const pEquippedItem = pPlayer->GetEquippedItem();
	if ( pEquippedItem == nullptr ) {
		return false;
	}

	kbMat4 WeaponMatrix = g_pEtherGame->GetCrossHairLocalSpaceMatrix();

	const kbVec3 aimAtPoint = WeaponMatrix[3].ToVec3();
	const kbVec3 WeaponPos = pEquippedItem->GetOwner()->GetPosition();	
	const kbVec3 zAxis = ( aimAtPoint - WeaponPos ).Normalized();
	static float extent = 15.0f;

	if ( kbRayOBBIntersection( m_pButtonEntity->GetOrientation().ToMat4(), m_pButtonEntity->GetPosition(), WeaponPos, WeaponPos + zAxis * 1000.0f, -kbVec3( extent, extent, extent ), kbVec3( extent, extent, extent ) ) ) {
		return true;
	}

	return false;
}

/**
 *	EtherGame::EtherGame
 */
EtherGame::EtherGame() :
	m_CameraMode( Cam_FirstPerson ),
	m_pPlayerComponent( nullptr ),
	m_pWorldGenComponent( nullptr ),
	m_pCharacterPackage( nullptr ),
	m_pWeaponsPackage( nullptr ),
	m_pFXPackage( nullptr ),
	m_pTranslucentShader( nullptr ),
	m_HMDWorldOffset( kbVec3::zero ),
	m_CurrentGameState( GamePlay ),
	m_VerseIdx( 0 ),
	m_pCrossHairEntity( nullptr ),
	m_SlomoStartTime( -1.0f ),
	m_pSlomoSound( nullptr ),
	m_AirstrikeTimeLeft( -120.0f ),
	m_BombersLeft( 0 ),
	m_NextBomberSpawnTime( 0.0f ),
	m_pELBomberModel( nullptr ),
	m_pAirstrikeFlybyWave( nullptr ),
	m_OLCTimer( -1.0f ),
	m_OLCPostProcess( -1.0f ),
	m_pOLCWindupWave( nullptr ),
	m_pOLCExplosion( nullptr ),
	m_OLCTint( kbVec3::zero ) {

	m_ELBomberEntity[0] = m_ELBomberEntity[1] = m_ELBomberEntity[2] = nullptr;
	m_BombTimer[0] = m_BombTimer[1] = m_BombTimer[2] = 0.0f;

	m_Camera.m_Position.Set( 0.0f, 2600.0f, 0.0f );

	if ( g_pEtherGame != nullptr ) {
		kbError( "EtherGame::EtherGame() - g_pEtherGame is not nullptr" );
	}
	g_pEtherGame = this;
}

/**
 *	EtherGame::~EtherGame
 */
EtherGame::~EtherGame() {
	if ( g_pEtherGame == nullptr ) {
		kbError( "EtherGame::~EtherGame() - g_pEtherGame is nullptr" );
	}
	g_pEtherGame = nullptr;
}

/**
 *	EtherGame::PlayGame_Internal
 */
void EtherGame::PlayGame_Internal() {

	// Add cross hair entity/model to the scene
	m_pCrossHairEntity = new kbGameEntity();
	kbModel *const pCrossHairModel = (kbModel*)g_ResourceManager.GetResource( "./assets/FX/crosshair.ms3d", true );
	std::vector<kbShader *> ShaderOverrideList;
	ShaderOverrideList.push_back( m_pTranslucentShader );
	g_pRenderer->AddRenderObject( m_pCrossHairEntity->GetComponent( 0 ), pCrossHairModel, kbVec3::zero, kbQuat(), kbVec3( 1.0f, 1.0f, 1.0f ), RP_InWorldUI, &ShaderOverrideList );

	// Set up UI buttons
	static float XPos = g_pRenderer->IsRenderingToHMD() ? ( 100.0f ) : ( 200.0f );
	static float YPos = g_pRenderer->IsRenderingToHMD() ? ( -175.0f ) : ( -175.0f );
	m_UIButtons[Airstrike].LoadButton( "./assets/FX/airstrike.ms3d", kbVec3( -XPos, YPos, 100.0f ) );
	m_UIButtons[Stimpack].LoadButton( "./assets/FX/stimpack.ms3d", kbVec3( -0.0, YPos, 100.0f ) );
	m_UIButtons[OLC].LoadButton( "./assets/FX/olc.ms3d", kbVec3( XPos, YPos, 100.0f ) );

	m_UIButtons[Stimpack].SetCount( m_pPlayerComponent->GetNumStimPacks() );
	m_UIButtons[Airstrike].SetCount( m_pPlayerComponent->GetNumAirstrikes() );
	m_UIButtons[OLC].SetCount( m_pPlayerComponent->GetNumOLC() );

	// Create air strike bombers
	kbPackage *const pVehiclePackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Vehicles.kbPkg" );
	kbErrorCheck( pVehiclePackage != nullptr, "Unable to Vehicle character package." );

	const kbGameEntity *const pBomberPrefab = pVehiclePackage->GetPrefab( "EL_Bomber" )->GetGameEntity( 0 );
	kbErrorCheck( pBomberPrefab != nullptr, "Unable to find bomber prefab" );

	for ( int i = 0; i < 3; i++ ) {
		m_ELBomberEntity[i] = CreateEntity( pBomberPrefab );

		// Hack - billboard type isn't being set correctly from data.  So force it here
		kbParticleComponent *const pParticleComp = (kbParticleComponent*)  m_ELBomberEntity[i]->GetComponentByType( kbParticleComponent::GetType() );
		if ( pParticleComp != nullptr ) {
			pParticleComp->SetBillboardType( BT_AxialBillboard );
		}
	}

	m_pAirstrikeFlybyWave = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/airstrike.wav", true );
	kbErrorCheck( m_pAirstrikeFlybyWave != nullptr, "EtherGame::PlayGame_Internal() - Failed to find airstrike.wav" );

	m_pOLCWindupWave = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/olc_windup.wav", true );
	m_pOLCExplosion = (kbWaveFile *)g_ResourceManager.GetResource( "./assets/Sounds/olc_explosion.wav", true );

	g_pRenderer->LoadTexture( "./assets/UI/TitleScreen.jpg", 5 );
	g_pRenderer->LoadTexture( "./assets/UI/verse1.jpg", 6 );
	g_pRenderer->LoadTexture( "./assets/UI/verse2.jpg", 7 );
	g_pRenderer->LoadTexture( "./assets/UI/verse3.jpg", 8 );
}

/**
 *	EtherGame::InitGame_Internal
 */
void EtherGame::InitGame_Internal() {

	m_AIManager.Initialize();

	m_pCharacterPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Characters.kbPkg" );
	kbErrorCheck( m_pCharacterPackage != nullptr, "Unable to find character package." );

	m_pWeaponsPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/Weapons.kbPkg" );
	kbErrorCheck( m_pWeaponsPackage != nullptr, "Unable to find weapons package." );

	m_pFXPackage = (kbPackage*) g_ResourceManager.GetPackage( "./assets/Packages/FX.kbPkg" );
	kbErrorCheck( m_pFXPackage != nullptr, "Unable to find FX package." );

	const kbPrefab *const pBaseMuzzleFlashPrefab = m_pFXPackage->GetPrefab( "BaseMuzzleFlash" );
	if ( pBaseMuzzleFlashPrefab != nullptr && pBaseMuzzleFlashPrefab->GetGameEntity(0) != nullptr ) {
		const kbGameEntity *const pCurParticleEntity = pBaseMuzzleFlashPrefab->GetGameEntity(0);
		for ( int i = 1; i < pCurParticleEntity->NumComponents(); i++ ) {
			if ( pCurParticleEntity->GetComponent( i )->IsA( kbParticleComponent::GetType() ) == false ) {
				continue;
			}

			const kbParticleComponent *const pParticleComponent = static_cast<kbParticleComponent*>( pCurParticleEntity->GetComponent( i ) );
			m_pParticleManager->PoolParticleComponent( pParticleComponent, 16 );
		}
	}
	m_GameStartTimer.Reset();

	m_pTranslucentShader = (kbShader*)g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/basicTranslucency.kbShader", true );
	m_pSlomoSound = (kbWaveFile*)g_ResourceManager.GetResource( "./assets/Sounds/slomo.wav", true );
}

/**
 *	EtherGame::StopGame_Internal
 */
void EtherGame::StopGame_Internal() {
	m_pPlayerComponent = nullptr;
	m_pLocalPlayer = nullptr;

	if ( m_pCrossHairEntity != nullptr ) {
		g_pRenderer->RemoveRenderObject( m_pCrossHairEntity->GetComponent( 0 ) );
		delete m_pCrossHairEntity;
		m_pCrossHairEntity = nullptr;
	}

	for ( int i = 0; i < NUM_UI_BUTTONS; i++ ) {
		m_UIButtons[i].ShutdownButton();
	}

	for ( int i = 0; i < 3; i++ ) {
		RemoveGameEntity( m_ELBomberEntity[i] );
	}
}

/**
 *	EtherGame::LevelLoaded_Internal
 */
void EtherGame::LevelLoaded_Internal() {

	kbGameEntity *const pPlayerEntity = CreatePlayer( 0, m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID(), kbVec3::zero );

	if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
		kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
		pEntity->SetPosition( pEntity->GetPosition() + kbVec3( 0.0f, 20000.0f, 0.0f ) );
	}

	m_pParticleManager->SetCustomParticleTextureAtlas( "./assets/FX/fx_atlas.jpg" );
}

/**
 *	EtherGame::Update_Internal
 */
void EtherGame::Update_Internal( float DT ) {

	if ( m_CurrentGameState == TitleScreen ) {
		UpdateTitleScreen( DT );
		m_Camera.m_Position = m_pLocalPlayer->GetPosition();
		g_pRenderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );
		return;
	} else if ( m_CurrentGameState == VerseScreen ) {
		UpdateVerseScreen( DT );
		m_Camera.m_Position = m_pLocalPlayer->GetPosition();
		g_pRenderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );
		return;
	}

	if ( IsConsoleActive() == false ) {
		ProcessInput( DT );
	}
	
	if ( g_LockMouse.GetInt() == 0 ) {
		g_pInputManager->SetMouseBehavior( kbInputManager::MB_LockToWindow );
		UpdateMotionControls( DT );
	} else {
		g_pInputManager->SetMouseBehavior( kbInputManager::MB_LockToCenter );
	}
	
	if ( ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) && g_pRenderer->GetFrameNum() > 0 ) {
	
		kbVec3 eyePos[2];
	
		const ovrPosef * eyeRenderPose = g_pRenderer->GetOvrEyePose();
		float eyeX = 200.0f * ( ( eyeRenderPose[0].Position.x + eyeRenderPose[1].Position.x ) * 0.5f );
	
		kbMat4 camMatrix = m_Camera.m_Rotation.ToMat4();
		m_HMDWorldOffset = camMatrix[0].ToVec3() * eyeX;
	
		for ( int eye = 0; eye < 2; eye++ ) {
			Vector3f curEyePos = eyeRenderPose[eye].Position;
			curEyePos.x += eyeX;
	
			if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
				kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
				EtherWeaponComponent *const pPlayerWeapon = static_cast<EtherWeaponComponent*>( pEntity->GetComponentByType( EtherWeaponComponent::GetType() ) );
				if ( pPlayerWeapon != nullptr ) {
	
					kbVec3 curPos( 5.5f, -10.0f, 3.0f );	// Weapon offset from camera
					curPos += m_HMDWorldOffset; 
					kbTransformComponent *const pTrans = static_cast<kbTransformComponent*>( pPlayerWeapon->GetOwner()->GetComponent(0) );
					pTrans->SetPosition( kbVec3( curPos.x, curPos.y, curPos.z ) );
				}
			}
	
			eyePos[eye] = ovrVecTokbVec3( curEyePos );
		}
	
		// Update renderer cam
		g_pRenderer->SetRenderViewTransform( nullptr, m_Camera.m_Position + m_HMDWorldOffset, m_Camera.m_Rotation );
	
	} else {
		if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
			kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
			EtherWeaponComponent *const pPlayerWeapon = static_cast<EtherWeaponComponent*>( pEntity->GetComponentByType( EtherWeaponComponent::GetType() ) );
			if ( pPlayerWeapon != nullptr ) {
		
				kbVec3 curPos( 5.5f, -10.0f, 3.0f );	// Weapon offset from camera
				kbTransformComponent *const pTrans = static_cast<kbTransformComponent*>( pPlayerWeapon->GetOwner()->GetComponent(0) );
				pTrans->SetPosition( kbVec3( curPos.x, curPos.y, curPos.z ) );
			}
		}

		// Update renderer cam
		g_pRenderer->SetRenderViewTransform( nullptr, m_Camera.m_Position, m_Camera.m_Rotation );
	}
	
	std::string PlayerPos;
	PlayerPos += "x:";
	PlayerPos += std::to_string( m_Camera.m_Position.x );
	PlayerPos += " y:";
	PlayerPos += std::to_string( m_Camera.m_Position.y );
	PlayerPos += " z:";
	PlayerPos += std::to_string( m_Camera.m_Position.z );
	
	//g_pRenderer->DrawDebugText( PlayerPos, 0, 0, g_DebugTextSize, g_DebugTextSize, kbColor::green );


	UpdateWorld( DT );

	for ( int i = 0; i < NUM_UI_BUTTONS; i++ ) {
		m_UIButtons[i].Update( DT );
	}

	UpdatePostProcess();
}

/**
 *	EtherGame::AddGameEntity_Internal
 */
void EtherGame::AddGameEntity_Internal( kbGameEntity *const pEntity ) {
	if ( pEntity == nullptr ) {
		kbLog( "EtherGame::AddGameEntity_Internal() - nullptr Entity" );
		return;
	}

	if ( pEntity == m_pLocalPlayer && m_pWorldGenComponent != nullptr ) {
		kbVec3 groundPt;
		if ( TraceAgainstWorld( pEntity->GetPosition() + kbVec3( 0.0f, 10000.0f, 0.0f ), pEntity->GetPosition() - kbVec3( 0.0f, 10000.0f, 0.0f ), groundPt, false ) ) {
			m_Camera.m_Position.y = groundPt.y + 256.0f;
			m_pLocalPlayer->SetPosition( m_Camera.m_Position );

			const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
			m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
		}
	}
}

/**
 *	EtherGame::ProcessInput
 */
void EtherGame::ProcessInput( const float DT ) {

	if ( m_pLocalPlayer == nullptr || m_pLocalPlayer->GetActorComponent()->IsDead() ) {
		return;
	}

	static bool bCursorHidden = false;
	static bool bWindowIsSelected = true;
	static bool bFirstRun = true;

	if ( bFirstRun ) {
		ShowCursor( false );
		bFirstRun = false;
	}

	if ( GetAsyncKeyState( VK_SPACE ) ) {
		const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
		m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
		m_Camera.m_Rotation = kbQuatFromMatrix( orientation );
		m_Camera.m_RotationTarget = kbQuatFromMatrix( orientation );

		POINT CursorPos;
		GetCursorPos( &CursorPos );
		RECT rc;
 		GetClientRect( m_Hwnd, &rc );
		SetCursorPos( (rc.right - rc.left ) / 2, (rc.bottom - rc.top) / 2 );
	}

	static bool bCameraChanged = false;
	if ( GetAsyncKeyState( 'C' ) && GetAsyncKeyState( VK_CONTROL ) ) {
		if ( bCameraChanged == false ) {
			bCameraChanged = true;
			m_CameraMode = eCameraMode_t( 1 + (int)m_CameraMode );
			if ( m_CameraMode >= Cam_Free ) {
				m_CameraMode = Cam_FirstPerson;
			}

			if ( m_pLocalPlayer != nullptr ) {
				for ( int i = 0; i < m_pLocalPlayer->NumComponents(); i++ ) {

					if ( m_pLocalPlayer->GetComponent(i)->IsA( EtherSkelModelComponent::GetType() ) ) {
						EtherSkelModelComponent *const SkelComp = static_cast<EtherSkelModelComponent*>( m_pLocalPlayer->GetComponent(i) );
						if ( m_CameraMode == Cam_FirstPerson ) {
							SkelComp->Enable( SkelComp->IsFirstPersonModel() );
						} else {
							SkelComp->Enable( !SkelComp->IsFirstPersonModel() );
						}
					}
				}
			}
		}
	} else {
		bCameraChanged = false;
	}

	if ( g_LockMouse.GetInt() <= 1 ) {
		if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != nullptr ) {
			m_pPlayerComponent->HandleMovement( GetInput(), DT );
		}
	}

	m_Camera.Update();

	if ( GetForegroundWindow() == this->m_Hwnd && m_pPlayerComponent != nullptr ) {
		m_pPlayerComponent->HandleAction( GetInput() );
	}
}

/**
 *	EtherGame::TraceAgainstWorld
 */
bool EtherGame::TraceAgainstWorld( const kbVec3 & startPt, const kbVec3 & endPt, kbVec3 & collisionPt, const bool bTraceAgainstDynamicCollision ) {

	if ( m_pWorldGenComponent == nullptr ) {
		return false;
	}

	EtherWorldGenCollision_t HitResult;
	m_pWorldGenComponent->TraceAgainstWorld( HitResult, startPt, endPt, bTraceAgainstDynamicCollision );
	if ( HitResult.m_bHitFound ) {
		collisionPt = HitResult.m_HitLocation;
	}

	return HitResult.m_bHitFound;
}

/**
 *	EtherGame::CoverObjectsPointTest
 */
bool EtherGame::CoverObjectsPointTest( const EtherCoverObject *& OutCoverObject, const kbVec3 & startPt ) const {
	if ( m_pWorldGenComponent == nullptr ) {
		return false;
	}

	return m_pWorldGenComponent->CoverObjectsPointTest( OutCoverObject, startPt );
}

/**
 *	EtherGame::MoveActorAlongGround
 */
void EtherGame::MoveActorAlongGround( EtherActorComponent *const pActor, const kbVec3 & startPt, const kbVec3 & endPt ) {

	kbErrorCheck( pActor != nullptr, "EtherGame::MoveActorAlongGround() - nullptr actor passed in" );

	if ( m_pWorldGenComponent == nullptr ) {
		return;
	}

	m_pWorldGenComponent->MoveActorAlongGround( pActor, startPt, endPt );
}

/**
 *	EtherGame::UpdateTitleScreen
 */
static float dist = 11.0f;
static float sizex = 16.0f;
static float sizey = 9.0f;

void EtherGame::UpdateTitleScreen( const float DeltaTimeSec ) {

	// Place on ground
	const kbMat4 orientation( kbVec4( 0.0f, 0.0f, -1.0f, 0.0f ), kbVec4( 0.0f, 1.0f, 0.0f, 0.0f ), kbVec4( -1.0f, 0.0f, 0.0f, 0.0f ), kbVec3::zero );
	m_pLocalPlayer->SetOrientation( kbQuatFromMatrix( orientation ) );
	m_Camera.m_Rotation = kbQuatFromMatrix( orientation );
	m_Camera.m_RotationTarget = kbQuatFromMatrix( orientation );

	g_pRenderer->DrawBillboard( m_pLocalPlayer->GetPosition() + m_pLocalPlayer->GetOrientation().ToMat4()[2].ToVec3() * dist, kbVec2( sizex, sizey ), 5, nullptr );

	if ( GetAsyncKeyState( VK_SPACE ) ) {
		POINT CursorPos;
		GetCursorPos( &CursorPos );
		RECT rc;
 		GetClientRect( m_Hwnd, &rc );  
		SetCursorPos( (rc.right - rc.left ) / 2, (rc.bottom - rc.top) / 2 );

		m_CurrentGameState = VerseScreen;
		m_GameStartTimer.Reset();

		m_VerseIdx = rand() % 3;
	}
}

void EtherGame::UpdateVerseScreen( const float DeltaTimeSec ) {
	g_pRenderer->DrawBillboard( m_pLocalPlayer->GetPosition() + m_pLocalPlayer->GetOrientation().ToMat4()[2].ToVec3() * dist, kbVec2( sizex, sizey ), 5 + m_VerseIdx, nullptr );

	if ( m_GameStartTimer.TimeElapsedSeconds() > 3 && GetAsyncKeyState( VK_SPACE ) ) {
		m_CurrentGameState = GamePlay;
		m_GameStartTimer.Reset();

		if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
			kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
		//	pEntity->SetPosition( pEntity->GetPosition() - kbVec3( 0.0f, 20000.0f, 0.0f ) );
		}
	}
}

/**
 *	EtherGame::UpdateWorld
 */
void EtherGame::UpdateWorld( const float DT ) {
	const double GameTimeInSeconds = m_GameStartTimer.TimeElapsedSeconds();

	m_AIManager.Update( DT );
	int numEntities = 0;
	for ( int iEntity = 0; iEntity < GetGameEntities().size(); iEntity++ ) {
		for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
			if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( EtherEnemySoldierAIComponent::GetType() ) ) {
				numEntities++;
			}
		}
	}

	if ( g_NoEnemies.GetBool() ) {
		for ( int iEntity = (int)GetGameEntities().size() - 1; iEntity >= 0; iEntity-- ) {
			for ( int iComp = 0; iComp < GetGameEntities()[iEntity]->NumComponents(); iComp++ ) {
				if ( GetGameEntities()[iEntity]->GetComponent( iComp )->IsA( EtherEnemySoldierAIComponent::GetType() ) ) {
					g_pGame->RemoveGameEntity( GetGameEntities()[iEntity] );
				}
			}
		}
	}

	if ( GameTimeInSeconds > 7.0f && GetPlayersList().size() > 0 && numEntities < 1 && g_NoEnemies.GetBool() == false ) {
		static float XOffsetRange = 500.0f;
		static float ZOffsetRange = 1750.0;
		const float xOffset = ( XOffsetRange * 0.33f + kbfrand() * ( XOffsetRange * 0.667f ) ) * ( ( kbfrand() < 0.5f ) ? ( 1.0f ) : ( -1.0f ) );
		const float zOffset = ( ZOffsetRange * 0.33f + kbfrand() * ( ZOffsetRange * 0.667f ) );
		const kbVec3 spawnLocation = GetPlayersList()[0]->GetPosition() + kbVec3( xOffset, 0.0f, zOffset );

		const kbGameEntity *const pGameEntity = m_pCharacterPackage->GetPrefab( "Cartel_Warrior" )->GetGameEntity( 0 );
		kbGameEntity *const pEntity = CreateEntity( pGameEntity );

		pEntity->SetPosition( spawnLocation );
	}

	UpdateStimPack( DT );
	UpdateAirstrike( DT );
	UpdateOLC( DT );
}

/**
 *	EtherGame::CreatePlayer
 */
kbGameEntity * EtherGame::CreatePlayer( const int netId, const kbGUID & prefabGUID, const kbVec3 & DesiredLocation ) {

	kbGameEntity * pNewEntity = nullptr;

	if ( prefabGUID == m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0)->GetGUID() ) {
		// Create players

		kbLog( "Creating player with id %d", netId );

		pNewEntity = g_pGame->CreateEntity( m_pCharacterPackage->GetPrefab( "Angelica" )->GetGameEntity(0), true );

		AddPrefabToEntity( m_pWeaponsPackage, "EL_Rifle", pNewEntity, false );
		AddPrefabToEntity( m_pWeaponsPackage, "EL_Grenade", pNewEntity, false );
		AddPrefabToEntity( m_pCharacterPackage, "FP_Hands", pNewEntity, false );

		for ( int i = 0; i < pNewEntity->NumComponents(); i++ ) {
			kbComponent *const pCurComponent = pNewEntity->GetComponent(i);
			if ( pCurComponent->IsA( EtherPlayerComponent::GetType() ) ) {
				m_pPlayerComponent = static_cast<EtherPlayerComponent*>( pCurComponent );
				kbErrorCheck( m_pPlayerComponent != nullptr, "EtherGame::CreatePlayer() - Failed to find player component" );
			} else if ( pCurComponent->IsA( EtherSkelModelComponent::GetType() ) ) {
				EtherSkelModelComponent *const pSkelModel = static_cast<EtherSkelModelComponent*>( pCurComponent );
				pSkelModel->Enable( pSkelModel->IsFirstPersonModel() );
			}
		}

		// Place on ground
		if ( m_pWorldGenComponent != nullptr ) {
			const kbVec3 desiredStartLocation = kbVec3::zero;
			kbVec3 groundPt;
			if ( TraceAgainstWorld( desiredStartLocation + kbVec3( 0.0f, 10000.0f, 0.0f ), desiredStartLocation - kbVec3( 0.0f, 10000.0f, 0.0f ), groundPt, false ) ) {
				pNewEntity->SetPosition( groundPt );
			} else {
				pNewEntity->SetPosition( DesiredLocation );
			}
		}

		m_pLocalPlayer = pNewEntity;

	} else {

		pNewEntity = g_pGame->CreateEntity( g_ResourceManager.GetGameEntityFromGUID( prefabGUID ), false );
		pNewEntity->SetPosition( DesiredLocation );
	}

	return pNewEntity;
}

/**
 *	EtherGame::AddPrefabToEntity
 */
void EtherGame::AddPrefabToEntity( const kbPackage *const pPrefabPackage, const std::string & prefabName, kbGameEntity *const pEntity, const bool bComponentsOnly ) {
	if ( pPrefabPackage == nullptr ) {
		kbError( "EtherGame::AddPrefabToEntity() - nullptr prefab package found while searching for %s", prefabName.c_str() );
		return;
	}

	const kbPrefab *const pPrefab = pPrefabPackage->GetPrefab( prefabName );
	if ( pPrefab == nullptr || pPrefab->GetGameEntity(0) == nullptr ) {
		kbError( "EtherGame::AddPrefabToEntity() - Null prefab or none Entities found with name %s", prefabName.c_str() );
		return;
	}

	const kbGameEntity *const pPrefabEntity = pPrefab->GetGameEntity(0);
	if ( bComponentsOnly == false ) {
		kbGameEntity *const newItem = new kbGameEntity( pPrefabEntity, false );
		pEntity->AddEntity( newItem );
		for ( int i = 0; i < pEntity->NumComponents(); i++ ) {
			if ( pEntity->GetComponent(i)->IsA( EtherActorComponent::GetType() ) ) {
				if ( static_cast<EtherActorComponent*>( pEntity->GetComponent(i) )->GetEquippedItem() == nullptr ) {
					static_cast<EtherActorComponent*>( pEntity->GetComponent(i) )->SetEquippedItem( newItem );
				}
			}
		}
		return;
	}

	for ( int iComp = 1; iComp < pPrefabEntity->NumComponents(); iComp++ ) {

		bool bShowComponent = true;
		const kbComponent *const pPrefabComponent = pPrefabEntity->GetComponent(iComp);
		if ( pPrefabComponent->IsA( EtherSkelModelComponent::GetType() ) ) {
			const EtherSkelModelComponent *const skelModel = static_cast<const EtherSkelModelComponent*>( pPrefabComponent );
			if ( skelModel->IsFirstPersonModel() && g_pGame->GetLocalPlayer() != pEntity ) {
				continue;
			}

			if ( skelModel->IsFirstPersonModel() == false && g_pGame->GetLocalPlayer() == pEntity ) {
				bShowComponent = false;
			}
		} else if ( pPrefabComponent->IsA( EtherPlayerComponent::GetType() ) && pEntity != g_pGame->GetLocalPlayer() ) {
			continue;
		}

		kbComponent *const pNewComponent = pPrefabComponent->Duplicate();
		pEntity->AddComponent( pNewComponent );
		pNewComponent->Enable( false );
		if ( bShowComponent ) {
			pNewComponent->Enable( true );
		}
	}
}

/**
 *	EtherGame::UpdateMotionControls
 */
void EtherGame::UpdateMotionControls( const float deltaTimeSec ) {

	if ( m_pLocalPlayer == nullptr || m_pLocalPlayer->GetActorComponent()->IsDead() ) {
		return;
	}

	RECT clientRect;
	GetClientRect( m_Hwnd, &clientRect );

	const kbVec2 CursorPos( ( float )m_InputManager.GetMouseCursorPosition().x, ( float )m_InputManager.GetMouseCursorPosition().y );

	kbMat4 weaponMatrix;

	EtherWeaponComponent * pPlayerWeapon = nullptr;
	if ( m_pLocalPlayer != nullptr && m_pLocalPlayer->GetChildEntities().size() > 0 ) {
		kbGameEntity *const pEntity = m_pLocalPlayer->GetChildEntities()[0];
		pPlayerWeapon = static_cast<EtherWeaponComponent*>( pEntity->GetComponentByType( EtherWeaponComponent::GetType() ) );
	}

	kbVec3 cursorScreenSpacePos( 2.0f * ( CursorPos.x / clientRect.right ) - 1.0f, -( ( 2.0f * CursorPos.y /  clientRect.bottom ) - 1.0f ), 0.0f );
	cursorScreenSpacePos.x = kbClamp( cursorScreenSpacePos.x, -1.0f, 1.0f );
	cursorScreenSpacePos.y = kbClamp( cursorScreenSpacePos.y, -1.0f, 1.0f );
	
	kbMat4 eyeMatrix;
	if ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly()  ) {
		eyeMatrix = g_pRenderer->GetEyeMatrices()[0];
		eyeMatrix.InvertFast();
	} else {
		eyeMatrix = m_Camera.m_Rotation.ToMat4();
		eyeMatrix.InvertFast();
	}

	const kbVec3 rightVec = eyeMatrix[0].ToVec3();
	const kbVec3 upVec = eyeMatrix[1].ToVec3();
	static float xAxisLength = 400.0f;
	static float yAxisLength = xAxisLength * ( 900 / 1600.0f );
	static float zAxisLength = 275.0f;

	kbVec3 centerEyePos = m_Camera.m_Position;
	if ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) {
		const ovrPosef *const hmdEyePos = g_pRenderer->GetOvrEyePose();
		centerEyePos +=  ( ovrVecTokbVec3( hmdEyePos[0].Position ) + ovrVecTokbVec3( hmdEyePos[1].Position ) ) * 0.5f;
	}

	kbVec3 crossHair3DPos = centerEyePos + zAxisLength * eyeMatrix[2].ToVec3();
	crossHair3DPos += rightVec * xAxisLength * cursorScreenSpacePos.x;
	crossHair3DPos += upVec * yAxisLength * cursorScreenSpacePos.y;

	kbMat4 invCam = ( g_pRenderer->IsRenderingToHMD() || g_pRenderer->IsUsingHMDTrackingOnly() ) ? ( g_pRenderer->GetEyeMatrices()[0] ) : ( m_Camera.m_Rotation.ToMat4() );
	invCam.InvertFast();

	static float crossHairWidth = 10.0f;
	static float crossHairHeight = 10.0f;

	{

		m_CrossHairLocalSpaceMatrix[0].Set( invCam[0].x, invCam[0].y, invCam[0].z, 0.0f );
		m_CrossHairLocalSpaceMatrix[1].Set( invCam[1].x, invCam[1].y, invCam[1].z, 0.0f );
		m_CrossHairLocalSpaceMatrix[2].Set( invCam[2].x, invCam[2].y, invCam[2].z, 1.0f );
	}

	kbModel *const pModel = (kbModel*)g_ResourceManager.GetResource( "./assets/FX/crosshair.ms3d", true );
	std::vector<kbShader *> ShaderOverrideList;
	ShaderOverrideList.push_back( m_pTranslucentShader );
	g_pRenderer->UpdateRenderObject( m_pCrossHairEntity->GetComponent( 0 ), pModel, crossHair3DPos, kbQuatFromMatrix( m_CrossHairLocalSpaceMatrix ), kbVec3( 1.0f, 1.0f, 1.0f ), RP_InWorldUI, &ShaderOverrideList );

	{
		const kbVec3 aimAtPoint = crossHair3DPos;//m_pParent->GetPosition() + 9999.0f * WeaponMatrix[2].ToVec3();
		const kbVec3 zAxis = ( aimAtPoint - pPlayerWeapon->GetOwner()->GetPosition() ).Normalized();
		const kbVec3 xAxis = kbVec3::up.Cross( zAxis ).Normalized();
		const kbVec3 yAxis = zAxis.Cross( xAxis ).Normalized();
	
		m_CrossHairLocalSpaceMatrix[0].Set( xAxis.x, xAxis.y, xAxis.z, 0.0f );
		m_CrossHairLocalSpaceMatrix[1].Set( yAxis.x, yAxis.y, yAxis.z, 0.0f );
		m_CrossHairLocalSpaceMatrix[2].Set( zAxis.x, zAxis.y, zAxis.z, 0.0f );
		m_CrossHairLocalSpaceMatrix[3].Set( crossHair3DPos.x, crossHair3DPos.y, crossHair3DPos.z, 1.0f );
	}

	// Orient first person gun towards the cross hair
	if ( pPlayerWeapon != nullptr ) {
		const kbQuat weaponOrientation = kbQuatFromMatrix( m_CrossHairLocalSpaceMatrix );
		pPlayerWeapon->GetOwner()->SetOrientation( weaponOrientation );

		if ( m_pPlayerComponent->GetFPHands() != nullptr ) {
			m_pPlayerComponent->GetFPHands()->GetOwner()->SetOrientation( kbQuatFromMatrix( invCam ) );
			// hack to get hands skel model to update rotation
			m_pPlayerComponent->GetFPHands()->Update( 0.016f );
		}
	}
}

/**
 *	EtherGame::UpdatePostProcess
 */
void EtherGame::UpdatePostProcess() {

	if ( m_pLocalPlayer == nullptr || m_pLocalPlayer->GetActorComponent() == nullptr ) {
		g_pRenderer->SetPostProcessSettings( kbPostProcessSettings_t() );
		return;
	}

	kbActorComponent *const playerActor = m_pLocalPlayer->GetActorComponent();

	const float MaxDeathScreenRed = 2.0f;
	const float t = playerActor->GetHealth() / playerActor->GetMaxHealth();
	const float reverseT = 1.0f - t;

	kbPostProcessSettings_t updatedPPSettings;
	updatedPPSettings.m_Tint.r = kbClamp( ( MaxDeathScreenRed - 1.0f ) * reverseT + 1.0f, 0.0f, 5.0f );
	updatedPPSettings.m_Tint.g = t;
	updatedPPSettings.m_Tint.b = t;

	if ( m_OLCPostProcess > 0.0f ) {
		updatedPPSettings.m_AdditiveColor.r = m_OLCPostProcess;
		updatedPPSettings.m_AdditiveColor.g = m_OLCPostProcess;
		updatedPPSettings.m_AdditiveColor.b = m_OLCPostProcess;
		updatedPPSettings.m_AdditiveColor.a = 0.0f;
	} else if ( m_SlomoStartTime > 0.0f ) {
		const float timeSinceSlomoActivated = g_GlobalTimer.TimeElapsedSeconds() - m_SlomoStartTime;
		const float blendTime = 0.5f;
		const float blendOutStartTime = g_SlomoLength - blendTime;

		static bool bBlendingOut = false;

		float finalBlend = 0.f;
		if ( timeSinceSlomoActivated < blendTime ) {
			finalBlend = timeSinceSlomoActivated / blendTime;

			const float freq = 1.0f - ( ( 1.0f - 0.05f ) * finalBlend );
			GetSoundManager().SetFrequencyRatio( 0.05f + freq );

			bBlendingOut = false;

		} else if ( timeSinceSlomoActivated > blendOutStartTime ) {
			finalBlend = 1.0f - ( timeSinceSlomoActivated - blendOutStartTime ) / blendTime;

			const float freq = 1.0f -  ( 1.0f - 0.05f ) * finalBlend;
			GetSoundManager().SetFrequencyRatio( 0.05f + freq );

		} else {
			finalBlend = 1.0f;
		}

		if ( bBlendingOut == false && timeSinceSlomoActivated > g_SlomoLength * 0.75f ){
			bBlendingOut = true;
			GetSoundManager().PlayWave( m_pSlomoSound, 1.0f );
		}

		updatedPPSettings.m_Tint.a = finalBlend;

	} else {
		updatedPPSettings.m_Tint.a = 0.0f;
	}

	updatedPPSettings.m_AdditiveColor.x += m_OLCTint.x;
	updatedPPSettings.m_AdditiveColor.y += m_OLCTint.y;
	updatedPPSettings.m_AdditiveColor.z += m_OLCTint.z;
	g_pRenderer->SetPostProcessSettings( updatedPPSettings );
}


/**
 *	EtherGame::PressHighlightedButton
 */
bool EtherGame::PressHighlightedButton() {

   if ( m_pPlayerComponent->IsDead() ) {
      return false;
   }

	EtherButtonTypes hitButton = NUM_UI_BUTTONS;
	for ( int i = 0; i < NUM_UI_BUTTONS; i++ ) {
		if ( m_UIButtons[i].IsHighlighted() ) {
			hitButton = (EtherButtonTypes)i;
			break;
		}
	}

	switch( hitButton ) {

		case Stimpack : {
			ActivateStimPack();
			break;
		}

		case Airstrike : {
			ActivateAirstrike();
			break;
		}

		case OLC : {
			ActivateOLC();
			break;
		}

		default : {
			return false;
		}
	}

	return true;
}

/**
 *	EtherGame::ActivateStimPack
 */
void EtherGame::ActivateStimPack() {

	if ( m_SlomoStartTime > 0.f || m_pPlayerComponent->GetNumStimPacks() == 0 ) {
	   return;
	}

	SetDeltaTimeScale( 0.2f );
	m_SlomoStartTime = g_GlobalTimer.TimeElapsedSeconds();

	m_pPlayerComponent->UseStimPack();
	m_UIButtons[Stimpack].SetCount( m_pPlayerComponent->GetNumStimPacks() );

	if ( m_pSlomoSound != nullptr ) {
		GetSoundManager().PlayWave( m_pSlomoSound, 0.5f );
	}
}

/**
 *	EtherGame::UpdateStimPack
 */
void EtherGame::UpdateStimPack( const float deltaTimeSec ) {

	if ( g_GlobalTimer.TimeElapsedSeconds() > m_SlomoStartTime + g_SlomoLength ) {
		m_SlomoStartTime = -1.0f;
		SetDeltaTimeScale(1.0f);
		GetSoundManager().SetFrequencyRatio(1.0f);
	}
}

/**
 *	EtherGame::ActivateAirstrike
 */
void EtherGame::ActivateAirstrike() {

	if ( m_AirstrikeTimeLeft > 0.0f || m_pPlayerComponent->GetNumAirstrikes() == 0 ) {
		return;
	}
	
	m_AirstrikeTimeLeft = g_AirstrikeDurationSec;

	m_pPlayerComponent->UseAirstrike();
	m_UIButtons[Airstrike].SetCount( m_pPlayerComponent->GetNumAirstrikes() );

	m_BombersLeft = 3;
	m_NextBomberSpawnTime = 0.0f;
	m_BombTimer[0] = m_BombTimer[1] = m_BombTimer[2] = g_TimeBetweenBombs;

	for ( int i = 0; i < 3; i++ ) {
		for ( int iComp = 0; iComp < m_ELBomberEntity[i]->NumComponents(); iComp++ ) {
			m_ELBomberEntity[i]->GetComponent(iComp)->Enable( true );
		}
	}
}

/**
 *	EtherGame::UpdateAirstrike
 */
void EtherGame::UpdateAirstrike( const float DeltaTimeSec ) {

	if ( m_AirstrikeTimeLeft < -10.0f ) {
		return;
	}

	m_AirstrikeTimeLeft -= DeltaTimeSec;
	if ( m_AirstrikeTimeLeft < 0 ) {
		for ( int i = 0; i < 3; i++ ) {
			for ( int iComp = 0; iComp < m_ELBomberEntity[i]->NumComponents(); iComp++ ) {
				m_ELBomberEntity[i]->SetPosition( kbVec3::zero );
			}
		}
		return;
	}

	// Spawn new bombers
	if ( m_BombersLeft > 0 ) {
		m_NextBomberSpawnTime -= DeltaTimeSec;
		if ( m_NextBomberSpawnTime <= 0 ) {
			m_NextBomberSpawnTime = g_TimeBetweenBombers;
			m_BombersLeft--;

			float xOffset;
			switch( m_BombersLeft ) {
				case 2 : xOffset = 0.0f; break;
				case 1 : xOffset = 600.0f; break;
				case 0 : xOffset = -700.0f; break;
			}

			m_ELBomberEntity[m_BombersLeft]->SetPosition( m_pLocalPlayer->GetPosition() + kbVec3( 0.0f, 1000.0f, -300.0f ) + kbVec3( xOffset, 0.0f, 0.0f ) );

			m_SoundManager.PlayWave( m_pAirstrikeFlybyWave, 1.0f );
		}
	}

	// Update bombers
	for ( int i = 2; i >= m_BombersLeft; i-- ) {

		static float moveRate = 3000.f;
		const kbVec3 moveDelta = kbVec3( 0.0f, 0.0f, 1.0f ) * DeltaTimeSec * moveRate;
		m_ELBomberEntity[i]->SetPosition( m_ELBomberEntity[i]->GetPosition() + moveDelta );
		if ( m_ELBomberEntity[i]->GetPosition().z < m_pLocalPlayer->GetPosition().z || m_ELBomberEntity[i]->GetPosition().z > m_pLocalPlayer->GetPosition().z + 4096.0f ) {
			continue;
		}

		m_BombTimer[i] -= DeltaTimeSec;
		if ( m_BombTimer[i] > 0 ) {
			continue;
		}
		m_BombTimer[i] = g_TimeBetweenBombs;
		
		const kbPrefab *const pBombPrefab = m_pWeaponsPackage->GetPrefab( "Airstrike_Bomb" );
		if ( pBombPrefab != nullptr && pBombPrefab->GetGameEntity(0) != nullptr ) {
			const kbGameEntity *const pBombPrefabEnt = pBombPrefab->GetGameEntity(0);
			kbGameEntity *const newProjectile = g_pGame->CreateEntity( pBombPrefabEnt );

			newProjectile->SetPosition( m_ELBomberEntity[i]->GetPosition() );

			static float xOffset = 2.0f;
			const float halfXOffset = xOffset * 0.5f;

			const kbVec3 zVec = kbVec3( ( kbfrand() * xOffset ) - halfXOffset, -1.0f, 0.0f ).Normalized();
			const kbVec3 yVec = ( kbVec3( 1.0f, 0.0f, 0.0f).Cross( zVec ) ).Normalized();
			const kbVec3 xVec = yVec.Cross( zVec ).Normalized();
			const kbMat4 downMat( kbVec4( xVec.x, xVec.y, xVec.z, 0.0f ),   kbVec4( yVec.x, yVec.y, yVec.z, 0.0f ), kbVec4( zVec.x, zVec.y, zVec.z, 0.0f ), kbVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			newProjectile->SetOrientation( kbQuatFromMatrix( downMat)  );

			// TODO: is it necessary to enable the projectile?
			EtherProjectileComponent *const pProj = (EtherProjectileComponent *)newProjectile->GetComponentByType( EtherProjectileComponent::GetType() );
			if ( pProj != nullptr ) {
				pProj->Enable( true );
			}
		}
	}
}

/**
 *	EtherGame::ActivateOLC
 */
const float g_OLCLen = 13.6f;
void EtherGame::ActivateOLC() {
	if ( m_OLCTimer > 0 || m_pPlayerComponent->GetNumOLC() == 0 ) {
		return;
	}

	m_OLCTimer = g_OLCLen;
	m_pPlayerComponent->UseOLC();
	m_UIButtons[OLC].SetCount( m_pPlayerComponent->GetNumOLC() );
}

/**
 *	EtherGame::UpdateOLC
 */
void EtherGame::UpdateOLC( const float DeltaTimeSec ) {

if ( m_pWorldGenComponent == nullptr ) {
		return;
	}
	if ( m_OLCTimer < 0 ) {
		m_OLCPostProcess = -1.0f;
	 	m_pWorldGenComponent->SetTerrainWarp( kbVec3::zero, 0, 0, 0 );
		m_OLCTint = kbVec3::zero;

		return;
	}

	m_OLCTimer -= DeltaTimeSec;
	const float fxSecElapsed = g_OLCLen - m_OLCTimer;

	kbParticleManager::CustomParticleInfo_t ParticleInfo;
	ParticleInfo.m_Type = BT_FaceCamera;
	ParticleInfo.m_Position = m_pLocalPlayer->GetPosition() + kbVec3( 0.0f, 0.0f, 900.0f );
	ParticleInfo.m_Direction = kbVec3( 0.0f, 1.0f, 0.0f );
	ParticleInfo.m_Width = 1024;
	ParticleInfo.m_Height = 4096;

	const float randV = 0.01f + kbfrand() * 0.23f;
	ParticleInfo.m_UVs[0].Set( 0.625f, randV );
	ParticleInfo.m_UVs[1].Set( 0.875, randV );

	kbVec3 finalColor( 1.0f, 1.0f, 1.0f );

	// Wind up
	const float STAGE_1_LEN = 5.0f;

	// Accelerate
	const float STAGE_2_START = STAGE_1_LEN;
	const float STAGE_2_LEN = 0.1f;
	const float STAGE_2_END = STAGE_1_LEN + STAGE_2_LEN;

	// Flash
	const float STAGE_3_START = STAGE_2_END;
	const float STAGE_3_LEN = 0.5f;
	const float STAGE_3_END = STAGE_2_END + STAGE_3_LEN;

	// Big Blast
	const float STAGE_4_START = STAGE_3_END;
	const float STAGE_4_LEN = 3.0f;
	const float STAGE_4_END = STAGE_3_END + STAGE_4_LEN;

	// Cool down
	const float STAGE_5_START = STAGE_4_END;
	const float STAGE_5_LEN = 5.0f;
	const float STAGE_5_END = STAGE_4_END + STAGE_5_LEN;

	if ( fxSecElapsed < STAGE_1_LEN ) {

		// Wind up
		const float MaxWidth = 4096.0f;
		const float MinWidth = 512.0f; 
		const float intensity = fxSecElapsed / STAGE_1_LEN;
		finalColor.Set( intensity, intensity, intensity );
		ParticleInfo.m_Width = MinWidth + ( MaxWidth - MinWidth ) * ( 1.0f - intensity );;

		const float playSoundTime = STAGE_1_LEN * 0.05f;
		if ( fxSecElapsed - DeltaTimeSec < playSoundTime && fxSecElapsed >= playSoundTime ) {
			m_SoundManager.PlayWave( m_pOLCWindupWave, 1.0f );
		}

	} else if ( fxSecElapsed < STAGE_2_END ) {

		// Accelerate
		const float MaxWidth = 512.0f;
		const float MinWidth = 0.0f;

		const float intensity = ( fxSecElapsed - STAGE_1_LEN ) / STAGE_2_LEN;
		float width = ( MaxWidth - MinWidth ) * ( 1.0f - intensity );
		ParticleInfo.m_Width = width;
	} else if ( fxSecElapsed < STAGE_3_END ) {

		// Flash
		ParticleInfo.m_Width = 800.0f;
		const float midPt = (  STAGE_2_END + STAGE_3_END ) * 0.5f;
		if ( fxSecElapsed < midPt ) {
			m_OLCPostProcess = ( fxSecElapsed - STAGE_2_END ) / ( STAGE_3_LEN * 0.5f );
		} else {
			m_OLCPostProcess = 1.0f - ( fxSecElapsed - midPt ) / ( STAGE_3_LEN * 0.5f );
		}

		if ( fxSecElapsed - DeltaTimeSec <= midPt && fxSecElapsed > midPt ) {
			m_SoundManager.PlayWave( m_pOLCExplosion, 1.0f );
		}

	} else {

		ParticleInfo.m_Width = 1024.0f + kbfrand() * 128.0f;
		m_OLCPostProcess = -1.0f;

		if ( fxSecElapsed < STAGE_4_END ) {
			for ( int i = 0; i < g_pEtherGame->GetGameEntities().size(); i++ ) {
				kbGameEntity *const pCurEnt = g_pEtherGame->GetGameEntities()[i];
				EtherAIComponent *const pAI = (EtherAIComponent*) pCurEnt->GetComponentByType( EtherAIComponent::GetType() );
				if ( pAI == nullptr ) {
					continue;
				}

				RemoveGameEntity( pCurEnt );
			}
		}
	}

	EtherWorldGenCollision_t hitInfo;
	if ( m_pWorldGenComponent->TraceAgainstWorld( hitInfo, ParticleInfo.m_Position + kbVec3( 0.0f, 10000.0f, 0.0f ), ParticleInfo.m_Position - kbVec3( 0.0f, 10000.0f, 0.0f ), false ) ) {

		float warpIntensity = 0.0f;
		float timeScale = 2.0f;

		kbVec3 redTint( 1.0f, 10.0f / 256.0f, 20.0f/ 256.0f );
		redTint *= 2.0f;
		if ( fxSecElapsed < STAGE_3_END ) {
			// Increase up to the flash
			warpIntensity = 2 * ( fxSecElapsed / STAGE_3_END );
			timeScale *= 3;
		} else if ( fxSecElapsed < STAGE_4_END ) {
			// Damage
			warpIntensity = 2.0f;
			timeScale = 8.0f * warpIntensity;
			m_OLCTint = redTint * warpIntensity * 0.05f;
		} else {
			warpIntensity = 2.0f * ( 1.0f - ( fxSecElapsed - STAGE_5_START ) / STAGE_5_LEN );
			timeScale = 16.0f;// * warpIntensity;
			finalColor.Set( warpIntensity, warpIntensity, warpIntensity );
			finalColor *= 0.5f;
			const float MaxWidth = 4096.0f;
			const float MinWidth = 1024.0f;
			ParticleInfo.m_Width  = ( MaxWidth - MinWidth ) * ( 2.0f - warpIntensity ) + MinWidth;

			float tintEndTime = 1.0f;
			float tintAdjust = 1.0f - kbClamp( ( fxSecElapsed - STAGE_5_START ) / tintEndTime, 0.0f, 1.0f );
			m_OLCTint = redTint * warpIntensity * tintAdjust * 0.05f;
		}

warpIntensity = kbClamp( warpIntensity, 0.0f, 10.0f );

//	kbLog( "Particle Width = %f", ParticleInfo.m_Width );
		//warpIntensity = kbClamp( warpIntensity, 0.0f, 1.0f );
		const float warpAmp = 16.0f * warpIntensity;
		const float radius = 1024.0f * warpIntensity;

	 	m_pWorldGenComponent->SetTerrainWarp( hitInfo.m_HitLocation, warpAmp, radius, timeScale );
	}

	ParticleInfo.m_Color = finalColor;
	this->m_pParticleManager->AddQuad( ParticleInfo );
}
