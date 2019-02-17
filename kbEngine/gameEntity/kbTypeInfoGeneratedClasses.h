// AUTO GENERATED TYPE INFO //////////////////////////////////////////

GenerateEnum( 
	ERenderPass, "ERenderPass",
	AddEnumField( RP_FirstPerson, "FirstPersonPass" )
	AddEnumField( RP_Lighting, "LightingPass" )
	AddEnumField( RP_Translucent, "TranlucentPass" )
	AddEnumField( RP_LightingPass, "Post-LightingPass" )
	AddEnumField( RP_Debug, "DebugPass" )
)

GenerateEnum( 
	EClothType, "EClothType",
	AddEnumField( CT_None, "None" )
	AddEnumField( CT_Square, "Square" )
)

GenerateEnum( 
	ECollisionType, "ECollisionType",
	AddEnumField( CollisionType_Sphere, "Sphere" )
	AddEnumField( CollisionType_Box, "Box" )
	AddEnumField( CollisionType_StaticMesh, "StaticMesh" )
	AddEnumField( CollisionType_CustomTriangles, "CustomTriangles" )
)

GenerateClass( 
	kbComponent, 
	AddField( "Enabled", KBTYPEINFO_BOOL, kbGameComponent, m_IsEnabled, false, "" )
)

GenerateClass( 
	kbGameComponent, 
	AddField( "LifeTime", KBTYPEINFO_FLOAT, kbGameComponent, m_StartingLifeTime, false, "" )
)

GenerateClass(
	kbShaderParamComponent,
	AddField( "ParamName", KBTYPEINFO_KBSTRING, kbShaderParamComponent, m_ParamName, false, "" )
	AddField( "Texture", KBTYPEINFO_TEXTURE, kbShaderParamComponent, m_pTexture, false, "" )
	AddField( "Vector", KBTYPEINFO_VECTOR4, kbShaderParamComponent, m_Vector, false, "" )
)

GenerateClass(
	kbMaterialComponent,
	AddField( "Shader", KBTYPEINFO_SHADER, kbMaterialComponent, m_pShader, false, "" )
	AddField( "ShaderParams", KBTYPEINFO_STRUCT, kbMaterialComponent, m_ShaderParamComponents, true, "kbShaderParamComponent" )
)

GenerateClass(
	kbClothBone,
	AddField( "BoneName", KBTYPEINFO_KBSTRING, kbClothBone, m_BoneName, false, "" )
	AddField( "NeighborBones", KBTYPEINFO_KBSTRING, kbClothBone, m_NeighborBones, true, "kbString" )
	AddField( "Anchored", KBTYPEINFO_BOOL, kbClothBone, m_bIsAnchored, false, "" )
)

GenerateClass(
	kbBoneCollisionSphere,
	AddField( "BoneName", KBTYPEINFO_KBSTRING, kbBoneCollisionSphere, m_BoneName, false, "" )
	AddField( "Sphere", KBTYPEINFO_VECTOR4, kbBoneCollisionSphere, m_Sphere, false, "" )
)

GenerateClass(
	kbClothComponent,
	AddField( "BoneInfo", KBTYPEINFO_STRUCT, kbClothComponent, m_BoneInfo, true, "kbClothBone" )
	AddField( "ClothType", KBTYPEINFO_ENUM, kbClothComponent, m_ClothType, false, "EClothType" )
	AddField( "NumIterations", KBTYPEINFO_INT, kbClothComponent, m_NumConstrainIterations, false, "" )
	AddField( "Width", KBTYPEINFO_INT, kbClothComponent, m_Width, false, "" )
	AddField( "Height", KBTYPEINFO_INT, kbClothComponent, m_Height, false, "" )
	AddField( "AdditionalBoneInfo", KBTYPEINFO_STRUCT, kbClothComponent, m_AdditionalBoneInfo, true, "kbClothBone" )
	AddField( "Collision", KBTYPEINFO_STRUCT, kbClothComponent, m_CollisionSpheres, true, "kbBoneCollisionSphere" )
)

GenerateClass(
	kbTransformComponent,
	AddField( "Name", KBTYPEINFO_KBSTRING, kbTransformComponent, m_Name, false, "" )
	AddField( "Position", KBTYPEINFO_VECTOR, kbTransformComponent, m_Position, false, "" )
	AddField( "Scale", KBTYPEINFO_VECTOR, kbTransformComponent, m_Scale, false, "" )
	AddField( "Orientation", KBTYPEINFO_VECTOR4, kbTransformComponent, m_Orientation, false, "" )
)

