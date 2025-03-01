// AUTO GENERATED TYPE INFO //////////////////////////////////////////

GenerateEnum(
	ERenderPass, "ERenderPass",
	AddEnumField(RP_FirstPerson, "FirstPersonPass")
	AddEnumField(RP_Lighting, "LightingPass")
	AddEnumField(RP_Translucent, "TranslucentPass")
	AddEnumField(RP_TranslucentWithDepth, "TranslucentWithDepthPass")
	AddEnumField(RP_LightingPass, "Post-LightingPass")
	AddEnumField(RP_InWorldUI, "In-World UI")
	AddEnumField(RP_Distortion, "DistortionPass")
	AddEnumField(RP_PostProcess, "PostProcess")
	AddEnumField(RP_UI, "UIPass")
	AddEnumField(RP_Debug, "DebugPass")
)

GenerateEnum(
	ECullMode, "ECullMode",
	AddEnumField(CullMode_ShaderDefault, "ShaderDefault")
	AddEnumField(CullMode_None, "None")
	AddEnumField(CullMode_FrontFaces, "FrontFaces")
	AddEnumField(CullMode_BackFaces, "BackFaces")
)

GenerateEnum(
	EClothType, "EClothType",
	AddEnumField(CT_None, "None")
	AddEnumField(CT_Square, "Square")
)

GenerateEnum(
	ECollisionType, "ECollisionType",
	AddEnumField(CollisionType_Sphere, "Sphere")
	AddEnumField(CollisionType_Box, "Box")
	AddEnumField(CollisionType_StaticMesh, "StaticMesh")
	AddEnumField(CollisionType_CustomTriangles, "CustomTriangles")
)

GenerateClass(
	kbComponent,
	AddField("Enabled", KBTYPEINFO_BOOL, kbGameComponent, m_IsEnabled, false, "")
)

GenerateClass(
	kbGameComponent,
	AddField("LifeTime", KBTYPEINFO_FLOAT, kbGameComponent, m_StartingLifeTime, false, "")
)

GenerateClass(
	kbEditorGlobalSettingsComponent,
	AddField("CameraSpeedIdx", KBTYPEINFO_INT, kbEditorGlobalSettingsComponent, m_CameraSpeedIdx, false, "")
)

GenerateClass(
	kbEditorLevelSettingsComponent,
	AddField("MainCameraPosition", KBTYPEINFO_VECTOR, kbEditorLevelSettingsComponent, m_CameraPosition, false, "")
	AddField("MainCameraRotation", KBTYPEINFO_VECTOR4, kbEditorLevelSettingsComponent, m_CameraRotation, false, "")
)

GenerateClass(
	kbAnimEvent,
	AddField("EventName", KBTYPEINFO_KBSTRING, kbAnimEvent, m_EventName, false, "")
	AddField("EventTime", KBTYPEINFO_FLOAT, kbAnimEvent, m_EventTime, false, "")
	AddField("EventValue", KBTYPEINFO_FLOAT, kbAnimEvent, m_EventValue, false, "")
)

GenerateClass(
	kbVectorAnimEvent,
	AddField("EventName", KBTYPEINFO_KBSTRING, kbVectorAnimEvent, m_EventName, false, "")
	AddField("EventTime", KBTYPEINFO_FLOAT, kbVectorAnimEvent, m_EventTime, false, "")
	AddField("EventValue", KBTYPEINFO_VECTOR4, kbVectorAnimEvent, m_EventValue, false, "")
)


GenerateClass(
	kbShaderParamComponent,
	AddField("ParamName", KBTYPEINFO_KBSTRING, kbShaderParamComponent, m_param_name, false, "")
	AddField("Texture", KBTYPEINFO_TEXTURE, kbShaderParamComponent, m_texture, false, "")
	AddField("Vector", KBTYPEINFO_VECTOR4, kbShaderParamComponent, m_vector, false, "")
)

