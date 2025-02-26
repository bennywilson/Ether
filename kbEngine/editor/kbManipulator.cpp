/// kbManipulator.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbLevelComponent.h"
#include "kbManipulator.h"
#include "DX11/kbRenderer_DX11.h"

/// kbManipulator::kbManipulator
kbManipulator::kbManipulator() :
	m_ManipulatorMode(kbManipulator::Translate),
	m_SelectedGroup(-1) {

	m_Orientation.set(0.0f, 0.0f, 0.0f, 1.0f);
	m_Scale.set(1.0f, 1.0f, 1.0f);

	memset(m_models, 0, sizeof(m_models));
}

/// kbManipulator::~kbManipulator
kbManipulator::~kbManipulator() { }

/// kbManipulator::AttemptMouseGrab
bool kbManipulator::AttemptMouseGrab(const Vec3& rayOrigin, const Vec3& rayDirection, const Quat4& cameraOrientation) {
	const kbModel* const pModel = m_models[m_ManipulatorMode];

	const float modelScale = kbLevelComponent::GetGlobalModelScale();
	kbModelIntersection_t intersection = pModel->RayIntersection(rayOrigin, rayDirection, m_Position, m_Orientation, Vec3(modelScale, modelScale, modelScale));

	if (intersection.hasIntersection == false) {
		intersection = pModel->RayIntersection(rayOrigin, -rayDirection, m_Position, m_Orientation, Vec3(modelScale, modelScale, modelScale));
	}
	if (intersection.hasIntersection) {
		m_SelectedGroup = intersection.meshNum;

		if (m_SelectedGroup != -1) {

			/*	if ( m_ManipulatorMode == kbManipulator::Translate || m_ManipulatorMode == kbManipulator::Scale ) {
					m_SelectedGroup /= 2;
				}*/
			Vec3 worldSpaceGrabPoint = intersection.intersectionPoint;
			m_MouseLocalGrabPoint = worldSpaceGrabPoint - m_Position;
			m_MouseWorldGrabPoint = worldSpaceGrabPoint;
			m_LastOrientation = m_Orientation;
			m_LastScale = m_Scale;
			UpdateMouseDrag(rayOrigin, rayDirection, cameraOrientation);
			return true;
		}
	}

	return false;
}

/// kbManipulator::UpdateMouseDrag
void kbManipulator::UpdateMouseDrag(const Vec3& rayOrigin, const Vec3& rayDirection, const Quat4& cameraOrientation) {
	if (m_SelectedGroup < 0 || m_SelectedGroup > 3) {
		return;
	}

	// Find intersection point with the plane facing the camera that goes through the mouse grab point
	const Vec3 cameraPlaneNormal = cameraOrientation.to_mat4()[2].ToVec3();
	const float d = m_MouseWorldGrabPoint.dot(cameraPlaneNormal);
	const float t = -(rayOrigin.dot(cameraPlaneNormal) - d) / rayDirection.dot(cameraPlaneNormal);
	const Vec3 camPlaneIntersection = rayOrigin + t * rayDirection;

	// Find the normal of the plane we'd like to move the object along
	const Mat4 manipulatorMatrix = m_LastOrientation.to_mat4();
	Vec3 movePlaneNormal = Vec3::up;

	if (m_SelectedGroup < 3) {
		const int planeNormalIndex = (m_SelectedGroup + 1) % 3;
		movePlaneNormal = manipulatorMatrix[planeNormalIndex].ToVec3().normalize_safe();
	}

	if (m_ManipulatorMode == kbManipulator::Translate) {
		if (m_SelectedGroup < 3) {
			const float distFromPlane = camPlaneIntersection.dot(movePlaneNormal) - m_MouseWorldGrabPoint.dot(movePlaneNormal);
			const Vec3 intersectionPoint = camPlaneIntersection - (movePlaneNormal * distFromPlane);
			const Vec3 moveDirection = manipulatorMatrix[m_SelectedGroup].ToVec3();

			const Vec3 finalTranslation = (intersectionPoint - m_MouseWorldGrabPoint).dot(moveDirection) * moveDirection;
			m_Position = (m_MouseWorldGrabPoint + finalTranslation) - m_MouseLocalGrabPoint;
		}
		else {
			m_Position = camPlaneIntersection - m_MouseLocalGrabPoint;
		}
	}
	else if (m_ManipulatorMode == kbManipulator::Rotate) {
		const float rotationRadius = (m_MouseWorldGrabPoint - m_Position).length();
		vecToGrabPoint = (m_MouseWorldGrabPoint - m_Position).normalize_safe();
		vecToNewPoint = (camPlaneIntersection - m_Position).normalize_safe();

		// find the angle between the old and new placements
		float rotationAngle = acos(vecToGrabPoint.dot(vecToNewPoint));
		const Vec3 crossTest = vecToGrabPoint.cross(vecToNewPoint);
		if ((crossTest.dot(movePlaneNormal)) > 0.0f) {
			rotationAngle *= -1.0f;
		}

		const Vec3 rotationAxes[] = { manipulatorMatrix[1].ToVec3(), manipulatorMatrix[2].ToVec3(), manipulatorMatrix[0].ToVec3() };
		const Quat4 rot(rotationAxes[m_SelectedGroup], rotationAngle);

		// Final rotation
		m_Orientation = (m_LastOrientation * rot).normalize_safe();
	} else if (m_ManipulatorMode == kbManipulator::Scale) {
		const float initialDist = (m_MouseLocalGrabPoint - m_Position).length();
		const float curDist = (camPlaneIntersection - m_MouseLocalGrabPoint).length();
		const float scaleAmount = curDist / initialDist;

		m_Scale.set(scaleAmount, scaleAmount, scaleAmount);
	}
}

