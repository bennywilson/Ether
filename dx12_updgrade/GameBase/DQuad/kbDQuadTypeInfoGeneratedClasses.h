// AUTO GENERATED TYPE INFO //////////////////////////////////////////

GenerateClass( 
	kbDQuadWorldGenComponent, 
	AddField( "TerrainDimensions", KBTYPEINFO_INT, kbDQuadWorldGenComponent, m_TerrainDimensions, false, "" )
	AddField( "ChunkDimensions", KBTYPEINFO_INT, kbDQuadWorldGenComponent, m_ChunkDimensions, false, "" )
	AddField( "ChunkWorldLength", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_ChunkWorldLength, false, "" )
	AddField( "NoiseScale", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_TerrainGenerationNoiseScale, false, "" )
	AddField( "MaxTerrainHeight", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_MaxTerrainHeight, false, "" )
	AddField( "CellMidPtHeight", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_MaxTerrainCellMidPointHeight, false, "" )
	AddField( "EnviroInfo", KBTYPEINFO_STRUCT, kbDQuadWorldGenComponent, m_EnviroInfo, true, "kbDQuadEnviroInfo" )
	AddField( "SecondsInADay", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_SecondsInADay, false, "" )
	AddField( "SunEntity", KBTYPEINFO_GAMEENTITY, kbDQuadWorldGenComponent, m_SunEntity, false, "" )
	AddField( "SkyEntity", KBTYPEINFO_GAMEENTITY, kbDQuadWorldGenComponent, m_SkyEntity, false, "" )
	AddField( "FogEntity", KBTYPEINFO_GAMEENTITY, kbDQuadWorldGenComponent, m_FogEntity, false, "" )
	AddField( "LightShaftEntity", KBTYPEINFO_GAMEENTITY, kbDQuadWorldGenComponent, m_LightShaftEntity, false, "" )
	AddField( "DebugHour", KBTYPEINFO_FLOAT, kbDQuadWorldGenComponent, m_DebugHour, false, "" )
)

GenerateClass(
	kbDQuadAnimComponent,
	AddField( "AnimationName", KBTYPEINFO_KBSTRING, kbDQuadAnimComponent, m_AnimationName, false, "" )
	AddField( "Animation", KBTYPEINFO_ANIMATION, kbDQuadAnimComponent, m_pAnimation, false, "" )
	AddField( "TimeScale", KBTYPEINFO_FLOAT, kbDQuadAnimComponent, m_TimeScale, false, "" )
	AddField( "IsLooping", KBTYPEINFO_BOOL, kbDQuadAnimComponent, m_bIsLooping, false, "" )
)

GenerateClass(
	kbDQuadSkelModelComponent,
	AddField( "Animations", KBTYPEINFO_STRUCT, kbDQuadSkelModelComponent, m_Animations, true, "kbDQuadAnimComponent" )
	AddField( "DebugAnimIndex", KBTYPEINFO_INT, kbDQuadSkelModelComponent, m_DebugAnimIdx, false, "" )
	AddField( "IsFirstPersonModel", KBTYPEINFO_BOOL, kbDQuadSkelModelComponent, m_bFirstPersonModel, false, "" )
)

GenerateClass(
	kbDQuadActorComponent,
	AddField( "m_Dummy", KBTYPEINFO_INT, kbDQuadActorComponent, m_Dummy, false, "" )
)

GenerateClass(
	kbDQuadAIComponent,
	AddField( "UpdateHz", KBTYPEINFO_INT, kbDQuadAIComponent, m_UpdateFrequency, false, "" )
)

GenerateClass(
	kbEnemyAIComponent,
	AddField( "m_TempDummy1", KBTYPEINFO_INT, kbEnemyAIComponent, m_TempDummy1, false, "" )
)

GenerateClass(
	kbEnemySoldierAIComponent,
	AddField( "m_TempDummy2", KBTYPEINFO_INT, kbEnemySoldierAIComponent, m_TempDummy2, false, "" )
	AddField( "EyeModel", KBTYPEINFO_STATICMODEL, kbEnemySoldierAIComponent, m_pEyeBall, false, "" )
)

GenerateClass(
	kbDQuadPlayerComponent,
	AddField( "m_PlayerDummy", KBTYPEINFO_INT, kbDQuadPlayerComponent, m_PlayerDummy, false, "" )
)

