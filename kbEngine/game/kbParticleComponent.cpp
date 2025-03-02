/// kbParticleComponent.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "blk_containers.h"
#include "Matrix.h"
#include "kbRenderer_defs.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"
#include "kbRenderer.h"
#include "renderer.h"


KB_DEFINE_COMPONENT(kbParticleComponent)

static const uint NumParticleBufferVerts = 10000;
static const uint NumMeshVerts = 10000;
// #define DX11_PARTICLES

/// kbParticle_t::kbParticle_t
kbParticle_t::kbParticle_t() {
	m_pSrcModelEmitter = nullptr;
}

/// kbParticle_t::~kbParticle_t
kbParticle_t::~kbParticle_t() {
}

/// kbParticle_t::Shutdown
void kbParticle_t::Shutdown() {
#ifdef DX11_PARTICLES
	if (m_render_object.m_pComponent != nullptr) {
		g_pRenderer->RemoveRenderObject(m_render_object);
		g_pGame->GetParticleManager().ReturnComponentToPool(m_render_object.m_pComponent);
	}
#endif
	m_render_object.m_VertBufferIndexCount = 0;
	m_render_object.m_pComponent = nullptr;
	m_pSrcModelEmitter = nullptr;
}

/// kbParticleComponent::Initialize
void kbParticleComponent::Constructor() {
	m_MaxParticlesToEmit = -1;
	m_TotalDuration = -1.0f;
	m_StartDelay = 0.0f;

	m_MinParticleSpawnRate = 1.0f;
	m_MaxParticleSpawnRate = 2.0f;
	m_MinParticleStartVelocity.set(-2.0f, 5.0f, -2.0f);
	m_MaxParticleStartVelocity.set(2.0f, 5.0f, 2.0f);
	m_MinParticleEndVelocity.set(0.0f, 0.0f, 0.0f);
	m_MaxParticleEndVelocity.set(0.0f, 0.0f, 0.0f);
	m_MinStartRotationRate = 0;
	m_MaxStartRotationRate = 0;
	m_MinEndRotationRate = 0;
	m_MaxEndRotationRate = 0;

	m_MinStart3DRotation = Vec3::zero;
	m_MaxStart3DRotation = Vec3::zero;

	m_MinStart3DOffset = Vec3::zero;
	m_MaxStart3DOffset = Vec3::zero;

	m_MinParticleStartSize.set(3.0f, 3.0f, 3.0f);
	m_MaxParticleStartSize.set(3.0f, 3.0f, 3.0f);
	m_MinParticleEndSize.set(3.0f, 3.0f, 3.0f);
	m_MaxParticleEndSize.set(3.0f, 3.0f, 3.0f);
	m_ParticleMinDuration = 3.0f;
	m_ParticleMaxDuration = 3.0f;
	m_ParticleStartColor.set(1.0f, 1.0f, 1.0f, 1.0f);
	m_ParticleEndColor.set(1.0f, 1.0f, 1.0f, 1.0f);
	m_MinBurstCount = 0;
	m_MaxBurstCount = 0;
	m_BurstCount = 0;
	m_StartDelayRemaining = 0;
	m_NumEmittedParticles = 0;
	m_ParticleBillboardType = BT_FaceCamera;
	m_gravity.set(0.0f, 0.0f, 0.0f);
	m_render_order_bias = 0.0f;
	m_DebugPlayEntity = false;

	m_LeftOverTime = 0.0f;
	m_vertex_buffer = nullptr;
	m_index_buffer = nullptr;
	m_CurrentParticleBuffer = 255;
	m_bIsSpawning = true;

	m_bIsPooled = false;
	m_ParticleTemplate = nullptr;
}

/// kbParticleComponent::~kbParticleComponent
kbParticleComponent::~kbParticleComponent() {
	StopParticleSystem();

	// Dx12
	for (int i = 0; i < NumParticleBuffers; i++) {
		m_models[i].Release();
	}
}

