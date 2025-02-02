//===================================================================================================
// kbClothComponent.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbClothComponent.h"
#include "kbConsole.h"
#include "DX11/kbRenderer_DX11.h"			// HACK

KB_DEFINE_COMPONENT(kbClothBone)
KB_DEFINE_COMPONENT(kbClothComponent)

kbConsoleVariable g_DebugCloth( "debugcloth", false, kbConsoleVariable::Console_Int, "Draw cloth debugging info.  Takes values 1-13", "" );
kbConsoleVariable g_ClothGrav( "clothgravity", 0.0f, kbConsoleVariable::Console_Float, "Cloth gravity", "" );
kbConsoleVariable g_ClothSpring( "clothspring", 0.5f, kbConsoleVariable::Console_Float, "Cloth spring", "" );
kbConsoleVariable g_ClothFriction( "clothFriction", 0.02f, kbConsoleVariable::Console_Float, "Cloth friction", "" );

/**
 *	kbClothBone::Constructor
 */
void kbClothBone::Constructor() {
	m_bIsAnchored = false;
}

/**
 *	kbBoneCollisionSphere::Constructor
 */
void kbBoneCollisionSphere::Constructor() {
	m_Sphere.Set( 0.0f, 0.0f, 0.0f, 10.0f );
}

/**
 *	kbClothComponent::~kbClothComponent
 */
kbClothComponent::~kbClothComponent() {
}

/**
 *	kbClothComponent::Initialize
 */
void kbClothComponent::Constructor() {
	m_ClothType = CT_None;
	m_Width = 0;
	m_Height = 0;
	m_pSkeletalModel = nullptr;
	m_NumConstrainIterations = 1;

	m_Gravity.Set( 0.0f, -100.0f, 0.0f );
	m_MaxWindVelocity.Set( 20.0f, 160.0f, -9.0f );
	m_MinWindVelocity.Set( 64.0f, 30.0f, -20.0f );
	m_MinWindGustDuration = 0.2f;
	m_MaxWindGustDuration = 0.35f;
	m_bAddFakeOscillation = false;

	m_CurWindVelocity = kbVec3::zero;
	m_NextWindVelocity = kbVec3::zero;
	m_NextWindChangeTime = 0;

	m_CurrentTickFrame = 0;
}

/**
 *	kbClothComponent::Update_Internal
 */
