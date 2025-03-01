/// kbClothComponent.cpp
///
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbClothComponent.h"
#include "blk_console.h"
#include "DX11/kbRenderer_DX11.h"			// HACK

KB_DEFINE_COMPONENT(kbClothBone)
KB_DEFINE_COMPONENT(kbClothComponent)

kbConsoleVariable g_DebugCloth("debugcloth", false, kbConsoleVariable::Console_Int, "Draw cloth debugging info.  Takes values 1-13", "");
kbConsoleVariable g_ClothGrav("clothgravity", 0.0f, kbConsoleVariable::Console_Float, "Cloth gravity", "");
kbConsoleVariable g_ClothSpring("clothspring", 0.5f, kbConsoleVariable::Console_Float, "Cloth spring", "");
kbConsoleVariable g_ClothFriction("clothFriction", 0.02f, kbConsoleVariable::Console_Float, "Cloth friction", "");

/// kbClothBone::Constructor
void kbClothBone::Constructor() {
	m_bIsAnchored = false;
}

/// kbBoneCollisionSphere::Constructor
void kbBoneCollisionSphere::Constructor() {
	m_Sphere.set(0.0f, 0.0f, 0.0f, 10.0f);
}

/// kbClothComponent::~kbClothComponent
kbClothComponent::~kbClothComponent() {
}

/// kbClothComponent::Constructor
void kbClothComponent::Constructor() {
	m_ClothType = CT_None;
	m_Width = 0;
	m_Height = 0;
	m_pSkeletalModel = nullptr;
	m_NumConstrainIterations = 1;

	m_gravity.set(0.0f, -100.0f, 0.0f);
	m_MaxWindVelocity.set(20.0f, 160.0f, -9.0f);
	m_MinWindVelocity.set(64.0f, 30.0f, -20.0f);
	m_MinWindGustDuration = 0.2f;
	m_MaxWindGustDuration = 0.35f;
	m_bAddFakeOscillation = false;

	m_CurWindVelocity = Vec3::zero;
	m_NextWindVelocity = Vec3::zero;
	m_NextWindChangeTime = 0;

	m_CurrentTickFrame = 0;
}