/// kbParticleComponent::StopParticleSystem
void kbParticleComponent::StopParticleSystem() {

	blk::error_check(g_pRenderer->IsRenderingSynced() == true, "kbParticleComponent::StopParticleSystem() - Shutting down particle component even though rendering is not synced");

	if (IsModelEmitter()) {
		return;
	}

#ifdef DX11_PARTICLES
	g_pRenderer->RemoveParticle(m_render_object);
#endif

	// Dx12
	for (u32 i = 0; i < NumParticleBuffers; i++) {
		if (m_models[i].IsVertexBufferMapped()) {
			m_models[i].UnmapVertexBuffer(0);
		}

		if (m_models[i].IsIndexBufferMapped()) {
			m_models[i].UnmapIndexBuffer();		// todo : don't need to map/remap index buffer
		}
	}
	m_CurrentParticleBuffer = 255;
	m_Particles.clear();
	m_LeftOverTime = 0.0f;

	//m_ParticleBillboardType = BT_FaceCamera;
}

/// kbParticleComponent::update_internal
void kbParticleComponent::update_internal(const float DeltaTime) {
	Super::update_internal(DeltaTime);

	if (m_StartDelay > 0) {

		m_StartDelay -= DeltaTime;
		if (m_StartDelay < 0) {
			m_TimeAlive = 0.0f;
			if (m_MaxBurstCount > 0) {
				m_BurstCount = m_MinBurstCount;
				if (m_MaxBurstCount > m_MinBurstCount) {
					m_BurstCount += rand() % (m_MaxBurstCount - m_MinBurstCount);
				}
			}
		} else {
			return;
		}

	}

	const float eps = 0.00000001f;
	/*if ( IsModelEmitter() == false && ( m_pVertexBuffer == nullptr || m_pIndexBuffer == nullptr ) ) {
		return;
	}*/

	if (m_MaxBurstCount <= 0 && (m_MaxParticleSpawnRate <= eps || m_MinParticleSpawnRate < eps || m_MaxParticleSpawnRate < m_MinParticleSpawnRate || m_ParticleMinDuration <= eps)) {
		return;
	}

	Vec3 currentCameraPosition;
	Quat4 currentCameraRotation;
	g_pRenderer->GetRenderViewTransform(nullptr, currentCameraPosition, currentCameraRotation);

	int iVertex = 0;
	int curVBPosition = 0;
	const Vec3 scale = GetScale();
	const Vec3 direction = GetOrientation().to_mat4()[2].ToVec3();
	byte iBillboardType = 0;
	switch (m_ParticleBillboardType) {
		case EBillboardType::BT_FaceCamera: iBillboardType = 0; break;
		case EBillboardType::BT_AxialBillboard: iBillboardType = 1; break;
		case EBillboardType::BT_AlignAlongVelocity: iBillboardType = 1; break;
		default: blk::warn("kbParticleComponent::update_internal() - Invalid billboard type specified"); break;
	}

	kbParticleVertex* pDstVerts = nullptr;

	for (int i = (int)m_Particles.size() - 1; i >= 0; i--) {
		kbParticle_t& particle = m_Particles[i];

		if (particle.m_LifeLeft >= 0.0f) {
			particle.m_LifeLeft -= DeltaTime;

			if (particle.m_LifeLeft <= 0.0f || (IsModelEmitter() && particle.m_render_object.m_pComponent == nullptr)) {
				particle.Shutdown();
				blk::std_remove_idx_swap(m_Particles, i);
				continue;
			}
		}
	}

	m_render_object.m_VertBufferIndexCount = (uint)m_Particles.size() * 6;
	if (IsModelEmitter() == false && m_Particles.size() > 0) {
		kbParticleManager& particleMgr = g_pGame->GetParticleManager();
		particleMgr.ReserveScratchBufferSpace(pDstVerts, m_render_object, (int)m_Particles.size() * 4);
		blk::error_check(pDstVerts != nullptr, "kbParticleComponent::update_internal() - pDstVerts is null");

		for (int i = 0; i < m_Particles.size() * 4; i++) {
			pDstVerts[i].position = Vec3::zero;
		}
	}

	for (int i = (int)m_Particles.size() - 1; i >= 0; i--) {
		kbParticle_t& particle = m_Particles[i];
		const float normalizedTime = (particle.m_TotalLife - particle.m_LifeLeft) / particle.m_TotalLife;
		Vec3 curVelocity = Vec3::zero;

		if (m_velocityOverLifeTimeCurve.size() == 0) {
			curVelocity = kbLerp(particle.m_StartVelocity, particle.m_EndVelocity, normalizedTime);
		} else {
			const float velCurve = kbAnimEvent::Evaluate(m_velocityOverLifeTimeCurve, normalizedTime);
			curVelocity = particle.m_StartVelocity * velCurve;
		}

		curVelocity += m_gravity * (particle.m_TotalLife - particle.m_LifeLeft);

		particle.m_position = particle.m_position + curVelocity * DeltaTime;

		const float curRotationRate = kbLerp(particle.m_StartRotation, particle.m_EndRotation, normalizedTime);
		particle.m_Rotation += curRotationRate * DeltaTime;

		Vec3 curSize = Vec3::zero;
		if (m_SizeOverLifeTimeCurve.size() == 0) {
			curSize = kbLerp(particle.m_StartSize * scale.x, particle.m_EndSize * scale.y, normalizedTime);
		} else {
			Vec3 eval = kbVectorAnimEvent::Evaluate(m_SizeOverLifeTimeCurve, normalizedTime).ToVec3();
			curSize.x = eval.x * particle.m_StartSize.x * scale.x;
			curSize.y = eval.y * particle.m_StartSize.y * scale.y;
			curSize.z = eval.z * particle.m_StartSize.z * scale.z;
		}

		Vec4 curColor = Vec4::zero;
		if (m_ColorOverLifeTimeCurve.size() == 0) {
			curColor = kbLerp(m_ParticleStartColor, m_ParticleEndColor, normalizedTime);
		} else {
			curColor = kbVectorAnimEvent::Evaluate(m_ColorOverLifeTimeCurve, normalizedTime);
		}

		if (m_AlphaOverLifeTimeCurve.size() == 0) {
			curColor.w = kbLerp(m_ParticleStartColor.w, m_ParticleEndColor.w, normalizedTime);
		} else {
			curColor.w = kbAnimEvent::Evaluate(m_AlphaOverLifeTimeCurve, normalizedTime);
		}

		byte byteColor[4] = { (byte)kbClamp(curColor.x * 255.0f, 0.0f, 255.0f), (byte)kbClamp(curColor.y * 255.0f, 0.0f, 255.0f), (byte)kbClamp(curColor.z * 255.0f, 0.0f, 255.0f), (byte)kbClamp(curColor.w * 255.0f, 0.0f, 255.0f) };

		if (IsModelEmitter()) {

			kbRenderObject& renderObj = particle.m_render_object;

			renderObj.m_position = particle.m_position;
			//renderObj.m_Orientation = Quat4( 0.0f, 0.0f, 0.0f, 1.0f );	TODO
			renderObj.m_Scale.set(curSize.x, curSize.y, curSize.z);
			renderObj.m_Scale *= kbLevelComponent::GetGlobalModelScale();

			if (m_RotationOverLifeTimeCurve.size() > 0) {
				const Vec4 rotationFactor = kbVectorAnimEvent::Evaluate(m_RotationOverLifeTimeCurve, normalizedTime);
				Quat4 xAxis, yAxis, zAxis;
				xAxis.from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), kbToRadians(particle.m_rotation_axis.x * rotationFactor.x));
				yAxis.from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kbToRadians(particle.m_rotation_axis.y * rotationFactor.y));
				zAxis.from_axis_angle(Vec3(0.0f, 0.0f, 1.0f), kbToRadians(particle.m_rotation_axis.z * rotationFactor.z));
				renderObj.m_Orientation = xAxis * yAxis * zAxis;
			}

			for (int iMat = 0; iMat < renderObj.m_Materials.size(); iMat++) {
				renderObj.m_Materials[iMat].SetVec4("particleColor", curColor);
			}