void kbClothComponent::Update_Internal( const float dt ) {
	Super::Update_Internal( dt );

	float DeltaTime = dt;
	if ( DeltaTime == 0.0f ) {
		DeltaTime = 0.016f;
	}
	// Dont start sim for a few frames in case the entity is teleported
	m_CurrentTickFrame++;
	if ( m_CurrentTickFrame < 5 ) {
		return;
	}

	kbSkeletalModelComponent * pSkelModelComponent = nullptr;
	for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
		if ( GetOwner()->GetComponent( i )->IsA( kbSkeletalModelComponent::GetType() ) ) {
			pSkelModelComponent = static_cast<kbSkeletalModelComponent*>( GetOwner()->GetComponent( i ) );
			if ( pSkelModelComponent->GetModel() != m_pSkeletalModel ) {
				m_pSkeletalModel = pSkelModelComponent->GetModel();
				SetupCloth();
				break;
			}
		}
	}

	if ( pSkelModelComponent == nullptr || m_pSkeletalModel == nullptr ) {
		return;
	}

	RunSimulation( DeltaTime );

	kbMat4 WorldMat;
	GetOwner()->CalculateWorldMatrix( WorldMat );

	// todo: Need my own inverse matrix function
	kbMat4 invParentMatrix;
	XMMATRIX inverseMat = XMMatrixInverse( nullptr, XMMATRIXFromkbMat4( WorldMat ) );
	invParentMatrix = kbMat4FromXMMATRIX( inverseMat );

	std::vector<kbBoneMatrix_t> & FinalBoneMatrices = pSkelModelComponent->GetFinalBoneMatrices();
	if ( FinalBoneMatrices.size() == 0 ) {
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

	for ( int i = 0; i < m_Masses.size(); i++ ) {

		const int BoneIndex = m_BoneIndices[i];

		if ( m_Masses[i].m_bAnchored ) {
			const kbBoneMatrix_t refBoneMatrix = pSkelModelComponent->GetBoneRefMatrix( BoneIndex );
			const kbVec3 localBonePos = refBoneMatrix.GetAxis(3);
			kbVec3 finalPosition = WorldMat.TransformPoint( localBonePos * FinalBoneMatrices[BoneIndex] );
			m_Masses[i].SetPosition( finalPosition );
		}


		const kbVec3 worldPos = m_Masses[i].GetPosition();
		kbBoneMatrix_t WorldToLocalSpace;
		WorldToLocalSpace.SetAxis( 0, m_Masses[i].GetAxis( 0 ) );
		WorldToLocalSpace.SetAxis( 1, m_Masses[i].GetAxis( 1 ) );
		WorldToLocalSpace.SetAxis( 2, m_Masses[i].GetAxis( 2 ) );
		WorldToLocalSpace.SetAxis( 3, worldPos );
		WorldToLocalSpace *= invParentMatrix;

		kbBoneMatrix_t LocalToRef = pSkelModelComponent->GetBoneRefMatrix( BoneIndex );
		LocalToRef.Invert();
		FinalBoneMatrices[BoneIndex] = LocalToRef * WorldToLocalSpace;

		if ( clothDebug == 1 || g_DebugCloth.GetInt() == 1 ) {
			kbBounds bounds( true );
			bounds.AddPoint( worldPos );
			bounds.AddPoint( worldPos + kbVec3( 0.1f, 0.1f, 0.1f ) );
			bounds.AddPoint( worldPos - kbVec3( 0.1f, 0.1f, 0.1f ) );

			if ( m_Masses[i].m_bAnchored ) {
				g_pRenderer->DrawBox( bounds, kbColor( 0.0f, 1.0f, 0.0f, 1.0f ) );
			} else {
				g_pRenderer->DrawBox( bounds, kbColor( 1.0f, 1.0f, 1.0f, 1.0f ) );
			}

			const float AxisLen = 1.0f;
			g_pRenderer->DrawLine( worldPos, worldPos + m_Masses[i].GetAxis( 0 ) * AxisLen, kbColor::red );
			g_pRenderer->DrawLine( worldPos, worldPos + m_Masses[i].GetAxis( 1 ) * AxisLen, kbColor::green );
			g_pRenderer->DrawLine( worldPos, worldPos + m_Masses[i].GetAxis( 2 ) * -AxisLen, kbColor::blue );
		}
	}


	if ( clothDebug == 2 || g_DebugCloth.GetInt() == 2 ) {
		for ( int i = 0; i < m_Springs.size(); i++ )  {
			const kbClothSpring_t & curSpring = m_Springs[i];
			const kbClothMass_t & Mass1 = m_Masses[curSpring.m_MassIndices[0]];
			const kbClothMass_t & Mass2 = m_Masses[curSpring.m_MassIndices[1]];
			g_pRenderer->DrawLine( Mass1.GetPosition(), Mass2.GetPosition(), kbColor::white );
		}

		for ( int iCollision = 0; iCollision < m_CollisionSpheres.size(); iCollision++ ) {
			kbBoneMatrix_t boneWorldMatrix;

			if ( GetAsyncKeyState('I') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.02f, 0.0f, 0.0f );
			}
			if ( GetAsyncKeyState('O') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( -0.02f, 0.0f, 0.0f );
			}

			if ( GetAsyncKeyState('K') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.f, 0.02f, 0.0f );
			}
			if ( GetAsyncKeyState('L') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.0f, -0.02f, 0.0f );
			}

			if ( GetAsyncKeyState('N') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.0f, 0.0f, 0.02f );
			}
			if ( GetAsyncKeyState('M') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.0f, 0.0f, -0.02f );
			}

			if ( GetAsyncKeyState(',') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.0f, 0.0f, 0.02f );
			}
			if ( GetAsyncKeyState('.') ) {
				m_CollisionSpheres[iCollision].m_Sphere.ToVec3() += kbVec3( 0.0f, 0.0f, -0.02f );
			}

			if ( GetAsyncKeyState('U') ) {
				kbLog( "%f %f %f", m_CollisionSpheres[iCollision].m_Sphere.x, m_CollisionSpheres[iCollision].m_Sphere.y, m_CollisionSpheres[iCollision].m_Sphere.z );
			}

	//		kbLog( "%s looking for %s", pSkelModelComponent->GetModel()->GetFullFileName().c_str(), m_CollisionSpheres[iCollision].m_BoneName.c_str() );

			if ( pSkelModelComponent->GetBoneWorldMatrix( m_CollisionSpheres[iCollision].m_BoneName, boneWorldMatrix ) ) {
				boneWorldMatrix.m_Axis[0].Normalize();
				boneWorldMatrix.m_Axis[1].Normalize();
				boneWorldMatrix.m_Axis[2].Normalize();

				kbVec3 spherePos = m_CollisionSpheres[iCollision].m_Sphere.ToVec3() * boneWorldMatrix;
				g_pRenderer->DrawSphere( spherePos, m_CollisionSpheres[iCollision].m_Sphere.w, 12, kbColor( 1.0f, 0.0f, 1.0f, 1.0f ) );
			}
		}
	}
}