GenerateClass(
	kbMaterialComponent,
	AddField("Shader", KBTYPEINFO_SHADER, kbMaterialComponent, m_shader, false, "")
	AddField("ShaderParams", KBTYPEINFO_STRUCT, kbMaterialComponent, m_shader_params, true, "kbShaderParamComponent")
	AddField("CullModeOverride", KBTYPEINFO_ENUM, kbMaterialComponent, m_cull_override, false, "ECullMode")
)

GenerateClass(
	kbModelEmitter,
	AddField("Model", KBTYPEINFO_STATICMODEL, kbModelEmitter, m_model, false, "")
	AddField("MaterialList", KBTYPEINFO_STRUCT, kbModelEmitter, m_materials, true, "kbMaterialComponent")
)

GenerateClass(
	kbClothBone,
	AddField("BoneName", KBTYPEINFO_KBSTRING, kbClothBone, m_BoneName, false, "")
	AddField("NeighborBones", KBTYPEINFO_KBSTRING, kbClothBone, m_NeighborBones, true, "kbString")
	AddField("Anchored", KBTYPEINFO_BOOL, kbClothBone, m_bIsAnchored, false, "")
)

GenerateClass(
	kbBoneCollisionSphere,
	AddField("BoneName", KBTYPEINFO_KBSTRING, kbBoneCollisionSphere, m_BoneName, false, "")
	AddField("Sphere", KBTYPEINFO_VECTOR4, kbBoneCollisionSphere, m_Sphere, false, "")
)

GenerateClass(
	kbClothComponent,
	AddField("BoneInfo", KBTYPEINFO_STRUCT, kbClothComponent, m_BoneInfo, true, "kbClothBone")
	AddField("ClothType", KBTYPEINFO_ENUM, kbClothComponent, m_ClothType, false, "EClothType")
	AddField("NumIterations", KBTYPEINFO_INT, kbClothComponent, m_NumConstrainIterations, false, "")
	AddField("Width", KBTYPEINFO_INT, kbClothComponent, m_Width, false, "")
	AddField("Height", KBTYPEINFO_INT, kbClothComponent, m_Height, false, "")
	AddField("AdditionalBoneInfo", KBTYPEINFO_STRUCT, kbClothComponent, m_AdditionalBoneInfo, true, "kbClothBone")
	AddField("Collision", KBTYPEINFO_STRUCT, kbClothComponent, m_CollisionSpheres, true, "kbBoneCollisionSphere")
	AddField("Gravity", KBTYPEINFO_VECTOR, kbClothComponent, m_gravity, false, "")
	AddField("MinWindVelocity", KBTYPEINFO_VECTOR, kbClothComponent, m_MinWindVelocity, false, "")
	AddField("MaxWindVelocity", KBTYPEINFO_VECTOR, kbClothComponent, m_MaxWindVelocity, false, "")
	AddField("MinWindGustDuration", KBTYPEINFO_FLOAT, kbClothComponent, m_MinWindGustDuration, false, "")
	AddField("MaxWindGustDuration", KBTYPEINFO_FLOAT, kbClothComponent, m_MaxWindGustDuration, false, "")
	AddField("AddFakeOscillation", KBTYPEINFO_BOOL, kbClothComponent, m_bAddFakeOscillation, false, "")
)

GenerateClass(
	kbTransformComponent,
	AddField("Name", KBTYPEINFO_KBSTRING, kbTransformComponent, m_Name, false, "")
	AddField("Position", KBTYPEINFO_VECTOR, kbTransformComponent, m_position, false, "")
	AddField("Scale", KBTYPEINFO_VECTOR, kbTransformComponent, m_Scale, false, "")
	AddField("Orientation", KBTYPEINFO_VECTOR4, kbTransformComponent, m_Orientation, false, "")
)