#ifdef DX11_PARTICLES
			g_pRenderer->UpdateRenderObject(renderObj);
#endif

			continue;
		}

		pDstVerts[iVertex + 0].position = particle.m_position;
		pDstVerts[iVertex + 1].position = particle.m_position;
		pDstVerts[iVertex + 2].position = particle.m_position;
		pDstVerts[iVertex + 3].position = particle.m_position;

		pDstVerts[iVertex + 0].uv.set(0.0f, 0.0f);
		pDstVerts[iVertex + 1].uv.set(1.0f, 0.0f);
		pDstVerts[iVertex + 2].uv.set(1.0f, 1.0f);
		pDstVerts[iVertex + 3].uv.set(0.0f, 1.0f);


		pDstVerts[iVertex + 0].size = Vec2(-curSize.x, curSize.y);
		pDstVerts[iVertex + 1].size = Vec2(curSize.x, curSize.y);
		pDstVerts[iVertex + 2].size = Vec2(curSize.x, -curSize.y);
		pDstVerts[iVertex + 3].size = Vec2(-curSize.x, -curSize.y);

		memcpy(&pDstVerts[iVertex + 0].color, byteColor, sizeof(byteColor));
		memcpy(&pDstVerts[iVertex + 1].color, byteColor, sizeof(byteColor));
		memcpy(&pDstVerts[iVertex + 2].color, byteColor, sizeof(byteColor));
		memcpy(&pDstVerts[iVertex + 3].color, byteColor, sizeof(byteColor));

		if (m_ParticleBillboardType == EBillboardType::BT_AlignAlongVelocity) {
			Vec3 alignVec = Vec3::up;
			if (curVelocity.length_sqr() > 0.01f) {
				alignVec = curVelocity.normalize_safe();
				pDstVerts[iVertex + 0].direction = alignVec;
				pDstVerts[iVertex + 1].direction = alignVec;
				pDstVerts[iVertex + 2].direction = alignVec;
				pDstVerts[iVertex + 3].direction = alignVec;
			}
		} else {
			pDstVerts[iVertex + 0].direction = direction;
			pDstVerts[iVertex + 1].direction = direction;
			pDstVerts[iVertex + 2].direction = direction;
			pDstVerts[iVertex + 3].direction = direction;
		}

		pDstVerts[iVertex + 0].rotation = particle.m_Rotation;
		pDstVerts[iVertex + 1].rotation = particle.m_Rotation;
		pDstVerts[iVertex + 2].rotation = particle.m_Rotation;
		pDstVerts[iVertex + 3].rotation = particle.m_Rotation;

		pDstVerts[iVertex + 0].billboardType[0] = iBillboardType;
		pDstVerts[iVertex + 1].billboardType[0] = iBillboardType;
		pDstVerts[iVertex + 2].billboardType[0] = iBillboardType;
		pDstVerts[iVertex + 3].billboardType[0] = iBillboardType;

		pDstVerts[iVertex + 0].billboardType[1] = (byte)kbClamp(particle.m_Randoms[0] * 255.0f, 0.0f, 255.0f);
		pDstVerts[iVertex + 1].billboardType[1] = pDstVerts[iVertex + 0].billboardType[1];
		pDstVerts[iVertex + 2].billboardType[1] = pDstVerts[iVertex + 0].billboardType[1];
		pDstVerts[iVertex + 3].billboardType[1] = pDstVerts[iVertex + 0].billboardType[1];

		pDstVerts[iVertex + 0].billboardType[2] = (byte)kbClamp(particle.m_Randoms[1] * 255.0f, 0.0f, 255.0f);
		pDstVerts[iVertex + 1].billboardType[2] = pDstVerts[iVertex + 0].billboardType[2];
		pDstVerts[iVertex + 2].billboardType[2] = pDstVerts[iVertex + 0].billboardType[2];
		pDstVerts[iVertex + 3].billboardType[2] = pDstVerts[iVertex + 0].billboardType[2];

		pDstVerts[iVertex + 0].billboardType[3] = (byte)kbClamp(particle.m_Randoms[2] * 255.0f, 0.0f, 255.0f);
		pDstVerts[iVertex + 1].billboardType[3] = pDstVerts[iVertex + 0].billboardType[3];
		pDstVerts[iVertex + 2].billboardType[3] = pDstVerts[iVertex + 0].billboardType[3];
		pDstVerts[iVertex + 3].billboardType[3] = pDstVerts[iVertex + 0].billboardType[3];
		iVertex += 4;
		curVBPosition++;
	}

	m_TimeAlive += DeltaTime;
	if (m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_BurstCount <= 0) {
		return;
	}

	const float invMinSpawnRate = (m_MinParticleSpawnRate > 0.0f) ? (1.0f / m_MinParticleSpawnRate) : (0.0f);
	const float invMaxSpawnRate = (m_MaxParticleSpawnRate > 0.0f) ? (1.0f / m_MaxParticleSpawnRate) : (0.0f);
	float TimeLeft = DeltaTime - m_LeftOverTime;
	int currentListEnd = (int)m_Particles.size();
	float NextSpawn = 0.0f;

	Mat4 ownerMatrix = GetOwner()->GetOrientation().to_mat4();

	// Spawn particles
	Vec3 MyPosition = GetPosition();
	while (m_bIsSpawning && ((m_MaxParticleSpawnRate > 0 && TimeLeft >= NextSpawn) || m_BurstCount > 0) && (m_MaxParticlesToEmit <= 0 || m_NumEmittedParticles < m_MaxParticlesToEmit)) {
		if (m_MinStart3DOffset.compare(Vec3::zero) == false || m_MaxStart3DOffset.compare(Vec3::zero) == false) {
			const Vec3 startingOffset = Vec3Rand(m_MinStart3DOffset, m_MaxStart3DOffset);
			MyPosition += startingOffset;
		}

		kbParticle_t newParticle;
		newParticle.m_StartVelocity = Vec3Rand(m_MinParticleStartVelocity, m_MaxParticleStartVelocity) * ownerMatrix;
		newParticle.m_EndVelocity = Vec3Rand(m_MinParticleEndVelocity, m_MaxParticleEndVelocity) * ownerMatrix;

		newParticle.m_position = MyPosition + newParticle.m_StartVelocity * TimeLeft;
		newParticle.m_LifeLeft = m_ParticleMinDuration + (kbfrand() * (m_ParticleMaxDuration - m_ParticleMinDuration));
		newParticle.m_TotalLife = newParticle.m_LifeLeft;

		const float startSizeRand = kbfrand();
		newParticle.m_StartSize.x = m_MinParticleStartSize.x + (startSizeRand * (m_MaxParticleStartSize.x - m_MinParticleStartSize.x));
		newParticle.m_StartSize.y = m_MinParticleStartSize.y + (startSizeRand * (m_MaxParticleStartSize.y - m_MinParticleStartSize.y));
		newParticle.m_StartSize.z = m_MinParticleStartSize.z + (startSizeRand * (m_MaxParticleStartSize.z - m_MinParticleStartSize.z));

		const float endSizeRand = kbfrand();
		newParticle.m_EndSize.x = m_MinParticleEndSize.x + (endSizeRand * (m_MaxParticleEndSize.x - m_MinParticleEndSize.x));
		newParticle.m_EndSize.y = m_MinParticleEndSize.y + (endSizeRand * (m_MaxParticleEndSize.y - m_MinParticleEndSize.y));
		newParticle.m_EndSize.z = m_MinParticleEndSize.z + (endSizeRand * (m_MaxParticleEndSize.z - m_MinParticleEndSize.z));

		if (IsModelEmitter()) {
			blk::log("StartSize = %f %f %f", newParticle.m_StartSize.x, newParticle.m_StartSize.y, newParticle.m_StartSize.z);
			blk::log("End = %f %f %f", newParticle.m_EndSize.x, newParticle.m_EndSize.y, newParticle.m_EndSize.z);
		}

		newParticle.m_Randoms[0] = kbfrand();
		newParticle.m_Randoms[1] = kbfrand();
		newParticle.m_Randoms[2] = kbfrand();

		newParticle.m_StartRotation = kbfrand(m_MinStartRotationRate, m_MaxStartRotationRate);
		newParticle.m_EndRotation = kbfrand(m_MinEndRotationRate, m_MaxEndRotationRate);

		if (IsModelEmitter() && m_ModelEmitter.size()) {
			const kbGameComponent* const pComponent = g_pGame->GetParticleManager().GetComponentFromPool();
			if (pComponent != nullptr) {
				const int randIdx = rand() % m_ModelEmitter.size();
				kbModelEmitter* const pModelEmitter = &m_ModelEmitter[randIdx];
				newParticle.m_pSrcModelEmitter = &m_ModelEmitter[randIdx];

				kbRenderObject& renderObj = newParticle.m_render_object;
				renderObj.m_pComponent = pComponent;

				renderObj.m_model = pModelEmitter->model();
				renderObj.m_Materials = pModelEmitter->GetShaderParamOverrides();
				renderObj.m_render_pass = RP_Translucent;
				renderObj.m_render_order_bias = 0;
				renderObj.m_position = newParticle.m_position;

				renderObj.m_Scale = Vec3::one;
				renderObj.m_EntityId = 0;
				renderObj.m_CullDistance = -1;
				renderObj.m_casts_shadow = false;
				renderObj.m_bIsSkinnedModel = false;
				renderObj.m_bIsFirstAdd = true;
				renderObj.m_bIsRemove = false;

				renderObj.m_Orientation = Quat4(0.0f, 0.0f, 0.0f, 1.0f);
				if (m_MinStart3DRotation.compare(Vec3::zero) == false || m_MaxStart3DRotation.compare(Vec3::zero) == false) {
					newParticle.m_rotation_axis = Vec3Rand(m_MinStart3DRotation, m_MaxStart3DRotation);
					Quat4 xAxis, yAxis, zAxis;
					xAxis.from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), kbToRadians(newParticle.m_rotation_axis.x));
					yAxis.from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kbToRadians(newParticle.m_rotation_axis.y));
					zAxis.from_axis_angle(Vec3(0.0f, 0.0f, 1.0f), kbToRadians(newParticle.m_rotation_axis.z));

					renderObj.m_Orientation = xAxis * yAxis * zAxis;
				}

