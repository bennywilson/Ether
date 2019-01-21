// AUTO GENERATED TYPE INFO //////////////////////////////////////////

GenerateClass( 
	EtherWorldGenComponent, 
	AddField( "TerrainDimensions", KBTYPEINFO_INT, EtherWorldGenComponent, m_ChunksPerTerrainSide, false, "" )
	AddField( "ChunkDimensions", KBTYPEINFO_INT, EtherWorldGenComponent, m_TrisPerChunkSide, false, "" )
	AddField( "ChunkWorldLength", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_ChunkWorldLength, false, "" )
	AddField( "NoiseScale", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_TerrainGenNoiseScale, false, "" )
	AddField( "MaxTerrainHeight", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_MaxTerrainHeight, false, "" )
	AddField( "CellMidPtHeight", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_MaxTerrainCellMidPointHeight, false, "" )
	AddField( "EnviroInfo", KBTYPEINFO_STRUCT, EtherWorldGenComponent, m_EnviroInfo, true, "EtherEnviroInfo" )
	AddField( "SecondsInADay", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_SecondsInADay, false, "" )
	AddField( "SunEntity", KBTYPEINFO_GAMEENTITY, EtherWorldGenComponent, m_SunEntity, false, "" )
	AddField( "SkyEntity", KBTYPEINFO_GAMEENTITY, EtherWorldGenComponent, m_SkyEntity, false, "" )
	AddField( "FogEntity", KBTYPEINFO_GAMEENTITY, EtherWorldGenComponent, m_FogEntity, false, "" )
	AddField( "LightShaftEntity", KBTYPEINFO_GAMEENTITY, EtherWorldGenComponent, m_LightShaftEntity, false, "" )
	AddField( "DebugHour", KBTYPEINFO_FLOAT, EtherWorldGenComponent, m_DebugHour, false, "" )
)

GenerateClass(
	EtherAnimEvent,
	AddField( "EventName", KBTYPEINFO_KBSTRING, EtherAnimEvent, m_EventName, false, "" )
	AddField( "EventTime", KBTYPEINFO_FLOAT, EtherAnimEvent, m_EventTime, false, "" )
)

GenerateClass(
	EtherAnimComponent,
	AddField( "AnimationName", KBTYPEINFO_KBSTRING, EtherAnimComponent, m_AnimationName, false, "" )
	AddField( "Animation", KBTYPEINFO_ANIMATION, EtherAnimComponent, m_pAnimation, false, "" )
	AddField( "TimeScale", KBTYPEINFO_FLOAT, EtherAnimComponent, m_TimeScale, false, "" )
	AddField( "IsLooping", KBTYPEINFO_BOOL, EtherAnimComponent, m_bIsLooping, false, "" )
	AddField( "AnimationEvent", KBTYPEINFO_STRUCT, EtherAnimComponent, m_AnimEvents, true, "EtherAnimEvent" )
)

GenerateClass(
	EtherSkelModelComponent,
	AddField( "Animations", KBTYPEINFO_STRUCT, EtherSkelModelComponent, m_Animations, true, "EtherAnimComponent" )
	AddField( "DebugAnimIndex", KBTYPEINFO_INT, EtherSkelModelComponent, m_DebugAnimIdx, false, "" )
	AddField( "IsFirstPersonModel", KBTYPEINFO_BOOL, EtherSkelModelComponent, m_bFirstPersonModel, false, "" )
)

GenerateClass(
	EtherActorComponent,
	AddField( "m_Dummy", KBTYPEINFO_INT, EtherActorComponent, m_Dummy, false, "" )
)

GenerateClass(
	EtherAIComponent,
	AddField( "UpdateHz", KBTYPEINFO_INT, EtherAIComponent, m_UpdateFrequency, false, "" )
)

GenerateClass(
	EtherEnemySoldierAIComponent,
	AddField( "EyeModel", KBTYPEINFO_STATICMODEL, EtherEnemySoldierAIComponent, m_pEyeBall, false, "" )
	AddField( "SprayDurationSec", KBTYPEINFO_FLOAT, EtherEnemySoldierAIComponent, m_SprayDurationSec, false, "" )
	AddField( "SecBetweenSprays", KBTYPEINFO_FLOAT, EtherEnemySoldierAIComponent, m_SecTimeBetweenSprays, false, "" )
	AddField( "SecondsBetweenShots", KBTYPEINFO_FLOAT, EtherEnemySoldierAIComponent, m_SecBetweenShots, false, "" )
	AddField( "Projectile", KBTYPEINFO_GAMEENTITY, EtherEnemySoldierAIComponent, m_Projectile, false, "" )
)