GenerateClass(
	RenderComponent,
	AddField("RenderPass", KBTYPEINFO_ENUM, RenderComponent, m_render_pass, false, "ERenderPass")
	AddField("RenderOrderBias", KBTYPEINFO_FLOAT, RenderComponent, m_render_order_bias, false, "")
	AddField("CastsShadow", KBTYPEINFO_BOOL, RenderComponent, m_casts_shadow, false, "")
	AddField("Materials", KBTYPEINFO_STRUCT, RenderComponent, m_materials, true, "kbMaterialComponent")
)

GenerateClass(
	kbStaticModelComponent,
	AddField("Model", KBTYPEINFO_STATICMODEL, kbStaticModelComponent, m_model, false, "")
)

GenerateClass(
	kbAnimComponent,
	AddField("AnimationName", KBTYPEINFO_KBSTRING, kbAnimComponent, m_animation_name, false, "")
	AddField("Animation", KBTYPEINFO_ANIMATION, kbAnimComponent, m_animation, false, "")
	AddField("TimeScale", KBTYPEINFO_FLOAT, kbAnimComponent, m_time_scale, false, "")
	AddField("IsLooping", KBTYPEINFO_BOOL, kbAnimComponent, m_is_looping, false, "")
	AddField("AnimationEvent", KBTYPEINFO_STRUCT, kbAnimComponent, m_anim_events, true, "kbAnimEvent")
)

GenerateClass(
	SkeletalModelComponent,
	AddField("Model", KBTYPEINFO_STATICMODEL, SkeletalModelComponent, m_model, false, "")
	AddField("Animations", KBTYPEINFO_STRUCT, SkeletalModelComponent, m_Animations, true, "kbAnimComponent")
	AddField("DebugAnimIndex", KBTYPEINFO_INT, SkeletalModelComponent, m_DebugAnimIdx, false, "")
)

GenerateClass(
	kbFlingPhysicsComponent,
	AddField("MinLinearVelocity", KBTYPEINFO_VECTOR, kbFlingPhysicsComponent, m_min_linear_vel, false, "")
	AddField("MaxLinearVelocity", KBTYPEINFO_VECTOR, kbFlingPhysicsComponent, m_max_linear_vel, false, "")
	AddField("MinAngularSpeed", KBTYPEINFO_FLOAT, kbFlingPhysicsComponent, m_MinAngularSpeed, false, "")
	AddField("MaxAngularSpeed", KBTYPEINFO_FLOAT, kbFlingPhysicsComponent, m_MaxAngularSpeed, false, "")
	AddField("Gravity", KBTYPEINFO_VECTOR, kbFlingPhysicsComponent, m_max_linear_vel, false, "")
)

GenerateClass(
	kbLightComponent,
	AddField("Color", KBTYPEINFO_VECTOR4, kbLightComponent, m_Color, false, "")
	AddField("CastsShadows", KBTYPEINFO_BOOL, kbLightComponent, m_casts_shadow, false, "")
	AddField("Materials", KBTYPEINFO_STRUCT, kbLightComponent, m_materials, true, "kbMaterialComponent")
)

GenerateClass(
	kbLightShaftsComponent,
	AddField("Texture", KBTYPEINFO_TEXTURE, kbLightShaftsComponent, m_Texture, false, "")
	AddField("Color", KBTYPEINFO_VECTOR4, kbLightShaftsComponent, m_Color, false, "")
	AddField("BaseWidth", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_BaseWidth, false, "")
	AddField("BaseHeight", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_BaseHeight, false, "")
	AddField("IterationWidth", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_IterationWidth, false, "")
	AddField("IterationHeight", KBTYPEINFO_FLOAT, kbLightShaftsComponent, m_IterationHeight, false, "")
	AddField("NumIteractions", KBTYPEINFO_INT, kbLightShaftsComponent, m_NumIterations, false, "")
	AddField("Directional", KBTYPEINFO_BOOL, kbLightShaftsComponent, m_Directional, false, "")
)

