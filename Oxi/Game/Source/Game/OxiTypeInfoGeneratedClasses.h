// AUTO GENERATED TYPE INFO //////////////////////////////////////////

GenerateEnum( 
	ECameraMoveMode, "ECameraMoveMode",
	AddEnumField( MoveMode_None, "None" )
	AddEnumField( MoveMode_Follow, "Follow" )
)

GenerateClass(
	OxiCameraComponent,
	AddField( "NearPlane", KBTYPEINFO_FLOAT, OxiCameraComponent, m_NearPlane, false, "" )
	AddField( "FarPlane", KBTYPEINFO_FLOAT, OxiCameraComponent, m_FarPlane, false, "" )
	AddField( "MovementMode", KBTYPEINFO_ENUM, OxiCameraComponent, m_MoveMode, false, "ECameraMoveMode" )
	AddField( "PositionOffset", KBTYPEINFO_VECTOR, OxiCameraComponent, m_PositionOffset, false, "" )
	AddField( "LookAtOffset", KBTYPEINFO_VECTOR, OxiCameraComponent, m_LookAtOffset, false, "" )
)

GenerateClass(
	OxiCameraShakeComponent,
	AddField( "Duration", KBTYPEINFO_FLOAT, OxiCameraShakeComponent, m_Duration, false, "" )
	AddField( "AmplitudeX", KBTYPEINFO_FLOAT, OxiCameraShakeComponent, m_AmplitudeX, false, "" )
	AddField( "FrequencyX", KBTYPEINFO_FLOAT, OxiCameraShakeComponent, m_FrequencyX, false, "" )
	AddField( "AmplitudeY", KBTYPEINFO_FLOAT, OxiCameraShakeComponent, m_AmplitudeY, false, "" )
	AddField( "FrequencyY", KBTYPEINFO_FLOAT, OxiCameraShakeComponent, m_FrequencyY, false, "" )
	AddField( "ActivateOnEnable", KBTYPEINFO_BOOL, OxiCameraShakeComponent, m_bActivateOnEnable, false, "" )
)