/// kbClothComponent::update_internal
void kbClothComponent::update_internal(const float dt) {
	Super::update_internal(dt);

	float DeltaTime = dt;
	if (DeltaTime == 0.0f) {
		DeltaTime = 0.016f;
	}
	// Dont start sim for a few frames in case the entity is teleported
	m_CurrentTickFrame++;
	if (m_CurrentTickFrame < 5) {
		return;
	}

	SkeletalModelComponent* pSkelRenderComponent = nullptr;
	for (int i = 0; i < GetOwner()->NumComponents(); i++) {
		if (GetOwner()->GetComponent(i)->IsA(SkeletalModelComponent::GetType())) {
			pSkelRenderComponent = static_cast<SkeletalModelComponent*>(GetOwner()->GetComponent(i));
			if (pSkelRenderComponent->model() != m_pSkeletalModel) {
				m_pSkeletalModel = pSkelRenderComponent->model();
				SetupCloth();
				break;
			}
		}
	}

	if (pSkelRenderComponent == nullptr || m_pSkeletalModel == nullptr) {
		return;
	}

	RunSimulation(DeltaTime);

	Mat4 WorldMat;
	GetOwner()->CalculateWorldMatrix(WorldMat);

	// todo: Need my own inverse matrix function
	Mat4 invParentMatrix;
	XMMATRIX inverseMat = XMMatrixInverse(nullptr, XMMATRIXFromMat4(WorldMat));
	invParentMatrix = Mat4FromXMMATRIX(inverseMat);

	std::vector<kbBoneMatrix_t>& FinalBoneMatrices = pSkelRenderComponent->GetFinalBoneMatrices();
	if (FinalBoneMatrices.size() == 0) {
		return;
	}

	static int clothDebug = 0;

	/*if ( GetAsyncKeyState( 'I') ) {
		clothDebug = 0;
	} else if ( GetAsyncKeyState( 'O') ) {
		clothDebug = 1;
	} else if ( GetAsyncKeyState( 'P') ) {
		clothDebug = 2;
	}*/

	for (int i = 0; i < m_Masses.size(); i++) {

		const int BoneIndex = m_BoneIndices[i];

		if (m_Masses[i].m_bAnchored) {
			const kbBoneMatrix_t refBoneMatrix = pSkelRenderComponent->GetBoneRefMatrix(BoneIndex);
			const Vec3 localBonePos = refBoneMatrix.GetAxis(3);
			Vec3 finalPosition = WorldMat.transform_point(localBonePos * FinalBoneMatrices[BoneIndex]);
			m_Masses[i].SetPosition(finalPosition);
		}


		const Vec3 worldPos = m_Masses[i].GetPosition();
		kbBoneMatrix_t WorldToLocalSpace;
		WorldToLocalSpace.SetAxis(0, m_Masses[i].GetAxis(0));
		WorldToLocalSpace.SetAxis(1, m_Masses[i].GetAxis(1));
		WorldToLocalSpace.SetAxis(2, m_Masses[i].GetAxis(2));
		WorldToLocalSpace.SetAxis(3, worldPos);
		WorldToLocalSpace *= invParentMatrix;

		kbBoneMatrix_t LocalToRef = pSkelRenderComponent->GetBoneRefMatrix(BoneIndex);
		LocalToRef.Invert();
		FinalBoneMatrices[BoneIndex] = LocalToRef * WorldToLocalSpace;

		if (clothDebug == 1 || g_DebugCloth.GetInt() == 1) {
			kbBounds bounds(true);
			bounds.AddPoint(worldPos);
			bounds.AddPoint(worldPos + Vec3(0.1f, 0.1f, 0.1f));
			bounds.AddPoint(worldPos - Vec3(0.1f, 0.1f, 0.1f));

			if (m_Masses[i].m_bAnchored) {
				g_pRenderer->DrawBox(bounds, kbColor(0.0f, 1.0f, 0.0f, 1.0f));
			} else {
				g_pRenderer->DrawBox(bounds, kbColor(1.0f, 1.0f, 1.0f, 1.0f));
			}

			const float AxisLen = 1.0f;
			g_pRenderer->DrawLine(worldPos, worldPos + m_Masses[i].GetAxis(0) * AxisLen, kbColor::red);
			g_pRenderer->DrawLine(worldPos, worldPos + m_Masses[i].GetAxis(1) * AxisLen, kbColor::green);
			g_pRenderer->DrawLine(worldPos, worldPos + m_Masses[i].GetAxis(2) * -AxisLen, kbColor::blue);
		}
	}


	if (clothDebug == 2 || g_DebugCloth.GetInt() == 2) {
		for (int i = 0; i < m_Springs.size(); i++) {
			const kbClothSpring_t& curSpring = m_Springs[i];
			const kbClothMass_t& Mass1 = m_Masses[curSpring.m_MassIndices[0]];
			const kbClothMass_t& Mass2 = m_Masses[curSpring.m_MassIndices[1]];
			g_pRenderer->DrawLine(Mass1.GetPosition(), Mass2.GetPosition(), kbColor::white);
		}

		for (int iCollision = 0; iCollision < m_CollisionSpheres.size(); iCollision++) {
			kbBoneMatrix_t boneWorldMatrix;

			if (GetAsyncKeyState('I')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.02f, 0.0f, 0.0f);
			}
			if (GetAsyncKeyState('O')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(-0.02f, 0.0f, 0.0f);
			}

			if (GetAsyncKeyState('K')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.f, 0.02f, 0.0f);
			}
			if (GetAsyncKeyState('L')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.0f, -0.02f, 0.0f);
			}

			if (GetAsyncKeyState('N')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.0f, 0.0f, 0.02f);
			}
			if (GetAsyncKeyState('M')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.0f, 0.0f, -0.02f);
			}

			if (GetAsyncKeyState(',')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.0f, 0.0f, 0.02f);
			}
			if (GetAsyncKeyState('.')) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += Vec3(0.0f, 0.0f, -0.02f);
			}

			if (GetAsyncKeyState('U')) {
				blk::log("%f %f %f", m_CollisionSpheres[iCollision].m_Sphere.x, m_CollisionSpheres[iCollision].m_Sphere.y, m_CollisionSpheres[iCollision].m_Sphere.z);
			}

			//		blk::log( "%s looking for %s", pSkelRenderComponent->model()->GetFullFileName().c_str(), m_CollisionSpheres[iCollision].m_BoneName.c_str() );

			if (pSkelRenderComponent->GetBoneWorldMatrix(m_CollisionSpheres[iCollision].m_BoneName, boneWorldMatrix)) {
				boneWorldMatrix.m_Axis[0].normalize_self();
				boneWorldMatrix.m_Axis[1].normalize_self();
				boneWorldMatrix.m_Axis[2].normalize_self();

				Vec3 spherePos = m_CollisionSpheres[iCollision].m_Sphere.ToVec3() * boneWorldMatrix;
				g_pRenderer->DrawSphere(spherePos, m_CollisionSpheres[iCollision].m_Sphere.w, 12, kbColor(1.0f, 0.0f, 1.0f, 1.0f));
			}
		}
	}
}