GenerateClass(
	kbFogComponent,
	AddField("Color", KBTYPEINFO_VECTOR4, kbFogComponent, m_Color, false, "")
	AddField("StartDistance", KBTYPEINFO_FLOAT, kbFogComponent, m_StartDistance, false, "")
	AddField("EndDistance", KBTYPEINFO_FLOAT, kbFogComponent, m_EndDistance, false, "")
)

GenerateClass(
	kbDirectionalLightComponent,
	AddField("CascadedShadowSplits", KBTYPEINFO_FLOAT, kbDirectionalLightComponent, m_SplitDistances, true, "float")
)

GenerateClass(
	kbPointLightComponent,
	AddField("LightRadius", KBTYPEINFO_FLOAT, kbPointLightComponent, m_Radius, false, "float")
)

GenerateClass(
	kbCylindricalLightComponent,
	AddField("Length", KBTYPEINFO_FLOAT, kbCylindricalLightComponent, m_Length, false, "float")
)

GenerateClass(
	kbGrass,
	AddField("GrassShader", KBTYPEINFO_SHADER, kbGrass, m_pGrassShader, false, "")
	AddField("GrassCellsPerTerrainSide", KBTYPEINFO_INT, kbGrass, m_GrassCellsPerTerrainSide, false, "")
	AddField("PatchStartCullDistance", KBTYPEINFO_FLOAT, kbGrass, m_PatchStartCullDistance, false, "")
	AddField("PatchEndCullDistance", KBTYPEINFO_FLOAT, kbGrass, m_PatchEndCullDistance, false, "")
	AddField("PatchesPerCellSide", KBTYPEINFO_INT, kbGrass, m_PatchesPerCellSide, false, "")
	AddField("BladeMinWidth", KBTYPEINFO_FLOAT, kbGrass, m_BladeMinWidth, false, "")
	AddField("BladeMaxWidth", KBTYPEINFO_FLOAT, kbGrass, m_BladeMaxWidth, false, "")
	AddField("BladeMinHeight", KBTYPEINFO_FLOAT, kbGrass, m_BladeMinHeight, false, "")
	AddField("BladeMaxHeight", KBTYPEINFO_FLOAT, kbGrass, m_BladeMaxHeight, false, "")
	AddField("MaxBladeJitterOffset", KBTYPEINFO_FLOAT, kbGrass, m_MaxBladeJitterOffset, false, "")
	AddField("MaxPatchJitterOffset", KBTYPEINFO_FLOAT, kbGrass, m_MaxPatchJitterOffset, false, "")
	AddField("FakeAODarkness", KBTYPEINFO_FLOAT, kbGrass, m_FakeAODarkness, false, "")
	AddField("FakeAOPower", KBTYPEINFO_FLOAT, kbGrass, m_FakeAOPower, false, "")
	AddField("FakeAOClipPlaneFadeOutDist", KBTYPEINFO_FLOAT, kbGrass, m_FakeAOClipPlaneFadeStartDist, false, "")
	AddField("ShaderParams", KBTYPEINFO_STRUCT, kbGrass, m_ShaderParamList, true, "kbShaderParamComponent")
)

GenerateClass(
	kbGrassZone,
	AddField("Center", KBTYPEINFO_VECTOR, kbGrassZone, m_Center, false, "")
	AddField("Extents", KBTYPEINFO_VECTOR, kbGrassZone, m_Extents, false, "")
)