#ifdef DX11_PARTICLES
				g_pRenderer->AddRenderObject(renderObj);
#endif
			} else {
				continue;
			}
		}

		if (newParticle.m_StartRotation != 0 || newParticle.m_EndRotation != 0) {
			newParticle.m_Rotation = kbfrand() * kbPI;
		} else {
			newParticle.m_Rotation = 0;
		}

		if (m_BurstCount > 0) {
			m_BurstCount--;
		} else {
			TimeLeft -= NextSpawn;
			NextSpawn = invMaxSpawnRate + (kbfrand() * (invMinSpawnRate - invMaxSpawnRate));
		}

		m_NumEmittedParticles++;
		m_Particles.push_back(newParticle);
	}


	//blk::log( "Num Indices = %d", m_NumIndicesInCurrentBuffer );
	m_LeftOverTime = NextSpawn - TimeLeft;
}

/// kbParticleComponent::EditorChange
void kbParticleComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	// Editor Hack!
	if (propertyName == "Materials") {
		for (int i = 0; i < this->m_materials.size(); i++) {
			m_materials[i].SetOwningComponent(this);
		}
	} else if (propertyName == "DebugPlayEntity") {
		kbGameEntity* const pEnt = GetOwner();
		const int numComp = (int)pEnt->NumComponents();
		for (int i = 0; i < numComp; i++) {
			kbParticleComponent* const pParticle = pEnt->GetComponent(i)->GetAs<kbParticleComponent>();
			if (pParticle == nullptr) {
				continue;
			}

			pParticle->Enable(!pParticle->IsEnabled());
		}
	}
}