/// kbManipulator::Update
void kbManipulator::Update() {
	if (g_pRenderer->DebugBillboardsEnabled()) {
		const Vec3 modelScale(kbLevelComponent::GetGlobalModelScale(), kbLevelComponent::GetGlobalModelScale(), kbLevelComponent::GetGlobalModelScale());
		g_pRenderer->DrawModel(m_models[m_ManipulatorMode], m_ManipulatorMaterials, m_Position, m_Orientation, modelScale, UINT16_MAX);
	}
}

/// kbManipulator::RenderSync
void kbManipulator::RenderSync() {
	static bool bFirstUpdate = true;
	if (bFirstUpdate == true) {
		bFirstUpdate = false;
		kbShaderParamOverrides_t material;
		material.m_shader = (kbShader*)g_ResourceManager.GetResource("../../kbEngine/assets/Shaders/UIManipulator.kbshader", true, true);
		kbTexture* const pTexture = (kbTexture*)g_ResourceManager.GetResource("../../kbEngine/assets/editor/manipulator.bmp", true, true);
		material.SetTexture("shaderTexture", pTexture);
		m_ManipulatorMaterials.push_back(material);
		m_ManipulatorMaterials.push_back(material);
		m_ManipulatorMaterials.push_back(material);

		m_ManipulatorMaterials.push_back(material);
		m_models[kbManipulator::Translate] = (kbModel*)g_ResourceManager.GetResource("../../kbEngine/assets/Models/Editor/translationManipulator.ms3d", true, true);
		m_models[kbManipulator::Rotate] = (kbModel*)g_ResourceManager.GetResource("../../kbEngine/assets/Models/Editor/rotationManipulator.ms3d", true, true);
		m_models[kbManipulator::Scale] = (kbModel*)g_ResourceManager.GetResource("../../kbEngine/assets/Models/Editor/scaleManipulator.ms3d", true, true);

		blk::error_check(m_models[kbManipulator::Translate] != nullptr && m_models[kbManipulator::Rotate] != nullptr && m_models[kbManipulator::Scale] != nullptr, "kbManipulator::RenderSync() - Unable to load manipulator models");
	}
}

/// kbManipulator::ProcessInput
void kbManipulator::ProcessInput(const bool leftMouseDown) {
	if (leftMouseDown == true && m_SelectedGroup != -1) {
		switch (m_ManipulatorMode) {
			case kbManipulator::Rotate: {
				const float rotationRadius = (m_MouseWorldGrabPoint - m_Position).length();

				// Draw vectors that show angle between old and new location
				g_pRenderer->DrawLine(m_Position, m_Position + vecToGrabPoint * rotationRadius, kbColor::red);
				g_pRenderer->DrawLine(m_Position, m_Position + vecToNewPoint * rotationRadius, kbColor::blue);
			}
		break;
		}
	}
}