GenerateClass(
	kbTerrainComponent,
	AddField("HeightMap", KBTYPEINFO_TEXTURE, kbTerrainComponent, m_pHeightMap, false, "")
	AddField("HeightScale", KBTYPEINFO_FLOAT, kbTerrainComponent, m_HeightScale, false, "")
	AddField("Width", KBTYPEINFO_FLOAT, kbTerrainComponent, m_TerrainWidth, false, "")
	AddField("Dimensions", KBTYPEINFO_INT, kbTerrainComponent, m_TerrainDimensions, false, "")
	AddField("SmoothAmount", KBTYPEINFO_INT, kbTerrainComponent, m_TerrainSmoothAmount, false, "")
	AddField("SplatMap", KBTYPEINFO_TEXTURE, kbTerrainComponent, m_pSplatMap, false, "")
	AddField("Grass", KBTYPEINFO_STRUCT, kbTerrainComponent, m_Grass, true, "kbGrass")
	AddField("DebugRegenTerrain", KBTYPEINFO_BOOL, kbTerrainComponent, m_bDebugForceRegenTerrain, false, "")
	AddField("GrassZones", KBTYPEINFO_STRUCT, kbTerrainComponent, m_GrassZones, true, "kbGrassZone")
)

GenerateEnum(
	EBillboardType, "EBillboardType",
	AddEnumField(BT_FaceCamera, "FaceCamera")
	AddEnumField(BT_AxialBillboard, "AxialBillboard")
	AddEnumField(BT_AlignAlongVelocity, "AlignAlongVelocity")
)