GenerateClass(
	kbModelComponent,
	AddField( "RenderPass", KBTYPEINFO_ENUM, kbModelComponent, m_RenderPass, false, "ERenderPass" )
	AddField( "TranslucencySortBias", KBTYPEINFO_FLOAT, kbModelComponent, m_TranslucencySortBias, false, "" )
	AddField( "CastsShadow", KBTYPEINFO_BOOL, kbModelComponent, m_bCastsShadow, false, "" )
	AddField( "Materials", KBTYPEINFO_STRUCT, kbModelComponent, m_MaterialList, true, "kbMaterialComponent" )
)

GenerateClass( 
	kbStaticModelComponent, 
	AddField( "Model", KBTYPEINFO_STATICMODEL, kbStaticModelComponent, m_pModel, false, "" )
	AddField( "ShaderOverride", KBTYPEINFO_SHADER, kbStaticModelComponent, m_pOverrideShaderList, true, "" )
)

GenerateClass( 
	kbSkeletalModelComponent, 
	AddField( "Model", KBTYPEINFO_STATICMODEL, kbSkeletalModelComponent, m_pModel, false, "" )
	AddField( "ShaderOverride", KBTYPEINFO_SHADER, kbSkeletalModelComponent, m_pOverrideShaderList, true, "" )
)

GenerateClass( 
	kbLightComponent,
	AddField( "Color", KBTYPEINFO_VECTOR4, kbLightComponent, m_Color, false, "" )
	AddField( "CastsShadows", KBTYPEINFO_BOOL, kbLightComponent, m_bCastsShadow, false, "" )
	AddField( "Materials", KBTYPEINFO_STRUCT, kbLightComponent, m_MaterialList, true, "kbMaterialComponent" )
)

GenerateClass(
	kbLightShaftsComponent,
	AddField( "Texture", KBTYPEINFO_TEXTURE, kbLightShaftsComponent, m_Texture, false, "" )
	AddField( "Color", KBTYPEINFO_VECTOR4, kbLightShaftsComponent, m_Color, false, "" )
	AddField( "BaseWidth", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_BaseWidth, false, "" )
	AddField( "BaseHeight", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_BaseHeight, false, "" )
	AddField( "IterationWidth", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_IterationWidth, false, "" )
	AddField( "IterationHeight", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_IterationHeight, false, "" )
	AddField( "NumIteractions", KBTYPEINFO_INT, kbLightShaftsComponent, m_NumIterations, false, "" )
	AddField( "Directional", KBTYPEINFO_BOOL, kbLightShaftsComponent, m_Directional, false, "" )
)

GenerateClass(
	kbFogComponent,
	AddField( "Color", KBTYPEINFO_VECTOR4, kbFogComponent, m_Color, false, "" )
	AddField( "StartDistance", KBTYPEINFO_FLOAT, kbFogComponent, m_StartDistance, false, "" )
	AddField( "EndDistance", KBTYPEINFO_FLOAT, kbFogComponent, m_EndDistance, false, "" )
)

GenerateClass( 
	kbDirectionalLightComponent,
	AddField( "CascadedShadowSplits", KBTYPEINFO_FLOAT, kbDirectionalLightComponent, m_SplitDistances, true, "float" )
)

GenerateClass( 
	kbPointLightComponent,
	AddField( "LightRadius", KBTYPEINFO_FLOAT, kbPointLightComponent, m_Radius, false, "float" )
)

GenerateClass( 
	kbCylindricalLightComponent,
	AddField( "Length", KBTYPEINFO_FLOAT, kbCylindricalLightComponent, m_Length, false, "float" )
)