/*
GenerateEnum( 
	ELevelType, "ELevelType",
	AddEnumField( LevelType_Menu, "Menu" )
	AddEnumField( LevelType_2D, "2D" )
)

GenerateEnum( 
	ECameraMoveMode, "ECameraMoveMode",
	AddEnumField( MoveMode_None, "None" )
	AddEnumField( MoveMode_Follow, "Follow" )
)

GenerateClass(
	CannonActorComponent,
	AddField( "MaxRunSpeed", KBTYPEINFO_FLOAT, CannonActorComponent, m_MaxRunSpeed, false, "" )
	AddField( "MaxRotateSpeed", KBTYPEINFO_FLOAT, CannonActorComponent, m_MaxRotateSpeed, false, "" )
	AddField( "Health", KBTYPEINFO_FLOAT, CannonActorComponent, m_Health, false, "" )
	AddField( "AttackVO", KBTYPEINFO_STRUCT, CannonActorComponent, m_AttackVO, true, "kbSoundData" )
)

GenerateClass(
	KungFuSheepComponent,
	AddField( "CannonBallImpactFX", KBTYPEINFO_GAMEENTITY, KungFuSheepComponent, m_CannonBallImpactFX, false, "" )
	AddField( "ShakeNBakeFX", KBTYPEINFO_GAMEENTITY, KungFuSheepComponent, m_ShakeNBakeFX, false, "" )
	AddField( "SplashFX", KBTYPEINFO_GAMEENTITY, KungFuSheepComponent, m_SplashFX, false, "" )
	AddField( "CannonBallVO", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_CannonBallVO, true, "kbSoundData" )
	AddField( "BaaVO", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_BaaaVO, true, "kbSoundData" )
	AddField( "CannonBallImpactSound", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_CannonBallImpactSound, true, "kbSoundData" )
	AddField( "AttackImpactSound", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_BasicAttackImpactSound, true, "kbSoundData" )
	AddField( "JumpSmearMagnitude", KBTYPEINFO_FLOAT, KungFuSheepComponent, m_JumpSmearMagnitude, false, "" )
	AddField( "DropSmearMagnitude", KBTYPEINFO_FLOAT, KungFuSheepComponent, m_DropSmearMagnitude, false, "" )
	AddField( "HeadBand", KBTYPEINFO_GAMEENTITY, KungFuSheepComponent, m_HeadBand, false, "" )
)

GenerateClass(
	KungFuSnolafComponent,
	AddField( "StepImpactFX", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_FootStepImpactFX, false, "" )
	AddField( "PoofDeathFX", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_PoofDeathFX, false, "" )
	AddField( "DecapHead", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_DecapitatedHead, false, "" )
	AddField( "TopHalf", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_TopHalfOfBody, false, "" )
	AddField( "BottomHalf", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_BottomHalfOfBody, false, "" )
	AddField( "SplashFX", KBTYPEINFO_GAMEENTITY, KungFuSnolafComponent, m_SplashFX, false, "" )
)

GenerateClass(
	CannonCameraComponent,
	AddField( "NearPlane", KBTYPEINFO_FLOAT, CannonCameraComponent, m_NearPlane, false, "" )
	AddField( "FarPlane", KBTYPEINFO_FLOAT, CannonCameraComponent, m_FarPlane, false, "" )
	AddField( "MovementMode", KBTYPEINFO_ENUM, CannonCameraComponent, m_MoveMode, false, "ECameraMoveMode" )
	AddField( "PositionOffset", KBTYPEINFO_VECTOR, CannonCameraComponent, m_PositionOffset, false, "" )
	AddField( "LookAtOffset", KBTYPEINFO_VECTOR, CannonCameraComponent, m_LookAtOffset, false, "" )
)

GenerateClass(
	CannonCameraShakeComponent,
	AddField( "Duration", KBTYPEINFO_FLOAT, CannonCameraShakeComponent, m_Duration, false, "" )
	AddField( "AmplitudeX", KBTYPEINFO_FLOAT, CannonCameraShakeComponent, m_AmplitudeX, false, "" )
	AddField( "FrequencyX", KBTYPEINFO_FLOAT, CannonCameraShakeComponent, m_FrequencyX, false, "" )
	AddField( "AmplitudeY", KBTYPEINFO_FLOAT, CannonCameraShakeComponent, m_AmplitudeY, false, "" )
	AddField( "FrequencyY", KBTYPEINFO_FLOAT, CannonCameraShakeComponent, m_FrequencyY, false, "" )
	AddField( "ActivateOnEnable", KBTYPEINFO_BOOL, CannonCameraShakeComponent, m_bActivateOnEnable, false, "" )
)

GenerateClass(
	CannonLevelComponent,
	AddField( "Dummy2", KBTYPEINFO_FLOAT, CannonLevelComponent, m_Dummy2, false, "" )
)

GenerateClass(
	KungFuLevelComponent,
	AddField( "LevelLength", KBTYPEINFO_FLOAT, KungFuLevelComponent, m_LevelLength, false, "" )
	AddField( "LevelMusic", KBTYPEINFO_STRUCT, KungFuLevelComponent, m_LevelMusic, true, "kbSoundData" )
	AddField( "SnolafPrefab", KBTYPEINFO_GAMEENTITY, KungFuLevelComponent, m_SnolafPrefab, false, "" )
	AddField( "SheepPrefab", KBTYPEINFO_GAMEENTITY, KungFuLevelComponent, m_SheepPrefab, false, "" )
	AddField( "WaterDropletScreenFX", KBTYPEINFO_GAMEENTITY, KungFuLevelComponent, m_WaterDropletScreenFX, false, "" )
	AddField( "WaterSplashSound", KBTYPEINFO_STRUCT, KungFuLevelComponent, m_WaterSplashSound, true, "kbSoundData" )
)

GenerateClass(
	CannonFogComponent,
	AddField( "Shader", KBTYPEINFO_SHADER, CannonFogComponent, m_pShader, false, "" )
	AddField( "StartDist", KBTYPEINFO_FLOAT, CannonFogComponent, m_FogStartDist, false, "" )
	AddField( "EndDist", KBTYPEINFO_FLOAT, CannonFogComponent, m_FogEndDist, false, "" )
	AddField( "Clamp", KBTYPEINFO_FLOAT, CannonFogComponent, m_FogClamp, false, "" )
	AddField( "Color", KBTYPEINFO_VECTOR4, CannonFogComponent, m_FogColor, false, "" )	
)

GenerateClass(
	CannonHealthBarUIComponent,
	AddField( "HealthBarWarningFlashThreshold", KBTYPEINFO_FLOAT, CannonHealthBarUIComponent, m_HealthBarWarningFlashThreshold, false, "" )
	AddField( "HealthBarWarningFlashSpeed", KBTYPEINFO_FLOAT, CannonHealthBarUIComponent, m_HealthBarWarningFlashSpeed, false, "" )
)

GenerateClass(
	CannonBallUIComponent,
	AddField( "SparkRelativePosition", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_SparkRelativePosition, false, "" )
	AddField( "SparkRelativeSize", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_SparkRelativeSize, false, "" )
	AddField( "BoomRelativePosition", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_BoomRelativePosition, false, "" )
	AddField( "BoomRelativeSize", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_BoomRelativeSize, false, "" )
	AddField( "SmokeRelativePosition", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_SmokeRelativePosition, false, "" )
	AddField( "SmokeRelativeSize", KBTYPEINFO_VECTOR, CannonBallUIComponent, m_SmokeRelativeSize, false, "" )
)

GenerateClass(
	CannonBallPauseMenuUIComponent,
	AddField( "SliderWidgets", KBTYPEINFO_STRUCT, CannonBallPauseMenuUIComponent, m_SliderWidgets, true, "kbUISlider" )
	AddField( "Widgets", KBTYPEINFO_STRUCT, CannonBallPauseMenuUIComponent, m_Widgets, true, "kbUIWidgetComponent" )
	AddField( "WidgetSize", KBTYPEINFO_VECTOR, CannonBallPauseMenuUIComponent, m_WidgetSize, false, "" )
	AddField( "StartingWidgetAnchor", KBTYPEINFO_VECTOR, CannonBallPauseMenuUIComponent, m_StartingWidgetAnchorPt, false, "" )
	AddField( "SpaceBetweenWidgets", KBTYPEINFO_FLOAT, CannonBallPauseMenuUIComponent, m_SpaceBetweenWidgets, false, "" )
	AddField( "TestSound", KBTYPEINFO_STRUCT, CannonBallPauseMenuUIComponent, m_VolumeSliderTestWav, true, "kbSoundData" )
)

GenerateClass(
	CannonBallMainMenuComponent,
	AddField( "ActionVO", KBTYPEINFO_STRUCT, CannonBallMainMenuComponent, m_ActionVO, true, "kbSoundData" )
)

GenerateClass(
	CannonBallYesNoPromptComponent,
)

GenerateClass(
	CannonBallGameSettingsComponent,
	AddField( "MasterVolume", KBTYPEINFO_INT, CannonBallGameSettingsComponent, m_Volume, false, "" )
	AddField( "Brightness", KBTYPEINFO_INT, CannonBallGameSettingsComponent, m_Brightness, false, "" )
	AddField( "VideoQuality", KBTYPEINFO_INT, CannonBallGameSettingsComponent, m_VisualQuality, false, "" )
)
*/