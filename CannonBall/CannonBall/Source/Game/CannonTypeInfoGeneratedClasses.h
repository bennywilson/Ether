// AUTO GENERATED TYPE INFO //////////////////////////////////////////

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
)

GenerateClass(
	KungFuSheepComponent,
	AddField("CannonBallImpactFX", KBTYPEINFO_GAMEENTITY, KungFuSheepComponent, m_CannonBallImpactFX, false, "")
	AddField("CannonBallVO", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_CannonBallVO, true, "kbSoundData")
	AddField("CannonBallImpactSound", KBTYPEINFO_STRUCT, KungFuSheepComponent, m_CannonBallImpactSound, true, "kbSoundData")
	AddField("JumpSmearMagnitude", KBTYPEINFO_FLOAT, KungFuSheepComponent, m_JumpSmearMagnitude, false, "")
	AddField("DropSmearMagnitude", KBTYPEINFO_FLOAT, KungFuSheepComponent, m_DropSmearMagnitude, false, "")
)

GenerateClass(
	KungFuSnolafComponent,
	AddField("Dummy", KBTYPEINFO_FLOAT, KungFuSnolafComponent, m_Dummy, false, "")
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
)

GenerateClass(
	CannonLevelComponent,
	AddField( "Dummy2", KBTYPEINFO_FLOAT, CannonLevelComponent, m_Dummy2, false, "" )
)

GenerateClass(
	KungFuLevelComponent,
	AddField( "Dummy", KBTYPEINFO_FLOAT, KungFuLevelComponent, m_Dummy, false, "" )
)