/// kbClothComponent::RunSimulation
void kbClothComponent::RunSimulation(const float inDeltaTime) {

	const float DeltaTime = kbClamp(inDeltaTime, 0.0f, 0.016f);

	if (m_bAddFakeOscillation) {

		if (g_GlobalTimer.TimeElapsedSeconds() >= m_NextWindChangeTime) {
			m_NextWindChangeTime = g_GlobalTimer.TimeElapsedSeconds() + (kbfrand() * (m_MaxWindGustDuration - m_MinWindGustDuration)) + m_MinWindGustDuration;
			m_NextWindVelocity = Vec3Rand(m_MinWindVelocity, m_MaxWindVelocity);
		}

		m_CurWindVelocity = kbLerp(m_CurWindVelocity, m_NextWindVelocity, 0.0075f);
	}


	// Wind Sim

	Vec3 wind = m_CurWindVelocity;


	static Vec3 BallPos(0.0f, -35.0f, 0.0f);
	static Vec3 BallDir(0.0f, 0.0f, 1.0f);
	static float BallRad = 65;

	// Ball Sim
	/*BallPos = BallPos + BallDir * DeltaTime * 124.0f;
	if ( BallDir.z > 0 && BallPos.z > 200.0f )
		BallDir.z = -1.0f;
	else if ( BallDir.z < 0 && BallPos.z < -200.0f )
		BallDir.z = 1.0f;

	kbBounds bounds( true );
	bounds.AddPoint( BallPos ),
	bounds.AddPoint( BallPos + Vec3( 1.0f, 1.0f, 1.0f ).normalize_safe() * BallRad );
	bounds.AddPoint( BallPos - ( Vec3( 1.0f, 1.0f, 1.0f ).normalize_safe() * BallRad ) );

	std::vector<Vec4> CollisionSpheres;
	Vec4 newSphere( BallPos.x, BallPos.y, BallPos.z, 1.0f );
	newSphere.w = BallRad;
	CollisionSpheres.push_back( newSphere );

	g_pRenderer->DrawSphere( BallPos, BallRad, 16, kbColor::yellow);*/
	// Ball Sim - end

	SkeletalModelComponent* pSkelRenderComponent = NULL;
	for (int i = 0; i < GetOwner()->NumComponents(); i++) {
		if (GetOwner()->GetComponent(i)->IsA(SkeletalModelComponent::GetType())) {
			pSkelRenderComponent = static_cast<SkeletalModelComponent*>(GetOwner()->GetComponent(i));
			if (pSkelRenderComponent->model() != m_pSkeletalModel) {
				break;
			}
		}
	}

	std::vector<Vec4> CollisionSpheres;
	for (int iCollision = 0; iCollision < m_CollisionSpheres.size(); iCollision++) {
		kbBoneMatrix_t boneWorldMatrix;
		if (pSkelRenderComponent->GetBoneWorldMatrix(m_CollisionSpheres[iCollision].m_BoneName, boneWorldMatrix)) {
			boneWorldMatrix.m_Axis[0].normalize_self();
			boneWorldMatrix.m_Axis[1].normalize_self();
			boneWorldMatrix.m_Axis[2].normalize_self();

			const Vec3 spherePos = m_CollisionSpheres[iCollision].m_Sphere.ToVec3() * boneWorldMatrix;
			CollisionSpheres.push_back(Vec4(spherePos, m_CollisionSpheres[iCollision].m_Sphere.w));
		}
	}

	// Apply forces and update positions
	std::vector<float> a;
	for (int i = 0; i < m_Width; i++) {
		float theRand = kbfrand();

		a.push_back(theRand);
		a.push_back(theRand);
		a.push_back(theRand);

	}

	float theRand = 0.0f;
	for (int massIdx = 0; massIdx < m_Masses.size(); massIdx++) {
		if (m_Masses[massIdx].m_bAnchored) {
			m_Masses[massIdx].m_FrameForces = Vec3::zero;
			continue;
		}

		Vec3 totalForce = m_gravity + Vec3(0.0f, g_ClothGrav.GetFloat(), 0.0f);
		if (m_bAddFakeOscillation) {

			Vec3 windAmt = ((wind - (wind * 0.5f) * kbfrand() + (wind * 0.5f)));
			int level = massIdx / m_Height;

			if (massIdx % 1 == 0) {
				theRand = kbfrand();
			}
			if (a[massIdx / m_Width] < 0.45f) {
				windAmt *= 1.3f + kbfrand() * 1.35f;
			}
			totalForce += windAmt;
		}
		totalForce += m_Masses[massIdx].m_FrameForces;
		Vec3 newLocation = m_Masses[massIdx].GetPosition();// + ( totalForce * DeltaTime );

		// ...
		/*for ( int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++ ) {
			const Vec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
			if ( sphereToVert.length_sqr() < ( CollisionSpheres[sphereIdx].w * CollisionSpheres[sphereIdx].w) ) {
				newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.normalize_safe() * CollisionSpheres[sphereIdx].w;
			}
		}*/
		// ...

		Vec3 velocity = m_Masses[massIdx].GetPosition() - m_Masses[massIdx].m_LastPosition;
		m_Masses[massIdx].m_LastPosition = m_Masses[massIdx].GetPosition();

		newLocation += velocity * (1.0f - g_ClothFriction.GetFloat()) + totalForce * (DeltaTime * DeltaTime);
		m_Masses[massIdx].SetPosition(newLocation);
		m_Masses[massIdx].m_FrameForces = Vec3::zero;
	}

	for (int iIteration = 0; iIteration < m_NumConstrainIterations; iIteration++) {

		// Apply constraints
		for (int iSpring = 0; iSpring < m_Springs.size(); iSpring++) {
			const int mass1Idx = m_Springs[iSpring].m_MassIndices[0];
			const int mass2Idx = m_Springs[iSpring].m_MassIndices[1];

			const Vec3 mass1Pos = m_Masses[mass1Idx].GetPosition();
			const Vec3 mass2Pos = m_Masses[mass2Idx].GetPosition();

			const Vec3 mass1ToMass2 = mass2Pos - mass1Pos;
			const float distanceBetween = mass1ToMass2.length();
			const float invDist = (distanceBetween > 0.001f) ? (1.0f / distanceBetween) : (0.0f);
			Vec3 finalOffset = ((mass1ToMass2 * invDist) * (distanceBetween - m_Springs[iSpring].m_Length)) * g_ClothSpring.GetFloat();

			if (m_Masses[mass1Idx].m_bAnchored || m_Masses[mass2Idx].m_bAnchored) {
				finalOffset *= 2.0f;
			}

			if (m_Masses[mass1Idx].m_bAnchored == false) {
				m_Masses[mass1Idx].SetPosition(mass1Pos + (finalOffset * 0.5f));
			}

			if (m_Masses[mass2Idx].m_bAnchored == false) {
				m_Masses[mass2Idx].SetPosition(mass2Pos - (finalOffset * 0.5f));
			}
		}

		// Apply collisions
		for (int massIdx = 0; massIdx < m_Masses.size(); massIdx++) {
			if (m_Masses[massIdx].m_bAnchored)
				continue;

			Vec3 newLocation = m_Masses[massIdx].GetPosition();

			for (int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++) {
				const Vec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
				const float	 lenSqr = sphereToVert.length_sqr();
				const float W = CollisionSpheres[sphereIdx].w;
				if (lenSqr < W * W) {
					newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.normalize_safe() * (CollisionSpheres[sphereIdx].w);
				}
			}

			m_Masses[massIdx].SetPosition(newLocation);
		}
	}

	// Apply collisions
	for (int massIdx = 0; massIdx < m_Masses.size(); massIdx++) {
		if (m_Masses[massIdx].m_bAnchored)
			continue;

		Vec3 newLocation = m_Masses[massIdx].GetPosition();

		for (int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++) {
			const Vec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
			const float	 lenSqr = sphereToVert.length_sqr();
			const float W = CollisionSpheres[sphereIdx].w;
			if (lenSqr < W * W) {
				newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.normalize_safe() * (CollisionSpheres[sphereIdx].w);
			}
		}

		m_Masses[massIdx].SetPosition(newLocation);
	}

	// Update orientation
	for (size_t massIdx = 0; massIdx < m_BoneInfo.size(); massIdx++) {
		const size_t curX = massIdx % (size_t)m_Width;
		const size_t curY = massIdx / (size_t)m_Width;
		Vec3 xAxis;
		Vec3 yAxis;
		Vec3 zAxis;

		if (curX < (size_t)m_Width - 1) {
			xAxis = (m_Masses[massIdx + 1].GetPosition() - m_Masses[massIdx].GetPosition()).normalize_safe();
		} else {
			xAxis = (m_Masses[massIdx].GetPosition() - m_Masses[massIdx - 1].GetPosition()).normalize_safe();
		}

		if (curY > 0) {
			yAxis = (m_Masses[massIdx - m_Width].GetPosition() - m_Masses[massIdx].GetPosition()).normalize_safe();
		} else {
			yAxis = (m_Masses[massIdx].GetPosition() - m_Masses[massIdx + m_Width].GetPosition()).normalize_safe();
		}

		zAxis = xAxis.cross(yAxis).normalize_safe();
		yAxis = zAxis.cross(xAxis).normalize_safe();

		m_Masses[massIdx].SetAxis(0, xAxis);
		m_Masses[massIdx].SetAxis(1, yAxis);
		m_Masses[massIdx].SetAxis(2, zAxis);
	}
}