GenerateClass(
	kbParticleComponent,
	AddField("DebugPlayEntity", KBTYPEINFO_BOOL, kbParticleComponent, m_DebugPlayEntity, false, "")
	AddField("RenderOrderBias", KBTYPEINFO_FLOAT, kbParticleComponent, m_render_order_bias, false, "")
	AddField("MaterialList", KBTYPEINFO_STRUCT, kbParticleComponent, m_materials, true, "kbMaterialComponent")
	AddField("TotalDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_TotalDuration, false, "")
	AddField("StartDelay", KBTYPEINFO_FLOAT, kbParticleComponent, m_StartDelay, false, "")
	AddField("MinSpawnRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MinParticleSpawnRate, false, "")
	AddField("MaxSpawnRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MaxParticleSpawnRate, false, "")
	AddField("MaxParticlesToEmit", KBTYPEINFO_INT, kbParticleComponent, m_MaxParticlesToEmit, false, "")

	AddField("MinDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_ParticleMinDuration, false, "")
	AddField("MaxDuration", KBTYPEINFO_FLOAT, kbParticleComponent, m_ParticleMaxDuration, false, "")

	AddField("MinStartVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleStartVelocity, false, "")
	AddField("MaxStartVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleStartVelocity, false, "")
	AddField("MinEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndVelocity, false, "")
	AddField("MaxEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndVelocity, false, "")
	AddField("MinEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndVelocity, false, "")
	AddField("VelocityCurve", KBTYPEINFO_STRUCT, kbParticleComponent, m_velocityOverLifeTimeCurve, true, "kbAnimEvent")
	AddField("MaxEndVelocity", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndVelocity, false, "")

	AddField("MinStartSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleStartSize, false, "")
	AddField("MaxStartSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleStartSize, false, "")
	AddField("MinEndSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinParticleEndSize, false, "")
	AddField("MaxEndSize", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxParticleEndSize, false, "")

	AddField("StartColor", KBTYPEINFO_VECTOR4, kbParticleComponent, m_ParticleStartColor, false, "")
	AddField("EndColor", KBTYPEINFO_VECTOR4, kbParticleComponent, m_ParticleEndColor, false, "")

	AddField("SizeOverLife", KBTYPEINFO_STRUCT, kbParticleComponent, m_SizeOverLifeTimeCurve, true, "kbVectorAnimEvent")
	AddField("RotationOverLife", KBTYPEINFO_STRUCT, kbParticleComponent, m_RotationOverLifeTimeCurve, true, "kbVectorAnimEvent")
	AddField("ColorOverLife", KBTYPEINFO_STRUCT, kbParticleComponent, m_ColorOverLifeTimeCurve, true, "kbVectorAnimEvent")
	AddField("AlphaOverLife", KBTYPEINFO_STRUCT, kbParticleComponent, m_AlphaOverLifeTimeCurve, true, "kbAnimEvent")

	AddField("MaxBurstCount", KBTYPEINFO_INT, kbParticleComponent, m_MaxBurstCount, false, "")
	AddField("MinBurstCount", KBTYPEINFO_INT, kbParticleComponent, m_MinBurstCount, false, "")

	AddField("MinStartRotationRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MinStartRotationRate, false, "")
	AddField("MaxStartRotationRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MaxStartRotationRate, false, "")

	AddField("MinEndRotationRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MinEndRotationRate, false, "")
	AddField("MaxEndRotationRate", KBTYPEINFO_FLOAT, kbParticleComponent, m_MaxEndRotationRate, false, "")

	AddField("MinStart3DRotation", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinStart3DRotation, false, "")
	AddField("MaxStart3DRotation", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxStart3DRotation, false, "")

	AddField("MinStart3DOffset", KBTYPEINFO_VECTOR, kbParticleComponent, m_MinStart3DOffset, false, "")
	AddField("MaxStart3DOffset", KBTYPEINFO_VECTOR, kbParticleComponent, m_MaxStart3DOffset, false, "")

	AddField("Gravity", KBTYPEINFO_VECTOR, kbParticleComponent, m_gravity, false, "")

	AddField("ModelEmitter", KBTYPEINFO_STRUCT, kbParticleComponent, m_ModelEmitter, true, "kbModelEmitter")

	AddField("ParticleBillboardType", KBTYPEINFO_ENUM, kbParticleComponent, m_ParticleBillboardType, false, "EBillboardType")

)

GenerateClass(
	kbGameLogicComponent,
	AddField("DummyTemp", KBTYPEINFO_INT, kbGameLogicComponent, m_DummyTemp, false, "")
)

GenerateClass(
	kbDamageComponent,
	AddField("MinDamage", KBTYPEINFO_FLOAT, kbDamageComponent, m_MinDamage, false, "")
	AddField("MaxDamage", KBTYPEINFO_FLOAT, kbDamageComponent, m_MaxDamage, false, "")
)

GenerateClass(
	kbActorComponent,
	AddField("Health", KBTYPEINFO_FLOAT, kbActorComponent, m_MaxHealth, false, "")
)

GenerateClass(
	kbPlayerStartComponent,
	AddField("Dummy", KBTYPEINFO_FLOAT, kbPlayerStartComponent, m_DummyVar, false, "")
)

GenerateClass(
	kbCollisionComponent,
	AddField("CollisionType", KBTYPEINFO_ENUM, kbCollisionComponent, m_CollisionType, false, "ECollisionType")
	AddField("Extent", KBTYPEINFO_VECTOR, kbCollisionComponent, m_Extent, false, "")
	AddField("SphereCollision", KBTYPEINFO_STRUCT, kbCollisionComponent, m_LocalSpaceCollisionSpheres, true, "kbBoneCollisionSphere")
)

GenerateClass(
	kbSoundData,
	AddField("WaveFile", KBTYPEINFO_SOUNDWAVE, kbSoundData, m_pWaveFile, false, "")
	AddField("Radius", KBTYPEINFO_FLOAT, kbSoundData, m_Radius, false, "")
	AddField("Volume", KBTYPEINFO_FLOAT, kbSoundData, m_Volume, false, "")
	AddField("Looping", KBTYPEINFO_BOOL, kbSoundData, m_bLooping, false, "")
	AddField("TestPlaySoundNow", KBTYPEINFO_BOOL, kbSoundData, m_bDebugPlaySound, false, "")
)

GenerateClass(
	kbPlaySoundComponent,
	AddField("MinStartDelay", KBTYPEINFO_FLOAT, kbPlaySoundComponent, m_MinStartDelay, false, "")
	AddField("MaxStartDelay", KBTYPEINFO_FLOAT, kbPlaySoundComponent, m_MaxStartDelay, false, "")
	AddField("SoundData", KBTYPEINFO_STRUCT, kbPlaySoundComponent, m_SoundData, true, "kbSoundData")
)

GenerateClass(
	kbDebugSphereCollision,
	AddField("CollisionModel", KBTYPEINFO_STATICMODEL, kbDebugSphereCollision, m_pCollisionModel, false, "")
)

GenerateClass(
	kbLevelComponent,
	AddField("LevelType", KBTYPEINFO_ENUM, kbLevelComponent, m_LevelType, false, "ELevelType")
	AddField("GlobalModelScale", KBTYPEINFO_FLOAT, kbLevelComponent, m_GlobalModelScale, false, "")
	AddField("EditorIconScale", KBTYPEINFO_FLOAT, kbLevelComponent, m_EditorIconScale, false, "")
	AddField("GlobalVolumeScale", KBTYPEINFO_FLOAT, kbLevelComponent, m_GlobalVolumeScale, false, "")
)

GenerateClass(
	kbShaderModifierComponent,
	AddField("ShaderVectorEvents", KBTYPEINFO_STRUCT, kbShaderModifierComponent, m_ShaderVectorEvents, true, "kbVectorAnimEvent")
)

GenerateClass(
	kbDeleteEntityComponent,
	AddField("Dummy", KBTYPEINFO_FLOAT, kbDeleteEntityComponent, m_Dummy, false, "")
)

GenerateClass(
	kbUIComponent,
	AddField("AuthoredWidth", KBTYPEINFO_INT, kbUIComponent, m_AuthoredWidth, false, "")
	AddField("AuthoredHeight", KBTYPEINFO_INT, kbUIComponent, m_AuthoredHeight, false, "")
	AddField("NormalizedAnchorPoint", KBTYPEINFO_VECTOR, kbUIComponent, m_NormalizedAnchorPt, false, "")
	AddField("UIToScreenSizeRatio", KBTYPEINFO_VECTOR, kbUIComponent, m_UIToScreenSizeRatio, false, "")
)

GenerateEnum(
	eWidgetAnchor, "eWidgetAnchor",
	AddEnumField(TopLeft, "TopLeft")
	AddEnumField(MiddleLeft, "MiddleLeft")
	AddEnumField(BottomLeft, "BottomLeft")
	AddEnumField(TopCenter, "TopCenter")
	AddEnumField(MiddleCenter, "MiddleCenter")
	AddEnumField(BottomCenter, "BottomCenter")
	AddEnumField(TopRight, "TopRight")
	AddEnumField(MiddleRight, "MiddleRight")
	AddEnumField(BottomRight, "BottomRight")
)

GenerateEnum(
	eWidgetAxisLock, "eWidgetAxisLock",
	AddEnumField(LockAll, "LockAll")
	AddEnumField(LockXAxis, "LockXAxis")
	AddEnumField(LockYAxis, "LockYAxis")
)

GenerateClass(
	kbUIWidgetComponent,
	AddField("Anchor", KBTYPEINFO_ENUM, kbUIWidgetComponent, m_Anchor, false, "eWidgetAnchor")
	AddField("AxisLock", KBTYPEINFO_ENUM, kbUIWidgetComponent, m_AxisLock, false, "eWidgetAxisLock")
	AddField("RelativePosition", KBTYPEINFO_VECTOR, kbUIWidgetComponent, m_StartingPosition, false, "")
	AddField("RelativeSize", KBTYPEINFO_VECTOR, kbUIWidgetComponent, m_StartingSize, false, "")
	AddField("Materials", KBTYPEINFO_STRUCT, kbUIWidgetComponent, m_Materials, true, "kbMaterialComponent")
	AddField("ChildWidgets", KBTYPEINFO_STRUCT, kbUIWidgetComponent, m_ChildWidgets, true, "kbUIWidgetComponent")

)

GenerateClass(
	kbUISlider,
	AddField("SliderBoundsMin", KBTYPEINFO_VECTOR, kbUISlider, m_SliderBoundsMin, false, "")
	AddField("SliderBoundsMax", KBTYPEINFO_VECTOR, kbUISlider, m_SliderBoundsMax, false, "")
)

GenerateEnum(
	eCinematicActionType, "eCinematicActionType",
	AddEnumField(CineAction_Override, "Override")
	AddEnumField(CineAction_Animate, "Animate")
	AddEnumField(CineAction_MoveTo, "MoveTo")
)

GenerateClass(
	kbCinematicAction,
	AddField("ActionType", KBTYPEINFO_ENUM, kbCinematicAction, m_CineActionType, false, "eCinematicActionType")
	AddField("ActionStartTime", KBTYPEINFO_VECTOR4, kbCinematicAction, m_ActionStartTime, false, "")
	AddField("ActionDuration", KBTYPEINFO_VECTOR4, kbCinematicAction, m_ActionDuration, false, "")
	AddField("StringParam", KBTYPEINFO_KBSTRING, kbCinematicAction, m_sCineParam, false, "")
	AddField("FloatParam", KBTYPEINFO_FLOAT, kbCinematicAction, m_fCineParam, false, "")
	AddField("EntityParam", KBTYPEINFO_GAMEENTITY, kbCinematicAction, m_pCineParam, false, "")
	AddField("VectorParam", KBTYPEINFO_VECTOR4, kbCinematicAction, m_vCineParam, false, "")
)

GenerateClass(
	kbCinematicComponent,
	AddField("Actions", KBTYPEINFO_STRUCT, kbCinematicComponent, m_Actions, true, "kbCinematicAction")
)


GenerateEnum(
	EBreakableBehavior,
	"EBreakableBehavior",
	AddEnumField(PushFromImpactPoint, "PushFromImpactPoint")
	AddEnumField(UserVelocity, "UserVelocity")
)

GenerateClass(
	AnimationComponent,
	AddField("AnimationName", KBTYPEINFO_KBSTRING, AnimationComponent, m_animation_name, false, "")
	AddField("Animation", KBTYPEINFO_ANIMATION, AnimationComponent, m_animation, false, "")
	AddField("TimeScale", KBTYPEINFO_FLOAT, AnimationComponent, m_time_scale, false, "")
	AddField("IsLooping", KBTYPEINFO_BOOL, AnimationComponent, m_is_looping, false, "")
	AddField("AnimationEvent", KBTYPEINFO_STRUCT, AnimationComponent, m_anim_events, true, "kbAnimEvent")
)

GenerateClass(
	BreakableComponent,
	AddField("DestructibleBehavior", KBTYPEINFO_ENUM, BreakableComponent, m_destructible_type, false, "EBreakableBehavior")
	AddField("MaxLifeTime", KBTYPEINFO_FLOAT, BreakableComponent, m_life_duration, false, "")
	AddField("Gravity", KBTYPEINFO_VECTOR, BreakableComponent, m_gravity, false, "")
	AddField("MinLinearVelocity", KBTYPEINFO_VECTOR, BreakableComponent, m_min_linear_vel, false, "")
	AddField("MaxLinearVelocity", KBTYPEINFO_VECTOR, BreakableComponent, m_max_linear_vel, false, "")
	AddField("MinAngularVelocity", KBTYPEINFO_FLOAT, BreakableComponent, m_min_angular_vel, false, "")
	AddField("MaxAngularVelocity", KBTYPEINFO_FLOAT, BreakableComponent, m_max_angular_vel, false, "")
	AddField("Health", KBTYPEINFO_FLOAT, BreakableComponent, m_starting_health, false, "")
	AddField("ResetSim", KBTYPEINFO_BOOL, BreakableComponent, m_bDebugResetSim, false, "")
	AddField("DestructionFX", KBTYPEINFO_GAMEENTITY, BreakableComponent, m_complete_destruction_fx, false, "")
	AddField("DestructionFXLocalOffset", KBTYPEINFO_VECTOR, BreakableComponent, m_fx_local_offset, false, "")
)