GenerateClass(
	EtherPlayerComponent,
	AddField( "m_PlayerDummy", KBTYPEINFO_INT, EtherPlayerComponent, m_PlayerDummy, false, "" )
)

GenerateClass(
	EtherProjectileComponent,
	AddField( "DamageAmount", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_Damage, false, "" )
	AddField( "Velocity", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_Velocity, false, "" )
	AddField( "LifeTime", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_LifeTime, false, "" )
	AddField( "ExplosionFX", KBTYPEINFO_GAMEENTITY, EtherProjectileComponent, m_ExplosionFX, false, "" )
	AddField( "TracerLength", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_TracerLength, false, "" )
	AddField( "TraceWidth", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_TraceWidth, false, "" )
	AddField( "ImpactCharacterSound", KBTYPEINFO_STRUCT, EtherProjectileComponent, m_ImpactCharacterSoundData, true, "kbSoundData" )
	AddField( "ImpactEnvironmentSound", KBTYPEINFO_STRUCT, EtherProjectileComponent, m_ImpactEnvironmentSoundData, true, "kbSoundData" )
	AddField( "LaunchSound", KBTYPEINFO_STRUCT, EtherProjectileComponent, m_LaunchSoundData, true, "kbSoundData" )
	AddField( "RenderBillboard", KBTYPEINFO_BOOL, EtherProjectileComponent, m_bUseBillboard, false, "" )
	AddField( "DetonationTime", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_DetonationTimer, false, "" )
	AddField( "ExplodeOnImpact", KBTYPEINFO_BOOL, EtherProjectileComponent, m_bExplodeOnImpact, false, "" )
	AddField( "ExplosionSound", KBTYPEINFO_STRUCT, EtherProjectileComponent, m_ExplosionSoundData, true, "kbSoundData" )
	AddField( "DamageRadius", KBTYPEINFO_FLOAT, EtherProjectileComponent, m_DamageRadius, false, "" )
)

GenerateClass(
	EtherWeaponComponent,
	AddField( "SecondsBetweenShots", KBTYPEINFO_FLOAT, EtherWeaponComponent, m_SecondsBetweenShots, false, "" )
	AddField( "SecondsBetweenBursts", KBTYPEINFO_FLOAT, EtherWeaponComponent, m_SecondsBetweenBursts, false, "" )
	AddField( "BurstCount", KBTYPEINFO_INT, EtherWeaponComponent, m_BurstCount, false, "" )
	AddField( "MuzzleFlash", KBTYPEINFO_GAMEENTITY, EtherWeaponComponent, m_MuzzleFlashEntity, false, "" )
	AddField( "MuzzleFlashAnimations", KBTYPEINFO_STRUCT, EtherWeaponComponent, m_MuzzleFlashAnimData, true, "kbAnimatedQuadComponent" )
	AddField( "Projectile", KBTYPEINFO_GAMEENTITY, EtherWeaponComponent, m_Projectile, false, "" )
	AddField( "ShellModel", KBTYPEINFO_STATICMODEL, EtherWeaponComponent, m_pShellModel, false, "" )
	AddField( "ShellMinVelocity", KBTYPEINFO_VECTOR, EtherWeaponComponent, m_MinShellVelocity, false, "" )
	AddField( "ShellMaxVelocity", KBTYPEINFO_VECTOR, EtherWeaponComponent, m_MaxShellVelocity, false, "" )
	AddField( "ShellMinAxisVelocity", KBTYPEINFO_VECTOR, EtherWeaponComponent, m_MinAxisVelocity, false, "" )
	AddField( "ShellMaxAxisVelocity", KBTYPEINFO_VECTOR, EtherWeaponComponent, m_MaxAxisVelocity, false, "" )
	AddField( "ShellLifeTime", KBTYPEINFO_FLOAT, EtherWeaponComponent, m_ShellLifeTime, false, "" )
	AddField( "IsInstantHit", KBTYPEINFO_BOOL, EtherWeaponComponent, m_bInstantHit, false, "" )
)