/// kbClothComponent::SetupCloth
void kbClothComponent::SetupCloth() {
	if (m_pSkeletalModel == nullptr) {// || m_BoneInfo.size() <= 2 || m_Width <= 2 || m_Height <= 2 ) {
		return;
	}

	// Create a list of skeletal bone indices
	m_Springs.clear();
	m_Masses.clear();
	m_BoneIndices.clear();
	m_BoneIndices.insert(m_BoneIndices.begin(), (int)m_BoneInfo.size(), -1);
	for (int i = 0; i < m_BoneIndices.size(); i++) {
		m_BoneIndices[i] = m_pSkeletalModel->GetBoneIndex(m_BoneInfo[i].m_BoneName);
		if (m_BoneIndices[i] < 0) {
			blk::error("Unable to find index for bone %s", m_BoneInfo[i].m_BoneName.c_str());
		}
	}

	// Add additional bone indices
	m_BoneIndices.insert(m_BoneIndices.begin() + m_BoneInfo.size(), m_AdditionalBoneInfo.size(), -1);
	for (int i = 0; i < m_AdditionalBoneInfo.size(); i++) {
		const int curIdx = (int)m_BoneInfo.size() + i;
		m_BoneIndices[curIdx] = m_pSkeletalModel->GetBoneIndex(m_AdditionalBoneInfo[i].m_BoneName);
		if (m_BoneIndices[curIdx] < 0) {
			blk::error("Unable to find index for bone %s index is %d", m_AdditionalBoneInfo[i].m_BoneName.c_str(), i);
		}
	}

	// Create our masses
	Mat4 scaleMatrix(Mat4::identity);
	scaleMatrix[0].x = GetOwner()->GetScale().x;
	scaleMatrix[1].y = GetOwner()->GetScale().y;
	scaleMatrix[2].z = GetOwner()->GetScale().z;

	Mat4 parentMatrix = scaleMatrix * GetOwner()->GetOrientation().to_mat4();
	parentMatrix[3] = GetOwner()->GetPosition();

	m_Masses.insert(m_Masses.begin(), (int)m_BoneInfo.size(), kbClothMass_t());
	for (int i = 0; i < m_Masses.size(); i++) {
		m_Masses[i].m_Matrix = m_pSkeletalModel->GetRefBoneMatrix(m_BoneIndices[i]);
		const Vec3 org = m_Masses[i].m_Matrix.GetOrigin() * kbLevelComponent::GetGlobalModelScale();
		m_Masses[i].m_Matrix.SetAxis(3, org);

		m_Masses[i].m_Matrix *= parentMatrix;
		m_Masses[i].m_Matrix.SetAxis(0, Vec3(1.0f, 0.0f, 0.0f));
		m_Masses[i].m_Matrix.SetAxis(1, Vec3(0.0f, 1.0f, 0.0f));
		m_Masses[i].m_Matrix.SetAxis(2, Vec3(0.0f, 0.0f, 1.0f));
		m_Masses[i].m_LastPosition = m_Masses[i].GetPosition();

		if (i < m_Width) {
			m_Masses[i].m_bAnchored = true;
		} else {
			m_Masses[i].m_bAnchored = false;
		}
	}

	m_Masses.insert(m_Masses.begin() + m_Masses.size(), m_AdditionalBoneInfo.size(), kbClothMass_t());
	for (int i = 0; i < this->m_AdditionalBoneInfo.size(); i++) {
		const int curIdx = (int)m_BoneInfo.size() + i;
		m_Masses[curIdx].m_Matrix = m_pSkeletalModel->GetRefBoneMatrix(m_BoneIndices[curIdx]);
		m_Masses[curIdx].m_Matrix *= parentMatrix;
		m_Masses[curIdx].m_Matrix.SetAxis(0, Vec3(1.0f, 0.0f, 0.0f));
		m_Masses[curIdx].m_Matrix.SetAxis(1, Vec3(0.0f, 1.0f, 0.0f));
		m_Masses[curIdx].m_Matrix.SetAxis(2, Vec3(0.0f, 0.0f, 1.0f));
		m_Masses[i].m_LastPosition = m_Masses[i].GetPosition();

		m_Masses[curIdx].m_bAnchored = m_AdditionalBoneInfo[i].m_bIsAnchored;
	}

	// Create our springs
	if (m_ClothType == CT_Square) {
		for (int row = 0; row < m_Height; row++) {
			for (int col = 0; col < m_Width; col++) {

				const int curBoneIdx = (row * m_Width) + col;

				if (col < m_Width - 1) {
					const int rightBoneIdx = curBoneIdx + 1;

					m_Springs.push_back(kbClothSpring_t());
					kbClothSpring_t& newSpring = m_Springs[m_Springs.size() - 1];
					newSpring.m_MassIndices[0] = curBoneIdx;
					newSpring.m_MassIndices[1] = rightBoneIdx;
					newSpring.m_Length = (m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[rightBoneIdx].m_Matrix.GetOrigin()).length();
				}

				if (row < m_Height - 1) {
					m_Springs.push_back(kbClothSpring_t());
					kbClothSpring_t& newSpring = m_Springs[m_Springs.size() - 1];

					const int downBoneIdx = ((row + 1) * m_Width) + col;
					newSpring.m_MassIndices[0] = curBoneIdx;
					newSpring.m_MassIndices[1] = downBoneIdx;
					newSpring.m_Length = (m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[downBoneIdx].m_Matrix.GetOrigin()).length();

					/*	if ( col < m_Width - 1 ) {
							m_Springs.push_back( kbClothSpring_t() );
							kbClothSpring_t & cross1 = m_Springs[m_Springs.size() - 1];

							const int bone2Idx = downBoneIdx + 1;
							cross1.m_MassIndices[0] = curBoneIdx;
							cross1.m_MassIndices[1] = bone2Idx;
							cross1.m_Length = ( m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[bone2Idx].m_Matrix.GetOrigin() ).length();
						}

						if ( col > 0 ) {
							m_Springs.push_back( kbClothSpring_t() );
							kbClothSpring_t & cross1 = m_Springs[m_Springs.size() - 1];

							const int bone1Idx = curBoneIdx;
							const int bone2Idx = downBoneIdx - 1;
							cross1.m_MassIndices[0] = bone1Idx;
							cross1.m_MassIndices[1] = bone2Idx;
							cross1.m_Length = ( m_Masses[bone1Idx].m_Matrix.GetOrigin() - m_Masses[bone2Idx].m_Matrix.GetOrigin() ).length();
						}*/
				}
			}
		}
	}

	// Additional Springs
	for (int i = 0; i < m_AdditionalBoneInfo.size(); i++) {
		const int curBoneIdx = (int)m_BoneInfo.size() + i;
		for (int j = 0; j < m_AdditionalBoneInfo[i].m_NeighborBones.size(); j++) {
			int neighborIdx;
			for (neighborIdx = 0; neighborIdx < m_AdditionalBoneInfo.size(); neighborIdx++) {
				if (m_AdditionalBoneInfo[neighborIdx].m_BoneName == m_AdditionalBoneInfo[i].m_NeighborBones[j]) {
					break;
				}
			}

			if (neighborIdx >= m_AdditionalBoneInfo.size()) {
				blk::error("Unable to find neighbor bone");
				continue;
			}
			neighborIdx += (int)m_BoneInfo.size();

			m_Springs.push_back(kbClothSpring_t());
			kbClothSpring_t& newSpring = m_Springs[m_Springs.size() - 1];
			newSpring.m_MassIndices[0] = curBoneIdx;
			newSpring.m_MassIndices[1] = neighborIdx;
			newSpring.m_Length = (m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[newSpring.m_MassIndices[1]].m_Matrix.GetOrigin()).length();
		}
	}
}

/// kbClothComponent::SetClothCollisionSphere
void kbClothComponent::SetClothCollisionSphere(const int idx, const Vec4& sphere) {

	if (idx < 0 || idx >= m_CollisionSpheres.size()) {
		return;
	}

	m_CollisionSpheres[idx].m_Sphere = sphere;
}