GenerateClass(
	kbGrass,
	AddField( "GrassMap", KBTYPEINFO_TEXTURE, kbGrass, m_pGrassMap, false, "" )
    AddField( "NoiseMap", KBTYPEINFO_TEXTURE, kbGrass, m_pNoiseMap, false, "" )
	AddField( "GrassCellsPerTerrainSide", KBTYPEINFO_INT, kbGrass, m_GrassCellsPerTerrainSide, false, "" )
	AddField( "PatchStartCullDistance", KBTYPEINFO_FLOAT, kbGrass, m_PatchStartCullDistance, false, "" )
	AddField( "PatchEndCullDistance", KBTYPEINFO_FLOAT, kbGrass, m_PatchEndCullDistance, false, "" )
	AddField( "PatchesPerCellSide", KBTYPEINFO_INT, kbGrass, m_PatchesPerCellSide, false, "" )
	AddField( "BladeMinWidth", KBTYPEINFO_FLOAT, kbGrass, m_BladeMinWidth, false, "" )
	AddField( "BladeMaxWidth", KBTYPEINFO_FLOAT, kbGrass, m_BladeMaxWidth, false, "" )
	AddField( "BladeMinHeight", KBTYPEINFO_FLOAT, kbGrass, m_BladeMinHeight, false, "" )
	AddField( "BladeMaxHeight", KBTYPEINFO_FLOAT, kbGrass, m_BladeMaxHeight, false, "" )
	AddField( "MaxBladeJitterOffset", KBTYPEINFO_FLOAT, kbGrass, m_MaxBladeJitterOffset, false, "" )
	AddField( "MaxPatchJitterOffset", KBTYPEINFO_FLOAT, kbGrass, m_MaxPatchJitterOffset, false, "" )
	AddField( "DiffuseMap", KBTYPEINFO_TEXTURE, kbGrass, m_pDiffuseMap, false, "" )
    AddField( "TestWind", KBTYPEINFO_VECTOR, kbGrass, m_TestWind, false, "" )
    AddField( "FakeAODarkness", KBTYPEINFO_FLOAT, kbGrass, m_FakeAODarkness, false, "" )
    AddField( "FakeAOPower", KBTYPEINFO_FLOAT, kbGrass, m_FakeAOPower, false, "" )
	AddField( "ShaderParams", KBTYPEINFO_STRUCT, kbGrass, m_ShaderParamList, true, "kbShaderParamComponent" )
)

GenerateClass(
	kbTerrainMatComponent,
	AddField( "DiffuseMap", KBTYPEINFO_TEXTURE, kbTerrainMatComponent, m_pDiffuseMap, false, "" )
	AddField( "NormalMap", KBTYPEINFO_TEXTURE, kbTerrainMatComponent, m_pNormalMap, false, "" )
	AddField( "SpecMap", KBTYPEINFO_TEXTURE, kbTerrainMatComponent, m_pSpecMap, false, "" )
	AddField( "SpecFactors", KBTYPEINFO_FLOAT, kbTerrainMatComponent, m_SpecFactor, false, "" )
	AddField( "SpecPowerMultiplier", KBTYPEINFO_FLOAT, kbTerrainMatComponent, m_SpecPowerMultiplier, false, "" )
	AddField( "UVScale", KBTYPEINFO_VECTOR, kbTerrainMatComponent, m_UVScale, false, "" )
)

GenerateClass(
	kbTerrainComponent,
	AddField( "HeightMap", KBTYPEINFO_TEXTURE, kbTerrainComponent, m_pHeightMap, false, "" )
	AddField( "HeightScale", KBTYPEINFO_FLOAT, kbTerrainComponent, m_HeightScale, false, "" )
	AddField( "Width", KBTYPEINFO_FLOAT, kbTerrainComponent, m_TerrainWidth, false, "" )
	AddField( "Dimensions", KBTYPEINFO_INT, kbTerrainComponent, m_TerrainDimensions, false, "" )
	AddField( "MaterialList", KBTYPEINFO_STRUCT, kbTerrainComponent, m_MaterialList, true, "kbMaterialComponent" )
	AddField( "TerrainMaterials", KBTYPEINFO_STRUCT, kbTerrainComponent, m_TerrainMaterials, true, "kbTerrainMatComponent" )
	AddField( "SplatMap", KBTYPEINFO_TEXTURE, kbTerrainComponent, m_pSplatMap, false, "" )
	AddField( "Shader", KBTYPEINFO_SHADER, kbTerrainComponent, m_pTerrainShader, false, "" )
	AddField( "Grass", KBTYPEINFO_STRUCT, kbTerrainComponent, m_Grass, true, "kbGrass" )
)

GenerateEnum(
   EBillboardType, "EBillboardType",
   AddEnumField( BT_FaceCamera, "FaceCamera" )
   AddEnumField( BT_AxialBillboard, "AxialBillboard" )
)