GenerateClass(
	kbVec3TimePointComponent,
	AddField( "Vector", KBTYPEINFO_VECTOR, kbVec3TimePointComponent, m_Vector, false, "" )
	AddField( "Time", KBTYPEINFO_FLOAT, kbVec3TimePointComponent, m_Time, false, "" )
)

GenerateClass(
	kbAnimatedQuadComponent,
	AddField( "Texture", KBTYPEINFO_TEXTURE, kbAnimatedQuadComponent, m_pTexture, false, "" )
	AddField( "UVStart", KBTYPEINFO_VECTOR, kbAnimatedQuadComponent, m_UVStart, false, "" )
	AddField( "UVEnd", KBTYPEINFO_VECTOR, kbAnimatedQuadComponent, m_UVEnd, false, "" )
	AddField( "MinStartScale", KBTYPEINFO_VECTOR, kbAnimatedQuadComponent, m_MinStartScale, false, "" )
	AddField( "MaxStartScale", KBTYPEINFO_VECTOR, kbAnimatedQuadComponent, m_MaxStartScale, false, "" )
	AddField( "ScaleOverTime", KBTYPEINFO_STRUCT, kbAnimatedQuadComponent, m_ScaleOverTime, true, "kbVec3TimePointComponent" )
	AddField( "MinLifeTime", KBTYPEINFO_FLOAT, kbAnimatedQuadComponent, m_MinLifeTime, false, "" )
	AddField( "MaxLifeTime", KBTYPEINFO_FLOAT, kbAnimatedQuadComponent, m_MaxLifeTime, false, "" )
	AddField( "RandomRotation", KBTYPEINFO_BOOL, kbAnimatedQuadComponent, m_bRandomizeStartingRotation, false, "" )
)

GenerateClass(
	EtherEnviroMaterial,
	AddField( "Color", KBTYPEINFO_VECTOR4, EtherEnviroMaterial, m_Color, false, "" )
	AddField( "Texture", KBTYPEINFO_TEXTURE, EtherEnviroMaterial, m_Texture, false, "" )
)

GenerateClass(
	EtherEnviroObject,
	AddField( "Model", KBTYPEINFO_STATICMODEL, EtherEnviroObject, m_pModel, false, "" )
	AddField( "MinScale", KBTYPEINFO_VECTOR, EtherEnviroObject, m_MinScale, false, "" )
	AddField( "MaxScale", KBTYPEINFO_VECTOR, EtherEnviroObject, m_MaxScale, false, "" )
	AddField( "MinHealth", KBTYPEINFO_FLOAT, EtherEnviroObject, m_MinHealth, false, "" )
	AddField( "MaxHealth", KBTYPEINFO_FLOAT, EtherEnviroObject, m_MaxHealth, false, "" )
)

GenerateClass(
	EtherEnviroComponent,
	AddField( "EnviroMaterial", KBTYPEINFO_STRUCT, EtherEnviroComponent, m_EnviroMaterials, true, "EtherEnviroMaterial" )
	AddField( "CoverObjects", KBTYPEINFO_STRUCT, EtherEnviroComponent, m_CoverObjects, true, "EtherEnviroObject" )
	AddField( "EnviroObjects", KBTYPEINFO_STRUCT, EtherEnviroComponent, m_EnviroObjects, true, "EtherEnviroObject" )
	AddField( "TimeOfDayModifiers", KBTYPEINFO_STRUCT, EtherEnviroComponent, m_TimeOfDayModifiers, true, "EtherTimeOfDayModifier" )
)

GenerateClass(
	EtherEnviroInfo,
	AddField( "EnviroData", KBTYPEINFO_GAMEENTITY, EtherEnviroInfo, m_EnvironmentData, false, "" )
)

GenerateClass(
	EtherTimeOfDayModifier,
	AddField( "Hour", KBTYPEINFO_FLOAT, EtherTimeOfDayModifier, m_Hour, false, "" )
	AddField( "SunColor", KBTYPEINFO_VECTOR4, EtherTimeOfDayModifier, m_SunColor, false, "" )
	AddField( "FogColor", KBTYPEINFO_VECTOR4, EtherTimeOfDayModifier, m_FogColor, false, "" )
	AddField( "SkyColor", KBTYPEINFO_VECTOR4, EtherTimeOfDayModifier, m_SkyColor, false, "" )
	AddField( "LightShaftColor", KBTYPEINFO_VECTOR4, EtherTimeOfDayModifier, m_LightShaftColor, false, "" )
)
