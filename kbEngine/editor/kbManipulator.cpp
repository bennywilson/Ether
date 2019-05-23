//===================================================================================================
// kbManipulator.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbModel.h"
#include "kbManipulator.h"
#include "DX11/kbRenderer_DX11.h"

/**
 *	kbManipulator::kbManipulator
 */
kbManipulator::kbManipulator() :
	m_ManipulatorMode( kbManipulator::Translate ),
	m_SelectedGroup( -1 ) {
	
	m_Orientation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	m_Scale.Set( 1.0f, 1.0f, 1.0f );

	memset( m_pModels, 0, sizeof( m_pModels ) );
}

/**
 *	kbManipulator::~kbManipulator
 */
kbManipulator::~kbManipulator() {
}
	
/**
 *	kbManipulator::AttemptMouseGrab
 */
bool kbManipulator::AttemptMouseGrab( const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbQuat & cameraOrientation ) {

	kbModel *const pModel = m_pModels[m_ManipulatorMode];

	kbModelIntersection_t intersection = pModel->RayIntersection( rayOrigin, rayDirection, m_Position, m_Orientation );

	if ( intersection.hasIntersection ) {
		m_SelectedGroup = intersection.meshNum;

		if ( m_SelectedGroup != -1 ) {
	
		/*	if ( m_ManipulatorMode == kbManipulator::Translate || m_ManipulatorMode == kbManipulator::Scale ) {
				m_SelectedGroup /= 2;
			}*/
			kbVec3 worldSpaceGrabPoint = intersection.intersectionPoint;
			m_MouseLocalGrabPoint = worldSpaceGrabPoint - m_Position;
			m_MouseWorldGrabPoint = worldSpaceGrabPoint;
			m_LastOrientation = m_Orientation;
			m_LastScale = m_Scale;
			UpdateMouseDrag( rayOrigin, rayDirection, cameraOrientation );
			return true;
		}
	}

	return false;
}

/**
 *	kbManipulator::UpdateMouseDrag
 */
void kbManipulator::UpdateMouseDrag( const kbVec3 & rayOrigin, const kbVec3 & rayDirection, const kbQuat & cameraOrientation ) {

	if ( m_SelectedGroup < 0 || m_SelectedGroup > 3 ) {
		return;
	}

	// Find intersection point with the plane facing the camera that goes through the mouse grab point
	const kbVec3 cameraPlaneNormal = cameraOrientation.ToMat4()[2].ToVec3();
	const float d = m_MouseWorldGrabPoint.Dot( cameraPlaneNormal );
	const float t = -( rayOrigin.Dot( cameraPlaneNormal ) - d ) / rayDirection.Dot( cameraPlaneNormal );
	const kbVec3 camPlaneIntersection = rayOrigin + t * rayDirection;

	// Find the normal of the plane we'd like to move the object along
	const kbMat4 manipulatorMatrix = m_LastOrientation.ToMat4();
	kbVec3 movePlaneNormal = kbVec3::up;

	if ( m_SelectedGroup < 3 ) {
		const int planeNormalIndex = ( m_SelectedGroup + 1 ) % 3;
		movePlaneNormal = manipulatorMatrix[planeNormalIndex].ToVec3().Normalized();	
	}

	if ( m_ManipulatorMode == kbManipulator::Translate ) {

		if ( m_SelectedGroup < 3 ) {
			const float distFromPlane = camPlaneIntersection.Dot( movePlaneNormal ) - m_MouseWorldGrabPoint.Dot( movePlaneNormal );
			const kbVec3 intersectionPoint = camPlaneIntersection - ( movePlaneNormal * distFromPlane );
			const kbVec3 moveDirection = manipulatorMatrix[m_SelectedGroup].ToVec3();

			const kbVec3 finalTranslation = ( intersectionPoint - m_MouseWorldGrabPoint ).Dot( moveDirection ) * moveDirection;
			m_Position = ( m_MouseWorldGrabPoint + finalTranslation ) - m_MouseLocalGrabPoint;

		} else {
			m_Position = camPlaneIntersection - m_MouseLocalGrabPoint;
		}
	} else if ( m_ManipulatorMode == kbManipulator::Rotate ) {

		const float rotationRadius = ( m_MouseWorldGrabPoint - m_Position ).Length();
		vecToGrabPoint = ( m_MouseWorldGrabPoint - m_Position ).Normalized();
		vecToNewPoint = ( camPlaneIntersection - m_Position ).Normalized();

		// find the angle between the old and new placements
		float rotationAngle = acos( vecToGrabPoint.Dot( vecToNewPoint ) );
		const kbVec3 crossTest = vecToGrabPoint.Cross( vecToNewPoint );
		if ( ( crossTest.Dot( movePlaneNormal ) ) > 0.0f ) {
			rotationAngle *= -1.0f;
		}

		const kbVec3 rotationAxes[] = { manipulatorMatrix[1].ToVec3(), manipulatorMatrix[2].ToVec3(), manipulatorMatrix[0].ToVec3() };
		const kbQuat rot( rotationAxes[m_SelectedGroup], rotationAngle );

		// Final rotation
		m_Orientation = ( m_LastOrientation * rot ).Normalized();
	} else if ( m_ManipulatorMode == kbManipulator::Scale ) {
		const float initialDist = ( m_MouseLocalGrabPoint - m_Position ).Length();
		const float curDist = ( camPlaneIntersection - m_MouseLocalGrabPoint ).Length();
		const float scaleAmount = curDist / initialDist;
		
		m_Scale.Set( scaleAmount, scaleAmount, scaleAmount );
	}
}