GenerateClass(
	kbParticleComponent,
	AddField( "TranslucencySortBias", KBTYPEINFO_FLOAT, kbParticleComponent, m_TranslucencySortBias, false, "" )
	AddField( "ParticleShader", KBTYPEINFO_SHADER, kbParticleComponent, m_pParticleShader, false, "" )
	AddField( "MaterialList", KBTYPEINFO_STRUCT, kbParticleComponent, m_MaterialList, true, "kbMaterialComponent" )
	AddField( "TotalDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_TotalDuration, false, "" )
	AddField( "MinSpawnRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MinParticleSpawnRate, false, "" )
	AddField( "MaxSpawnRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MaxParticleSpawnRate, false, "" )

	AddField( "MinDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_ParticleMinDuration, false, "" )
	AddField( "MaxDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_ParticleMaxDuration, false, "" )

	AddField( "MinStartVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleStartVelocity, false, "" )
	AddField( "MaxStartVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleStartVelocity, false, "" )
	AddField( "MinEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndVelocity, false, "" )
	AddField( "MaxEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndVelocity, false, "" )
	AddField( "MinEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndVelocity, false, "" )
	AddField( "MaxEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndVelocity, false, "" )
	AddField( "LockVelocity", KBTYPEINFO_BOOL, kbParticleComponent, m_bLockVelocity, false, "" )

	AddField( "MinStartSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleStartSize, false, "" )
	AddField( "MaxStartSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleStartSize, false, "" )
	AddField( "MinEndSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndSize, false, "" )
	AddField( "MaxEndSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndSize, false, "" )

	AddField( "StartColor", KBTYPEINFO_VECTOR4, kbParticleComponent, m_ParticleStartColor, false, "" )
	AddField( "EndColor", KBTYPEINFO_VECTOR4, kbParticleComponent, m_ParticleEndColor, false, "" )
	AddField( "MaxBurstCount", KBTYPEINFO_INT, kbParticleComponent, m_MaxBurstCount, false, "" )
	AddField( "MinBurstCount", KBTYPEINFO_INT, kbParticleComponent, m_MinBurstCount, false, "" )
	AddField( "Gravity", KBTYPEINFO_VECTOR, kbParticleComponent, m_Gravity, false, "" )

	AddField( "ParticleBillboardType", KBTYPEINFO_ENUM, kbParticleComponent, m_ParticleBillboardType, false, "EBillboardType" )
	
)

GenerateClass(
	kbGameLogicComponent,
	AddField( "DummyTemp", KBTYPEINFO_INT, kbGameLogicComponent, m_DummyTemp, false, "" )
)

GenerateClass(
	kbDamageComponent,
	AddField( "MinDamage", KBTYPEINFO_FLOAT, kbDamageComponent, m_MinDamage, false, "" )
	AddField( "MaxDamage", KBTYPEINFO_FLOAT, kbDamageComponent, m_MaxDamage, false, "" )
)

GenerateClass(
	kbActorComponent,
	AddField( "Health", KBTYPEINFO_FLOAT, kbActorComponent, m_MaxHealth, false, "" )
)

GenerateClass(
	kbPlayerStartComponent,
	AddField( "Dummy", KBTYPEINFO_FLOAT, kbPlayerStartComponent, m_DummyVar, false, "" )
)

GenerateClass(
	kbCollisionComponent,
	AddField( "CollisionType", KBTYPEINFO_ENUM, kbCollisionComponent, m_CollisionType, false, "ECollisionType" )
	AddField( "Extent", KBTYPEINFO_VECTOR, kbCollisionComponent, m_Extent, false, "" )
	AddField( "SphereCollision", KBTYPEINFO_STRUCT, kbCollisionComponent, m_LocalSpaceCollisionSpheres, true, "kbBoneCollisionSphere" )
)

GenerateClass(
	kbSoundData,
	AddField( "WaveFile", KBTYPEINFO_SOUNDWAVE, kbSoundData, m_pWaveFile, false, "" )
	AddField( "Radius", KBTYPEINFO_FLOAT, kbSoundData, m_Radius, false, "" )
	AddField( "Volume", KBTYPEINFO_FLOAT, kbSoundData, m_Volume, false, "" )
)

GenerateClass(
	kbDebugSphereCollision,
	AddField( "CollisionModel", KBTYPEINFO_STATICMODEL, kbDebugSphereCollision, m_pCollisionModel, false, "" )
)