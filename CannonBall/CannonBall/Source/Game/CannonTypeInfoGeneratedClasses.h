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
	CannonPlayerComponent,
	AddField( "MaxRunSpeed", KBTYPEINFO_FLOAT, CannonPlayerComponent, m_MaxRunSpeed, false, "" )
	AddField( "MaxRotateSpeed", KBTYPEINFO_FLOAT, CannonPlayerComponent, m_MaxRotateSpeed, false, "" )
)

GenerateClass(
	CannonCameraComponent,
	AddField( "NearPlane", KBTYPEINFO_FLOAT, CannonCameraComponent, m_NearPlane, false, "" )
	AddField( "FarPlane", KBTYPEINFO_FLOAT, CannonCameraComponent, m_FarPlane, false, "" )
	AddField( "MovementMode", KBTYPEINFO_ENUM, CannonCameraComponent, m_MoveMode, false, "ECameraMoveMode" )
	AddField( "TargetOffset", KBTYPEINFO_VECTOR, CannonCameraComponent, m_Offset, false, "" )
)