/**
 *	kbManipulator::Update
 */
void kbManipulator::Update() {
	if ( g_pRenderer->DebugBillboardsEnabled() ) {
		g_pRenderer->DrawModel( m_pModels[m_ManipulatorMode], m_ManipulatorMaterials, m_Position, m_Orientation, kbVec3::one, UINT16_MAX );
	}
}

/**
 *	kbManipulator::RenderSync
 */
void kbManipulator::RenderSync() {

	static bool bFirstUpdate = true;
	if ( bFirstUpdate == true ) {

		bFirstUpdate = false;
		kbShaderParamOverrides_t material;
		material.m_pShader = (kbShader *) g_ResourceManager.GetResource( "../../kbEngine/assets/Shaders/UIManipulator.kbshader", true, true );
		kbTexture *const pTexture = (kbTexture *) g_ResourceManager.GetResource( "../../kbEngine/assets/editor/manipulator.bmp", true, true );
		material.SetTexture( "shaderTexture", pTexture );
		m_ManipulatorMaterials.push_back( material );
		m_ManipulatorMaterials.push_back( material );
		m_ManipulatorMaterials.push_back( material );

		m_ManipulatorMaterials.push_back( material );
		m_pModels[kbManipulator::Translate] = (kbModel *) g_ResourceManager.GetResource( "../../kbEngine/assets/Models/Editor/translationManipulator.ms3d", true, true  );
		m_pModels[kbManipulator::Rotate] = (kbModel *) g_ResourceManager.GetResource( "../../kbEngine/assets/Models/Editor/rotationManipulator.ms3d", true, true  );
		m_pModels[kbManipulator::Scale] = (kbModel *) g_ResourceManager.GetResource( "../../kbEngine/assets/Models/Editor/scaleManipulator.ms3d", true, true  );

		kbErrorCheck( m_pModels[kbManipulator::Translate] != nullptr && m_pModels[kbManipulator::Rotate] != nullptr && m_pModels[kbManipulator::Scale] != nullptr, "kbManipulator::RenderSync() - Unable to load manipulator models" );
	}
}

/**
 *	kbManipulator::ProcessInput
 */
void kbManipulator::ProcessInput( const bool leftMouseDown ) {
	if ( leftMouseDown == true && m_SelectedGroup != -1 ) {
		switch( m_ManipulatorMode ) {
			case kbManipulator::Rotate :
			{
				const float rotationRadius = ( m_MouseWorldGrabPoint - m_Position ).Length();

				// Draw vectors that show angle between old and new location
				g_pRenderer->DrawLine( m_Position, m_Position + vecToGrabPoint * rotationRadius, kbColor::red );
				g_pRenderer->DrawLine( m_Position, m_Position + vecToNewPoint * rotationRadius, kbColor::blue );
			}
			break;
		}
	}
}