/// kbParticleComponent::RenderSync
void kbParticleComponent::RenderSync() {
	Super::RenderSync();

	if (g_UseEditor && IsEnabled() == true && (m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_BurstCount <= 0)) {
		StopParticleSystem();
		Enable(false);
		Enable(true);
		return;
	}

	if (IsEnabled() == false || (m_TotalDuration > 0.0f && m_TimeAlive > m_TotalDuration && m_render_object.m_VertBufferIndexCount == 0)) {
		StopParticleSystem();
		Enable(false);
		if (m_bIsPooled) {
			g_pGame->GetParticleManager().ReturnParticleComponent(this);
		}
		return;
	}

	if (IsModelEmitter()) {
		return;
	}
	
	if (g_renderer != nullptr) {
		if (m_models[0].NumVertices() == 0) {
			for (int i = 0; i < NumParticleBuffers; i++) {
				m_models[i].create_dynamic(NumParticleBufferVerts, NumParticleBufferVerts);
				m_vertex_buffer = (ParticleVertex*)m_models[i].map_vertex_buffer();
				for (int iVert = 0; iVert < NumParticleBufferVerts; iVert++) {
					m_vertex_buffer[iVert].position.set(0.0f, 0.0f, 0.0f);
				}
				m_models[i].UnmapVertexBuffer();
			}
		}
	}

	m_render_object.m_pComponent = this;
	m_render_object.m_render_pass = RP_Translucent;
	m_render_object.m_position = GetPosition();
	m_render_object.m_Orientation = Quat4(0.0f, 0.0f, 0.0f, 1.0f);
	m_render_object.m_render_order_bias = m_render_order_bias;

	// Update materials
	m_render_object.m_Materials.clear();
	for (int i = 0; i < m_materials.size(); i++) {
		kbMaterialComponent& matComp = m_materials[i];

		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_shader = matComp.get_shader();

		const auto& srcShaderParams = matComp.shader_params();
		for (int j = 0; j < srcShaderParams.size(); j++) {
			if (srcShaderParams[j].texture() != nullptr) {
				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].texture());
			} else if (srcShaderParams[j].render_texture() != nullptr) {
				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].render_texture());
			} else {
				newShaderParams.SetVec4(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].vector());
			}
		}

		m_render_object.m_Materials.push_back(newShaderParams);
	}

	if (m_CurrentParticleBuffer == 255) {
		m_CurrentParticleBuffer = 0;
	} else {
#ifdef DX11_PARTICLES
		g_pRenderer->RemoveParticle(m_render_object);
#endif
	}

	//	m_render_object.m_model = &m_ParticleBuffer[m_CurrentParticleBuffer];
	if (m_render_object.m_VertBufferIndexCount > 0) {
		g_pRenderer->AddParticle(m_render_object);
	}

	m_CurrentParticleBuffer++;
	if (m_CurrentParticleBuffer >= NumParticleBuffers) {
		m_CurrentParticleBuffer = 0;
	}
	
	if (g_renderer != nullptr) {
		m_vertex_buffer = (ParticleVertex*)m_models[m_CurrentParticleBuffer].map_vertex_buffer();
		m_index_buffer = (u16*)m_models[m_CurrentParticleBuffer].map_index_buffer();
	}
}