/**
 *	kbClothComponent::RunSimulation
 */
void kbClothComponent::RunSimulation( const float inDeltaTime ) {

	const float DeltaTime = kbClamp( inDeltaTime, 0.0f, 0.016f );

	if ( m_bAddFakeOscillation ) {

		if ( g_GlobalTimer.TimeElapsedSeconds() >= m_NextWindChangeTime ) {
			m_NextWindChangeTime = g_GlobalTimer.TimeElapsedSeconds() + ( kbfrand() * ( m_MaxWindGustDuration- m_MinWindGustDuration ) ) + m_MinWindGustDuration;
			m_NextWindVelocity = kbVec3Rand( m_MinWindVelocity, m_MaxWindVelocity );
		}

		m_CurWindVelocity = kbLerp( m_CurWindVelocity, m_NextWindVelocity, 0.0075f );
	}


	// Wind Sim

	kbVec3 wind = m_CurWindVelocity;


	static kbVec3 BallPos( 0.0f, -35.0f, 0.0f );
	static kbVec3 BallDir( 0.0f, 0.0f, 1.0f );
	static float BallRad = 65;

	// Ball Sim
	/*BallPos = BallPos + BallDir * DeltaTime * 124.0f;
	if ( BallDir.z > 0 && BallPos.z > 200.0f )
		BallDir.z = -1.0f;
	else if ( BallDir.z < 0 && BallPos.z < -200.0f )
		BallDir.z = 1.0f;

	kbBounds bounds( true );
	bounds.AddPoint( BallPos ),
	bounds.AddPoint( BallPos + kbVec3( 1.0f, 1.0f, 1.0f ).Normalized() * BallRad );
	bounds.AddPoint( BallPos - ( kbVec3( 1.0f, 1.0f, 1.0f ).Normalized() * BallRad ) );

	std::vector<kbVec4> CollisionSpheres;
	kbVec4 newSphere( BallPos.x, BallPos.y, BallPos.z, 1.0f );
	newSphere.w = BallRad;
	CollisionSpheres.push_back( newSphere );

	g_pRenderer->DrawSphere( BallPos, BallRad, 16, kbColor::yellow);*/
	// Ball Sim - end

	kbSkeletalModelComponent * pSkelModelComponent = NULL;
	for ( int i = 0; i < GetOwner()->NumComponents(); i++ ) {
		if ( GetOwner()->GetComponent( i )->IsA( kbSkeletalModelComponent::GetType() ) ) {
			pSkelModelComponent = static_cast<kbSkeletalModelComponent*>( GetOwner()->GetComponent( i ) );
			if ( pSkelModelComponent->GetModel() != m_pSkeletalModel ) {
				break;
			}
		}
	}

	std::vector<kbVec4> CollisionSpheres;
	for ( int iCollision = 0; iCollision < m_CollisionSpheres.size(); iCollision++ ) {
		kbBoneMatrix_t boneWorldMatrix;
		if ( pSkelModelComponent->GetBoneWorldMatrix( m_CollisionSpheres[iCollision].m_BoneName, boneWorldMatrix ) ) {
			boneWorldMatrix.m_Axis[0].Normalize();
			boneWorldMatrix.m_Axis[1].Normalize();
			boneWorldMatrix.m_Axis[2].Normalize();
	
			const kbVec3 spherePos = m_CollisionSpheres[iCollision].m_Sphere.ToVec3() * boneWorldMatrix;
			CollisionSpheres.push_back( kbVec4( spherePos, m_CollisionSpheres[iCollision].m_Sphere.w ) );
		}
	}

	// Apply forces and update positions
	std::vector<float> a;
	for ( int i = 0; i < m_Width; i++ ) {
		float theRand = kbfrand();

		a.push_back( theRand );
		a.push_back( theRand );
		a.push_back( theRand );

	}

	float theRand = 0.0f;
	for ( int massIdx = 0; massIdx < m_Masses.size(); massIdx++ ) {
		if ( m_Masses[massIdx].m_bAnchored ) {
			m_Masses[massIdx].m_FrameForces = kbVec3::zero;
			continue;
		}

		kbVec3 totalForce = m_Gravity + kbVec3( 0.0f, g_ClothGrav.GetFloat(), 0.0f );
		if ( m_bAddFakeOscillation ) {

			kbVec3 windAmt =  ( ( wind - ( wind * 0.5f ) * kbfrand() + ( wind * 0.5f ) ) );
			int level = massIdx / m_Height;

			if ( massIdx % 1 == 0 ) {
				theRand = kbfrand();
			}
			if ( a[massIdx / m_Width] < 0.45f ) {
				windAmt *= 1.3f + kbfrand() * 1.35f;
			}
			totalForce += windAmt;
		}
		totalForce += m_Masses[massIdx].m_FrameForces;
		kbVec3 newLocation = m_Masses[massIdx].GetPosition();// + ( totalForce * DeltaTime );

		// ...
		/*for ( int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++ ) {
			const kbVec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
			if ( sphereToVert.LengthSqr() < ( CollisionSpheres[sphereIdx].w * CollisionSpheres[sphereIdx].w) ) {
				newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.Normalized() * CollisionSpheres[sphereIdx].w;
			}
		}*/
		// ...

		kbVec3 velocity = m_Masses[massIdx].GetPosition() - m_Masses[massIdx].m_LastPosition;
		m_Masses[massIdx].m_LastPosition = m_Masses[massIdx].GetPosition();

		newLocation += velocity * ( 1.0f - g_ClothFriction.GetFloat() ) + totalForce * ( DeltaTime * DeltaTime );
		m_Masses[massIdx].SetPosition( newLocation );
		m_Masses[massIdx].m_FrameForces = kbVec3::zero;
	}

	for ( int iIteration = 0; iIteration < m_NumConstrainIterations; iIteration++ ) {

		// Apply constraints
		for ( int iSpring = 0; iSpring < m_Springs.size(); iSpring++ ) {
			const int mass1Idx = m_Springs[iSpring].m_MassIndices[0];
			const int mass2Idx = m_Springs[iSpring].m_MassIndices[1];

			const kbVec3 mass1Pos = m_Masses[mass1Idx].GetPosition();
			const kbVec3 mass2Pos = m_Masses[mass2Idx].GetPosition();

			const kbVec3 mass1ToMass2 = mass2Pos - mass1Pos;
			const float distanceBetween = mass1ToMass2.Length();
			const float invDist = ( distanceBetween > 0.001f ) ? ( 1.0f / distanceBetween ) : ( 0.0f );
			kbVec3 finalOffset = ( ( mass1ToMass2 * invDist ) * ( distanceBetween - m_Springs[iSpring].m_Length ) ) * g_ClothSpring.GetFloat();

			if ( m_Masses[mass1Idx].m_bAnchored || m_Masses[mass2Idx].m_bAnchored ) {
				finalOffset *= 2.0f;
			}

			if ( m_Masses[mass1Idx].m_bAnchored == false ) {
				m_Masses[mass1Idx].SetPosition( mass1Pos + ( finalOffset * 0.5f ) );
			}

			if ( m_Masses[mass2Idx].m_bAnchored == false ) {
				m_Masses[mass2Idx].SetPosition( mass2Pos - ( finalOffset * 0.5f ) );
			}
		}

		// Apply collisions
		for ( int massIdx = 0; massIdx < m_Masses.size(); massIdx++ ) {
			if ( m_Masses[massIdx].m_bAnchored )
				continue;

			kbVec3 newLocation = m_Masses[massIdx].GetPosition();

			for ( int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++ ) {
				const kbVec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
				const float	 lenSqr = sphereToVert.LengthSqr();
				const float W = CollisionSpheres[sphereIdx].w;
				if ( lenSqr < W * W ) {
					newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.Normalized() * ( CollisionSpheres[sphereIdx].w );
				}
			}

			m_Masses[massIdx].SetPosition( newLocation );
		}
	}

		// Apply collisions
		for ( int massIdx = 0; massIdx < m_Masses.size(); massIdx++ ) {
			if ( m_Masses[massIdx].m_bAnchored )
				continue;

			kbVec3 newLocation = m_Masses[massIdx].GetPosition();

			for ( int sphereIdx = 0; sphereIdx < CollisionSpheres.size(); sphereIdx++ ) {
				const kbVec3 sphereToVert = newLocation - CollisionSpheres[sphereIdx].ToVec3();
				const float	 lenSqr = sphereToVert.LengthSqr();
				const float W = CollisionSpheres[sphereIdx].w;
				if ( lenSqr < W * W ) {
					newLocation = CollisionSpheres[sphereIdx].ToVec3() + sphereToVert.Normalized() * ( CollisionSpheres[sphereIdx].w );
				}
			}

			m_Masses[massIdx].SetPosition( newLocation );
		}

	// Update orientation
	for ( int massIdx = 0; massIdx < m_BoneInfo.size(); massIdx++ ) {
		const int curX = massIdx % m_Width;
		const int curY = massIdx / m_Width;
		kbVec3 xAxis;
		kbVec3 yAxis;
		kbVec3 zAxis;

		if ( curX < m_Width - 1 ) {
			xAxis = ( m_Masses[massIdx + 1].GetPosition() - m_Masses[massIdx].GetPosition() ).Normalized();
		} else {
			xAxis = ( m_Masses[massIdx].GetPosition() - m_Masses[massIdx - 1].GetPosition() ).Normalized();
		}

		if ( curY > 0 ) {
			yAxis = ( m_Masses[massIdx - m_Width].GetPosition() - m_Masses[massIdx].GetPosition() ).Normalized();
		} else {
			yAxis = ( m_Masses[massIdx].GetPosition() - m_Masses[massIdx + m_Width].GetPosition() ).Normalized();
		}

		zAxis = xAxis.Cross( yAxis ).Normalized();
		yAxis = zAxis.Cross( xAxis ).Normalized();

		m_Masses[massIdx].SetAxis( 0, xAxis );
		m_Masses[massIdx].SetAxis( 1, yAxis );
		m_Masses[massIdx].SetAxis( 2, zAxis );
	}
}