GenerateClass(
	kbDQuadProjectileComponent,
	AddField( "DamageAmount", KBTYPEINFO_FLOAT, kbDQuadProjectileComponent, m_Damage, false, "" )
	AddField( "Velocity", KBTYPEINFO_FLOAT, kbDQuadProjectileComponent, m_Velocity, false, "" )
	AddField( "LifeTime", KBTYPEINFO_FLOAT, kbDQuadProjectileComponent, m_LifeTime, false, "" )
	AddField( "ExplosionFX", KBTYPEINFO_GAMEENTITY, kbDQuadProjectileComponent, m_ExplosionFX, false, "" )
	AddField( "TracerLength", KBTYPEINFO_FLOAT, kbDQuadProjectileComponent, m_TracerLength, false, "" )
	AddField( "TraceWidth", KBTYPEINFO_FLOAT, kbDQuadProjectileComponent, m_TraceWidth, false, "" )
)

GenerateClass(
	kbDQuadWeaponComponent,
	AddField( "ShotsPerSecond", KBTYPEINFO_FLOAT, kbDQuadWeaponComponent, m_ShotsPerSecond, false, "" )
	AddField( "MuzzleFlash", KBTYPEINFO_GAMEENTITY, kbDQuadWeaponComponent, m_MuzzleFlashEntity, false, "" )
	AddField( "Projectile", KBTYPEINFO_GAMEENTITY, kbDQuadWeaponComponent, m_Projectile, false, "" )
	AddField( "IsInstantHit", KBTYPEINFO_BOOL, kbDQuadWeaponComponent, m_bInstantHit, false, "" )
)

GenerateClass(
	kbDQuadEnviroMaterial,
	AddField( "Color", KBTYPEINFO_VECTOR4, kbDQuadEnviroMaterial, m_Color, false, "" )
	AddField( "Texture", KBTYPEINFO_TEXTURE, kbDQuadEnviroMaterial, m_Texture, false, "" )
)

GenerateClass(
	kbDQuadEnviroObject,
	AddField( "Model", KBTYPEINFO_STATICMODEL, kbDQuadEnviroObject, m_pModel, false, "" )
	AddField( "MinScale", KBTYPEINFO_VECTOR, kbDQuadEnviroObject, m_MinScale, false, "" )
	AddField( "MaxScale", KBTYPEINFO_VECTOR, kbDQuadEnviroObject, m_MaxScale, false, "" )
	AddField( "MinHealth", KBTYPEINFO_FLOAT, kbDQuadEnviroObject, m_MinHealth, false, "" )
	AddField( "MaxHealth", KBTYPEINFO_FLOAT, kbDQuadEnviroObject, m_MaxHealth, false, "" )
)

GenerateClass(
	kbDQuadEnviroComponent,
	AddField( "EnviroMaterial", KBTYPEINFO_STRUCT, kbDQuadEnviroComponent, m_EnviroMaterials, true, "kbDQuadEnviroMaterial" )
	AddField( "CoverObjects", KBTYPEINFO_STRUCT, kbDQuadEnviroComponent, m_CoverObjects, true, "kbDQuadEnviroObject" )
	AddField( "EnviroObjects", KBTYPEINFO_STRUCT, kbDQuadEnviroComponent, m_EnviroObjects, true, "kbDQuadEnviroObject" )
	AddField( "TimeOfDayModifiers", KBTYPEINFO_STRUCT, kbDQuadEnviroComponent, m_TimeOfDayModifiers, true, "kbDQuadTimeOfDayModifier" )
)

GenerateClass(
	kbDQuadEnviroInfo,
	AddField( "EnviroData", KBTYPEINFO_GAMEENTITY, kbDQuadEnviroInfo, m_EnvironmentData, false, "" )
)

GenerateClass(
	kbDQuadTimeOfDayModifier,
	AddField( "Hour", KBTYPEINFO_FLOAT, kbDQuadTimeOfDayModifier, m_Hour, false, "" )
	AddField( "SunColor", KBTYPEINFO_VECTOR4, kbDQuadTimeOfDayModifier, m_SunColor, false, "" )
	AddField( "FogColor", KBTYPEINFO_VECTOR4, kbDQuadTimeOfDayModifier, m_FogColor, false, "" )
	AddField( "SkyColor", KBTYPEINFO_VECTOR4, kbDQuadTimeOfDayModifier, m_SkyColor, false, "" )
	AddField( "LightShaftColor", KBTYPEINFO_VECTOR4, kbDQuadTimeOfDayModifier, m_LightShaftColor, false, "" )
)