/// kbParticleComponent::enable_internal
void kbParticleComponent::enable_internal(const bool isEnabled) {
	Super::enable_internal(isEnabled);

	if (isEnabled) {
		m_bIsSpawning = true;
		m_NumEmittedParticles = 0;

		if (m_StartDelay > 0) {
			m_StartDelayRemaining = m_StartDelay;
		} else {
			m_TimeAlive = 0.0f;
			if (m_MaxBurstCount > 0) {
				m_BurstCount = m_MinBurstCount;
				if (m_MaxBurstCount > m_MinBurstCount) {
					m_BurstCount += rand() % (m_MaxBurstCount - m_MinBurstCount);
				}
			}
		}

		for (int i = 0; i < m_ModelEmitter.size(); i++) {
			m_ModelEmitter[i].Init();
		}

	} else {
#ifdef DX11_PARTICLES
		g_pRenderer->RemoveParticle(m_render_object);
#endif

		m_Particles.clear();
		m_LeftOverTime = 0.0f;
	}
}

/// kbParticleComponent::EnableNewSpawns
void kbParticleComponent::EnableNewSpawns(const bool bEnable) {
	if (m_bIsSpawning == bEnable) {
		return;
	}

	m_bIsSpawning = bEnable;
	m_LeftOverTime = 0.0f;
}

/// kbParticleComponent::Constructor
void kbModelEmitter::Constructor() {
	m_model = nullptr;

}

/// kbModelEmitter::Init
void kbModelEmitter::Init() {

	for (int i = 0; i < m_materials.size(); i++) {
		const kbMaterialComponent& matComp = m_materials[i];

		kbShaderParamOverrides_t newShaderParams;
		newShaderParams.m_shader = matComp.get_shader();

		const auto& srcShaderParams = matComp.shader_params();
		for (int j = 0; j < srcShaderParams.size(); j++) {
			if (srcShaderParams[j].texture() != nullptr) {
				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].texture());
			} else if (srcShaderParams[j].render_texture() != nullptr) {

				newShaderParams.SetTexture(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].render_texture());
			} else {
				newShaderParams.SetVec4(srcShaderParams[j].param_name().stl_str(), srcShaderParams[j].vector());
			}
		}

		m_ShaderParams.push_back(newShaderParams);
	}
}