/**
 *	kbClothComponent::SetupCloth
 */
void kbClothComponent::SetupCloth() {
	if ( m_pSkeletalModel == nullptr ) {// || m_BoneInfo.size() <= 2 || m_Width <= 2 || m_Height <= 2 ) {
		return;
	}

	// Create a list of skeletal bone indices
	m_Springs.clear();
	m_Masses.clear();
	m_BoneIndices.clear();
	m_BoneIndices.insert( m_BoneIndices.begin(), (int)m_BoneInfo.size(), -1 );
	for ( int i = 0; i < m_BoneIndices.size(); i++ ) {
		m_BoneIndices[i] = m_pSkeletalModel->GetBoneIndex( m_BoneInfo[i].m_BoneName );
		if ( m_BoneIndices[i] < 0 ) {
			kbError( "Unable to find index for bone %s", m_BoneInfo[i].m_BoneName.c_str() );
		}
	}

	// Add additional bone indices
	m_BoneIndices.insert( m_BoneIndices.begin() + m_BoneInfo.size(), m_AdditionalBoneInfo.size(), -1 );
	for ( int i = 0; i < m_AdditionalBoneInfo.size(); i++ ) {
		const int curIdx = (int)m_BoneInfo.size() + i;
		m_BoneIndices[curIdx] = m_pSkeletalModel->GetBoneIndex( m_AdditionalBoneInfo[i].m_BoneName );
		if ( m_BoneIndices[curIdx] < 0 ) {
			kbError( "Unable to find index for bone %s index is %d", m_AdditionalBoneInfo[i].m_BoneName.c_str(), i );
		}
	}

	// Create our masses
	kbMat4 scaleMatrix( kbMat4::identity );
	scaleMatrix[0].x = GetOwner()->GetScale().x;
	scaleMatrix[1].y = GetOwner()->GetScale().y;
	scaleMatrix[2].z = GetOwner()->GetScale().z;

	kbMat4 parentMatrix = scaleMatrix * GetOwner()->GetOrientation().ToMat4();
	parentMatrix[3] = GetOwner()->GetPosition();

	m_Masses.insert( m_Masses.begin(), (int)m_BoneInfo.size(), kbClothMass_t() );
	for ( int i = 0; i < m_Masses.size(); i++ ) {
		m_Masses[i].m_Matrix = m_pSkeletalModel->GetRefBoneMatrix( m_BoneIndices[i] );
		const kbVec3 org = m_Masses[i].m_Matrix.GetOrigin() * kbLevelComponent::GetGlobalModelScale();
		m_Masses[i].m_Matrix.SetAxis( 3, org );

		m_Masses[i].m_Matrix *= parentMatrix;
		m_Masses[i].m_Matrix.SetAxis( 0, kbVec3( 1.0f, 0.0f, 0.0f ) );
		m_Masses[i].m_Matrix.SetAxis( 1, kbVec3( 0.0f, 1.0f, 0.0f ) );
		m_Masses[i].m_Matrix.SetAxis( 2, kbVec3( 0.0f, 0.0f, 1.0f ) );
		m_Masses[i].m_LastPosition = m_Masses[i].GetPosition();

		if ( i < m_Width ) {
			m_Masses[i].m_bAnchored = true;
		} else {
			m_Masses[i].m_bAnchored = false;
		}
	}

	m_Masses.insert( m_Masses.begin() + m_Masses.size(), m_AdditionalBoneInfo.size(), kbClothMass_t() );
	for ( int i = 0; i < this->m_AdditionalBoneInfo.size(); i++ ) {
		const int curIdx = (int)m_BoneInfo.size() + i;
		m_Masses[curIdx].m_Matrix = m_pSkeletalModel->GetRefBoneMatrix( m_BoneIndices[curIdx] );
		m_Masses[curIdx].m_Matrix *= parentMatrix;
		m_Masses[curIdx].m_Matrix.SetAxis( 0, kbVec3( 1.0f, 0.0f, 0.0f ) );
		m_Masses[curIdx].m_Matrix.SetAxis( 1, kbVec3( 0.0f, 1.0f, 0.0f ) );
		m_Masses[curIdx].m_Matrix.SetAxis( 2, kbVec3( 0.0f, 0.0f, 1.0f ) );
		m_Masses[i].m_LastPosition = m_Masses[i].GetPosition();

		m_Masses[curIdx].m_bAnchored = m_AdditionalBoneInfo[i].m_bIsAnchored;
	}

	// Create our springs
	if ( m_ClothType == CT_Square ) {
		for ( int row = 0; row < m_Height; row++ ) {
			for ( int col = 0; col < m_Width; col++ ) {

				const int curBoneIdx = ( row * m_Width ) + col;

				if ( col < m_Width - 1 ) {
					const int rightBoneIdx = curBoneIdx + 1;

					m_Springs.push_back( kbClothSpring_t() );
					kbClothSpring_t & newSpring = m_Springs[m_Springs.size() - 1];
					newSpring.m_MassIndices[0] = curBoneIdx;
					newSpring.m_MassIndices[1] = rightBoneIdx;
					newSpring.m_Length = ( m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[rightBoneIdx].m_Matrix.GetOrigin() ).Length();
				}

				if ( row < m_Height - 1 ) {
					m_Springs.push_back( kbClothSpring_t() );
					kbClothSpring_t & newSpring = m_Springs[m_Springs.size() - 1];

					const int downBoneIdx = ( ( row + 1 ) * m_Width ) + col;
					newSpring.m_MassIndices[0] = curBoneIdx;
					newSpring.m_MassIndices[1] = downBoneIdx;
					newSpring.m_Length = ( m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[downBoneIdx].m_Matrix.GetOrigin() ).Length();

				/*	if ( col < m_Width - 1 ) {
						m_Springs.push_back( kbClothSpring_t() );
						kbClothSpring_t & cross1 = m_Springs[m_Springs.size() - 1];

						const int bone2Idx = downBoneIdx + 1;
						cross1.m_MassIndices[0] = curBoneIdx;
						cross1.m_MassIndices[1] = bone2Idx;
						cross1.m_Length = ( m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[bone2Idx].m_Matrix.GetOrigin() ).Length();
					}

					if ( col > 0 ) {
						m_Springs.push_back( kbClothSpring_t() );
						kbClothSpring_t & cross1 = m_Springs[m_Springs.size() - 1];

						const int bone1Idx = curBoneIdx;
						const int bone2Idx = downBoneIdx - 1;
						cross1.m_MassIndices[0] = bone1Idx;
						cross1.m_MassIndices[1] = bone2Idx;
						cross1.m_Length = ( m_Masses[bone1Idx].m_Matrix.GetOrigin() - m_Masses[bone2Idx].m_Matrix.GetOrigin() ).Length();
					}*/
				}
			}
		}
	}

	// Additional Springs
	for ( int i = 0; i < m_AdditionalBoneInfo.size(); i++ ) {
		const int curBoneIdx = (int)m_BoneInfo.size() + i;
		for ( int j = 0; j < m_AdditionalBoneInfo[i].m_NeighborBones.size(); j++ ) {
			int neighborIdx;
			for ( neighborIdx = 0; neighborIdx < m_AdditionalBoneInfo.size(); neighborIdx++ ) {
				if ( m_AdditionalBoneInfo[neighborIdx].m_BoneName == m_AdditionalBoneInfo[i].m_NeighborBones[j] ) {
					break;
				}
			}

			if ( neighborIdx >= m_AdditionalBoneInfo.size() ) {
				kbError( "Unable to find neighbor bone" );
				continue;
			}
			neighborIdx += (int)m_BoneInfo.size();

			m_Springs.push_back( kbClothSpring_t() );
			kbClothSpring_t & newSpring = m_Springs[m_Springs.size() - 1];
			newSpring.m_MassIndices[0] = curBoneIdx;
			newSpring.m_MassIndices[1] = neighborIdx;
			newSpring.m_Length = ( m_Masses[curBoneIdx].m_Matrix.GetOrigin() - m_Masses[newSpring.m_MassIndices[1]].m_Matrix.GetOrigin() ).Length();			
		}
	}
}

/**
 *	kbClothComponent::SetClothCollisionSphere
 */
void kbClothComponent::SetClothCollisionSphere( const int idx, const kbVec4 & sphere ) {

	if ( idx < 0 || idx >= m_CollisionSpheres.size() ) {
		return;
	}

	m_CollisionSpheres[idx].m_Sphere = sphere;
}
