//==============================================================================
// kbRenderer.cpp
//
// Renderer implementation
//
// 2018 blk 1.0
//==============================================================================
#include "kbCore.h"
#include "containers.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbRenderer.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbLightComponent.h"
#include "renderer.h"

const float g_DebugLineSpacing = 0.0165f + 0.007f;
const float g_DebugTextSize = 0.0165f;

kbRenderer* g_pRenderer = nullptr;

/**
 *	kbRenderSubmesh::GetShader
 */
const kbShader* kbRenderSubmesh::GetShader() const {
	const kbRenderObject& renderObj = *GetRenderObject();

	const kbModel* const pModel = GetRenderObject()->m_pModel;
	const kbModel::mesh_t& mesh = pModel->GetMeshes()[GetMeshIdx()];
	if (pModel->GetMaterials().size() > mesh.m_MaterialIndex) {
		return pModel->GetMaterials()[mesh.m_MaterialIndex].GetShader();
	}

	return nullptr;
}

/**
 *	kbRenderWindow::kbRenderWindow
 */
kbRenderWindow::kbRenderWindow(HWND inHwnd, const RECT& windowDimensions, const float nearPlane, const float farPlane) :
	m_Hwnd(inHwnd),
	m_NearPlane_GameThread(nearPlane),
	m_NearPlane_RenderThread(m_NearPlane_GameThread),
	m_FarPlane_GameThread(farPlane),
	m_FarPlane_RenderThread(m_FarPlane_GameThread),
	m_ViewPixelWidth(0),
	m_ViewPixelHeight(0),
	m_fViewPixelWidth(0.0f),
	m_fViewPixelHeight(0.0f),
	m_fViewPixelHalfWidth(0.0f),
	m_fViewPixelHalfHeight(0.0f) {

	m_ProjectionMatrix.make_identity();
	m_InverseProjectionMatrix.make_identity();
	m_ViewMatrix.make_identity();
	m_ViewProjectionMatrix.make_identity();
	m_InverseViewProjectionMatrix.make_identity();
	m_CameraRotation.x = m_CameraRotation.y = m_CameraRotation.z = m_CameraRotation.w = 0.0f;

	m_ViewPixelWidth = windowDimensions.right - windowDimensions.left;
	m_ViewPixelHeight = windowDimensions.bottom - windowDimensions.top;
	m_fViewPixelWidth = static_cast<float>(m_ViewPixelWidth);
	m_fViewPixelHeight = static_cast<float>(m_ViewPixelHeight);
	m_fViewPixelHalfWidth = m_fViewPixelWidth * 0.5f;
	m_fViewPixelHalfHeight = m_fViewPixelHeight * 0.5f;
	m_ProjectionMatrix.create_perspective_matrix(kbToRadians(50.0f), m_fViewPixelWidth / m_fViewPixelHeight, nearPlane, farPlane);
}

/**
 *	kbRenderWindow::~kbRenderWindow
 */
kbRenderWindow::~kbRenderWindow() {

	{
		auto iter = m_RenderObjectMap.begin();

		for (; iter != m_RenderObjectMap.end(); iter++) {
			delete iter->second;
		}
	}

	{
		std::map<const kbLightComponent*, kbRenderLight*>::iterator iter;

		for (iter = m_RenderLightMap.begin(); iter != m_RenderLightMap.end(); iter++) {
			delete iter->second;
		}
	}
}


/**
 *	kbRenderer::SetRenderViewTransform
 */
void kbRenderer::SetRenderViewTransform(const HWND hwnd, const Vec3& position, const Quat4& rotation) {
	int viewIndex = -1;

	if (hwnd == nullptr) {
		viewIndex = 0;
	} else {
		for (int i = 0; i < m_RenderWindowList.size(); i++) {
			if (m_RenderWindowList[i]->GetHWND() == hwnd) {
				viewIndex = i;
				break;
			}
		}
	}

	if (viewIndex < 0 || viewIndex >= m_RenderWindowList.size()) {
		blk::warn("kbRenderer::SetRenderViewTransform() - Invalid view index");
		return;
	}

	m_RenderWindowList[viewIndex]->m_CameraPosition_GameThread = position;
	m_RenderWindowList[viewIndex]->m_CameraRotation_GameThread = rotation;
}

/**
 *	kbRenderer::GetRenderViewTransform
 */
void kbRenderer::GetRenderViewTransform(const HWND hwnd, Vec3& position, Quat4& rotation) {
	int viewIndex = -1;

	if (hwnd == nullptr) {
		viewIndex = 0;
	} else {
		for (int i = 0; i < m_RenderWindowList.size(); i++) {
			if (m_RenderWindowList[i]->m_Hwnd == hwnd) {
				viewIndex = i;
				break;
			}
		}
	}

	if (viewIndex < 0 || viewIndex >= m_RenderWindowList.size()) {
		blk::error("Invalid view index");
	}

	position = m_RenderWindowList[viewIndex]->m_CameraPosition;
	rotation = m_RenderWindowList[viewIndex]->m_CameraRotation;
}

/**
 *	kbRenderer::SetNearFarPlane
 */
void kbRenderer::SetNearFarPlane(const HWND hwnd, const float nearPlane, const float farPlane) {
	int viewIndex = -1;

	if (hwnd == nullptr) {
		viewIndex = 0;
	} else {
		for (int i = 0; i < m_RenderWindowList.size(); i++) {
			if (m_RenderWindowList[i]->m_Hwnd == hwnd) {
				viewIndex = i;
				break;
			}
		}
	}

	if (viewIndex < 0 || viewIndex >= m_RenderWindowList.size()) {
		blk::error("Invalid view index");
	}

	m_RenderWindowList[viewIndex]->m_NearPlane_GameThread = nearPlane;
	m_RenderWindowList[viewIndex]->m_FarPlane_GameThread = farPlane;
}

/**
 *	kbRenderWindow::BeginFrame
 */
void kbRenderWindow::BeginFrame() {
	Mat4 translationMatrix(Mat4::identity);
	translationMatrix[3].ToVec3() = -m_CameraPosition;
	Mat4 rotationMatrix = m_CameraRotation.to_mat4();
	rotationMatrix.transpose_self();

	m_ViewMatrix = translationMatrix * rotationMatrix;

	m_ProjectionMatrix.create_perspective_matrix(kbToRadians(50.0f), m_fViewPixelWidth / m_fViewPixelHeight, m_NearPlane_RenderThread, m_FarPlane_RenderThread);

	m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;

	BeginFrame_Internal();
}

/**
 *	kbRenderWindow::EndFrame
 */
void kbRenderWindow::EndFrame() {
	EndFrame_Internal();
}

/**
 *	kbRenderWindow::Release
 */
void kbRenderWindow::Release() {
	Release_Internal();
}

/**
 *	kbRenderJob::Run()
 */
void kbRenderJob::Run() {
	SetThreadName("Render thread");
	while (m_bRequestShutdown == false) {
		if (g_pRenderer != nullptr && g_pRenderer->m_RenderThreadSync == 1) {
			g_pRenderer->RenderScene();
			g_pRenderer->m_RenderThreadSync = 0;
		}
	}

	MarkJobAsComplete();
};

/**
 *	kbRenderer::kbRenderer
 */
kbRenderer::kbRenderer() :
	Back_Buffer_Width(1280),
	Back_Buffer_Height(1024),
	m_GlobalModelScale_GameThread(1.0f),
	m_GlobalModelScale_RenderThread(1.0f),
	m_EditorIconScale_GameThread(1.0f),
	m_EditorIconScale_RenderThread(1.0f),
	m_pCurrentRenderWindow(nullptr),
	m_ViewMode_GameThread(ViewMode_Shaded),
	m_ViewMode(ViewMode_Shaded),
	m_FogColor_GameThread(1.0f, 1.0f, 1.0f, 1.0f),
	m_FogColor_RenderThread(1.0f, 1.0f, 1.0f, 1.0f),
	m_FogStartDistance_GameThread(2100),
	m_FogStartDistance_RenderThread(2200),
	m_FogEndDistance_GameThread(2100),
	m_FogEndDistance_RenderThread(2200),
	m_bConsoleEnabled(false),
	m_pRenderJob(nullptr),
	m_RenderThreadSync(0),
	m_bDebugBillboardsEnabled(false) {

	m_pAccumBuffers[0] = m_pAccumBuffers[1] = nullptr;
	m_iAccumBuffer = 0;

	g_pRenderer = this;
}

/**
 *	kbRenderer::~kbRenderer
 */
kbRenderer::~kbRenderer() {

}

/**
 *	kbRenderer::Init
 */
void kbRenderer::Init(HWND hwnd, const int width, const int height) {

	blk::log("Initializing kbRenderer");
	const float startInitTime = g_GlobalTimer.TimeElapsedSeconds();

	Init_Internal(hwnd, width, height);

	if (g_renderer == nullptr) {
		// Kick off render thread
		m_pRenderJob = new kbRenderJob();
		g_pJobManager->RegisterJob(m_pRenderJob);
	}

	blk::log("	Rendered Initialized.  Took %f seconds", g_GlobalTimer.TimeElapsedSeconds() - startInitTime);
}

/**
 *	kbRenderer::Shutdown
 */
void kbRenderer::Shutdown() {

	blk::log("Shutting down kbRenderer");

	// Wait for render thread to become idle
	m_pRenderJob->RequestShutdown();
	while (m_pRenderJob->IsJobFinished() == false) {}
	delete m_pRenderJob;
	m_pRenderJob = nullptr;

	g_pRenderer = nullptr;

	for (int i = 0; i < m_pRenderTargets.size(); i++) {
		m_pRenderTargets[i]->Release();
	}
	m_pRenderTargets.clear();

	Shutdown_Internal();
}

/**
 *	kbRenderer::LoadTexture
 */
void kbRenderer::LoadTexture(const char* name, int index, int width, int height) {
	LoadTexture_Internal(name, index, width, height);
}

/**
 *	kbRenderer::AddRenderObject
 */
void kbRenderer::AddRenderObject(const kbRenderObject& renderObjectToAdd) {
	blk::error_check(m_pCurrentRenderWindow != nullptr, "kbRenderer::AddRenderObject - NULL m_pCurrentRenderWindow");
	blk::error_check(renderObjectToAdd.m_pComponent != nullptr, "kbRenderer::AddRenderObject - NULL component");

	m_RenderObjectList_GameThread.push_back(renderObjectToAdd);
	kbRenderObject& renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = true;
	renderObj.m_bIsRemove = false;
}

/**
 *	kbRenderer::UpdateRenderObject
 */
void kbRenderer::UpdateRenderObject(const kbRenderObject& renderObjectToUpdate) {
	blk::error_check(m_pCurrentRenderWindow != nullptr, "kbRenderer::UpdateRenderObject() - NULL m_pCurrentRenderWindow");
	blk::error_check(renderObjectToUpdate.m_pComponent != nullptr, "kbRenderer::UpdateRenderObject() - NULL component");

	m_RenderObjectList_GameThread.push_back(renderObjectToUpdate);
	kbRenderObject& renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = false;
	renderObj.m_bIsRemove = false;
}

/**
 *	kbRenderer::RemoveRenderObject
 */
void kbRenderer::RemoveRenderObject(const kbRenderObject& renderObjectToRemove) {
	blk::error_check(m_pCurrentRenderWindow != nullptr, "kbRenderer::RemoveRenderObject() - NULL m_pCurrentRenderWindow");
	blk::error_check(renderObjectToRemove.m_pComponent != nullptr, "kbRenderer::RemoveRenderObject - NULL component");

	// Remove duplicates
	for (int i = 0; i < m_RenderObjectList_GameThread.size(); i++) {
		if (m_RenderObjectList_GameThread[i].m_pComponent == renderObjectToRemove.m_pComponent) {
			m_RenderObjectList_GameThread.erase(m_RenderObjectList_GameThread.begin() + i);
			i--;
		}
	}

	m_RenderObjectList_GameThread.push_back(renderObjectToRemove);
	kbRenderObject& renderObj = m_RenderObjectList_GameThread[m_RenderObjectList_GameThread.size() - 1];

	renderObj.m_bIsFirstAdd = false;
	renderObj.m_bIsRemove = true;
}

/**
 *	kbRenderer::DrawDebugText
 */
void kbRenderer::DrawDebugText(const std::string& theString, const float X, const float Y, const float ScreenCharW, const float ScreenCharH, const kbColor& Color) {

	m_DebugStrings_GameThread.push_back(kbTextInfo_t());

	kbTextInfo_t& newTextInfo = m_DebugStrings_GameThread[m_DebugStrings_GameThread.size() - 1];
	newTextInfo.TextInfo = theString;
	newTextInfo.screenX = X;
	newTextInfo.screenY = Y;
	newTextInfo.screenW = ScreenCharW;
	newTextInfo.screenH = ScreenCharH;
	newTextInfo.color = Color;
}

/**
 *	kbRenderer::AddLight
 */
void kbRenderer::AddLight(const kbLightComponent* pLightComponent, const Vec3& pos, const Quat4& orientation) {

	if (m_pCurrentRenderWindow == nullptr) {
		blk::error("kbRenderer::AddLight - nullptr Render Window");
	}

	kbRenderLight newLight;

	newLight.m_pLightComponent = pLightComponent;
	newLight.m_Position = pos;
	newLight.m_Orientation = orientation;
	newLight.m_Radius = pLightComponent->GetRadius();
	newLight.m_Length = pLightComponent->GetLength();

	// If there are empty entries in the splits distance array, a value of FLT_MAX ensures the split won't be selected in the projection shader
	newLight.m_CascadedShadowSplits[0] = FLT_MAX;
	newLight.m_CascadedShadowSplits[1] = FLT_MAX;
	newLight.m_CascadedShadowSplits[2] = FLT_MAX;
	newLight.m_CascadedShadowSplits[3] = FLT_MAX;

	if (pLightComponent->CastsShadow() && pLightComponent->IsA(kbDirectionalLightComponent::GetType())) {
		const kbDirectionalLightComponent* const dirLight = static_cast<const kbDirectionalLightComponent*>(pLightComponent);
		for (int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++) {
			newLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	newLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	newLight.m_bCastsShadow = pLightComponent->CastsShadow();
	newLight.m_bIsFirstAdd = true;
	newLight.m_bIsRemove = false;
	m_LightList_GameThread.push_back(newLight);
}

/**
 *	kbRenderer::UpdateLight
 */
void kbRenderer::UpdateLight(const kbLightComponent* pLightComponent, const Vec3& pos, const Quat4& orientation) {

	if (m_pCurrentRenderWindow == nullptr) {
		blk::error("kbRenderer::UpdateLight - nullptr Render Window");
	}

	for (int i = 0; i < m_LightList_GameThread.size(); i++) {
		if (m_LightList_GameThread[i].m_pLightComponent == pLightComponent) {
			if (m_LightList_GameThread[i].m_bIsRemove == false) {
				m_LightList_GameThread[i].m_Position = pos;
				m_LightList_GameThread[i].m_Orientation = orientation;
				m_LightList_GameThread[i].m_bCastsShadow = pLightComponent->CastsShadow();
				m_LightList_GameThread[i].m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
				m_LightList_GameThread[i].m_Radius = pLightComponent->GetRadius();
				m_LightList_GameThread[i].m_Length = pLightComponent->GetLength();

				memset(&m_LightList_GameThread[i].m_CascadedShadowSplits, 0, sizeof(m_LightList_GameThread[i].m_CascadedShadowSplits));
				if (pLightComponent->IsA(kbDirectionalLightComponent::GetType())) {
					const kbDirectionalLightComponent* const dirLight = static_cast<const kbDirectionalLightComponent*>(pLightComponent);
					for (int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++) {
						m_LightList_GameThread[i].m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
					}
				}
			}
			return;
		}
	}

	kbRenderLight updateLight;
	updateLight.m_pLightComponent = pLightComponent;
	updateLight.m_Position = pos;
	updateLight.m_Orientation = orientation;
	updateLight.m_bIsFirstAdd = false;
	updateLight.m_bIsRemove = false;
	updateLight.m_bCastsShadow = pLightComponent->CastsShadow();
	updateLight.m_Color = pLightComponent->GetColor() * pLightComponent->GetBrightness();
	updateLight.m_Radius = pLightComponent->GetRadius();
	updateLight.m_Length = pLightComponent->GetLength();

	memset(&updateLight.m_CascadedShadowSplits, 0, sizeof(updateLight.m_CascadedShadowSplits));
	if (pLightComponent->IsA(kbDirectionalLightComponent::GetType())) {
		const kbDirectionalLightComponent* const dirLight = static_cast<const kbDirectionalLightComponent*>(pLightComponent);
		for (int i = 0; i < 4 && i < dirLight->GetSplitDistances().size(); i++) {
			updateLight.m_CascadedShadowSplits[i] = dirLight->GetSplitDistances()[i];
		}
	}

	m_LightList_GameThread.push_back(updateLight);
}

/**
 *	kbRenderer::RemoveLight
 */
void kbRenderer::RemoveLight(const kbLightComponent* const pLightComponent) {

	if (m_pCurrentRenderWindow == nullptr) {
		blk::error("kbRenderer::RemoveLight - nullptr Render Window");
	}

	kbRenderLight lightToRemove;
	lightToRemove.m_pLightComponent = pLightComponent;
	lightToRemove.m_bIsRemove = true;
	m_LightList_GameThread.push_back(lightToRemove);
}

/**
 *	kbRenderer::HackClearLight
 */
void kbRenderer::HackClearLight(const kbLightComponent* const pLightComponent) {

	for (int i = 0; i < m_LightList_GameThread.size(); i++) {
		if (m_LightList_GameThread[i].m_pLightComponent == pLightComponent) {
			m_LightList_GameThread.erase(m_LightList_GameThread.begin() + i);
			i--;
		}
	}

}

/**
 *	kbRenderer::AddParticle
 */
void kbRenderer::AddParticle(const kbRenderObject& renderObject) {

	m_ParticleList_GameThread.push_back(renderObject);
	m_ParticleList_GameThread[m_ParticleList_GameThread.size() - 1].m_bIsFirstAdd = true;
	m_ParticleList_GameThread[m_ParticleList_GameThread.size() - 1].m_bIsRemove = false;
}

/**
 *	kbRenderer::RemoveParticle
 */
void kbRenderer::RemoveParticle(const kbRenderObject& renderObject) {

	m_ParticleList_GameThread.push_back(renderObject);
	m_ParticleList_GameThread[m_ParticleList_GameThread.size() - 1].m_bIsFirstAdd = false;
	m_ParticleList_GameThread[m_ParticleList_GameThread.size() - 1].m_bIsRemove = true;
}

/**
 *	kbRenderer::AddLightShafts
 */
void kbRenderer::AddLightShafts(const kbLightShaftsComponent* const pComponent, const Vec3& pos, const Quat4& orientation) {
	kbLightShafts newLightShafts;
	newLightShafts.m_pLightShaftsComponent = pComponent;
	newLightShafts.m_pTexture = pComponent->GetTexture();
	newLightShafts.m_Color = pComponent->GetColor();
	newLightShafts.m_Width = pComponent->GetBaseWidth();
	newLightShafts.m_Height = pComponent->GetBaseHeight();
	newLightShafts.m_NumIterations = pComponent->GetNumIterations();
	newLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	newLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	newLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	newLightShafts.m_Pos = pos;
	newLightShafts.m_Rotation = orientation;
	newLightShafts.m_Operation = ROO_Add;

	for (int i = 0; i < m_LightShafts_GameThread.size(); i++) {
		if (m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent) {
			m_LightShafts_GameThread.erase(m_LightShafts_GameThread.begin());
			break;
		}
	}
	m_LightShafts_GameThread.push_back(newLightShafts);
}

/**
 *	kbRenderer::UpdateLightShafts
 */
void kbRenderer::UpdateLightShafts(const kbLightShaftsComponent* const pComponent, const Vec3& pos, const Quat4& orientation) {
	kbLightShafts updatedLightShafts;
	updatedLightShafts.m_pLightShaftsComponent = pComponent;
	updatedLightShafts.m_pTexture = pComponent->GetTexture();
	updatedLightShafts.m_Color = pComponent->GetColor();
	updatedLightShafts.m_Width = pComponent->GetBaseWidth();
	updatedLightShafts.m_Height = pComponent->GetBaseHeight();
	updatedLightShafts.m_NumIterations = pComponent->GetNumIterations();
	updatedLightShafts.m_IterationWidth = pComponent->GetIterationWidth();
	updatedLightShafts.m_IterationHeight = pComponent->GetIterationHeight();
	updatedLightShafts.m_bIsDirectional = pComponent->IsDirectional();
	updatedLightShafts.m_Pos = pos;
	updatedLightShafts.m_Rotation = orientation;
	updatedLightShafts.m_Operation = ROO_Update;

	for (int i = 0; i < m_LightShafts_GameThread.size(); i++) {
		if (m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent) {
			if (m_LightShafts_GameThread[i].m_Operation == ROO_Remove) {
				return;
			}
		}
	}
	m_LightShafts_GameThread.push_back(updatedLightShafts);
}

/**
 *	kbRenderer::RemoveLightShafts
 */
void kbRenderer::RemoveLightShafts(const kbLightShaftsComponent* const pComponent) {
	kbLightShafts removeLightShafts;
	removeLightShafts.m_pLightShaftsComponent = pComponent;
	removeLightShafts.m_Operation = ROO_Remove;

	for (int i = 0; i < m_LightShafts_GameThread.size(); i++) {
		if (m_LightShafts_GameThread[i].m_pLightShaftsComponent == pComponent) {
			m_LightShafts_GameThread.erase(m_LightShafts_GameThread.begin());
			break;
		}
	}
	m_LightShafts_GameThread.push_back(removeLightShafts);
}

/**
 *	kbRenderer::UpdateFog
 */
void kbRenderer::UpdateFog(const kbColor& color, const float startDistance, const float endDistance) {
	m_FogColor_GameThread = color;
	m_FogStartDistance_GameThread = startDistance;
	m_FogEndDistance_GameThread = endDistance;
}

/**
 *	kbRenderer::RenderSync
 */
void kbRenderer::RenderSync() {

	// Copy requested game thread data over to their corresponding render thread structures
	m_DepthLines_RenderThread = m_DepthLines_GameThread;
	m_DepthLines_GameThread.clear();

	m_NoDepthLines_RenderThread = m_NoDepthLines_GameThread;
	m_NoDepthLines_GameThread.clear();

	m_DebugBillboards = m_DebugBillboards_GameThread;
	m_DebugBillboards_GameThread.clear();

	m_DebugModels = m_DebugModels_GameThread;
	m_DebugModels_GameThread.clear();

	m_DebugStrings = m_DebugStrings_GameThread;
	m_DebugStrings_GameThread.clear();

	m_ScreenSpaceQuads_RenderThread = m_ScreenSpaceQuads_GameThread;
	m_ScreenSpaceQuads_GameThread.clear();

	m_ViewMode = m_ViewMode_GameThread;

	// Add/update render objects
	for (int i = 0; i < m_RenderObjectList_GameThread.size(); i++)
	{
		kbRenderObject* renderObject = nullptr;

		if (m_RenderObjectList_GameThread[i].m_bIsRemove) {
			kbRenderObject* const pRenderObject = m_pCurrentRenderWindow->m_RenderObjectMap[m_RenderObjectList_GameThread[i].m_pComponent];
			m_pCurrentRenderWindow->m_RenderObjectMap.erase(m_RenderObjectList_GameThread[i].m_pComponent);
			delete pRenderObject;
		} else {
			const kbGameComponent* const pComponent = m_RenderObjectList_GameThread[i].m_pComponent;
			blk::error_check(pComponent != nullptr, "kbRenderer::RenderSync() - Adding/updating a render object with a nullptr component");

			if (m_RenderObjectList_GameThread[i].m_bIsFirstAdd == false) {

				// Updating a renderobject 
				auto it = m_pCurrentRenderWindow->m_RenderObjectMap.find(pComponent);
				if (it == m_pCurrentRenderWindow->m_RenderObjectMap.end() || it->second == nullptr) {
					blk::warn("kbRenderer::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist. %s", pComponent->GetOwner()->GetName().c_str());
					return;
				}

				renderObject = it->second;
				*renderObject = m_RenderObjectList_GameThread[i];
				if (pComponent->IsA(kbSkeletalModelComponent::GetType()) && renderObject->m_pModel->NumBones() > 0) {
					const kbSkeletalModelComponent* const skelComp = static_cast<const kbSkeletalModelComponent*>(pComponent);
					renderObject->m_MatrixList = skelComp->GetFinalBoneMatrices();
					renderObject->m_bIsSkinnedModel = true;
				}
			} else {
				// Adding new renderobject
				renderObject = m_pCurrentRenderWindow->m_RenderObjectMap[pComponent];
				blk::warn_check(renderObject == nullptr, "kbRenderer::AddRenderObject() - Model %s already added", m_RenderObjectList_GameThread[i].m_pModel->GetFullName().c_str());

				if (pComponent->IsA(kbSkeletalModelComponent::GetType()) && m_RenderObjectList_GameThread[i].m_pModel->NumBones() > 0) {
					renderObject = new kbRenderObject();
					*renderObject = m_RenderObjectList_GameThread[i];
					renderObject->m_bIsSkinnedModel = true;
					const kbSkeletalModelComponent* const skelComp = static_cast<const kbSkeletalModelComponent*>(pComponent);
					renderObject->m_MatrixList = skelComp->GetFinalBoneMatrices();
				} else {
					renderObject = new kbRenderObject;
					*renderObject = m_RenderObjectList_GameThread[i];
				}

				m_pCurrentRenderWindow->m_RenderObjectMap[m_RenderObjectList_GameThread[i].m_pComponent] = renderObject;
			}
		}
	}
	m_RenderObjectList_GameThread.clear();

	// Light
	for (int i = 0; i < m_LightList_GameThread.size(); i++) {

		if (m_LightList_GameThread[i].m_bIsRemove) {

			kbRenderLight* const pRenderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];
			m_pCurrentRenderWindow->m_RenderLightMap.erase(m_LightList_GameThread[i].m_pLightComponent);
			delete pRenderLight;
		} else {
			kbRenderLight* renderLight = nullptr;

			bool bIsFirstAdd = m_LightList_GameThread[i].m_bIsFirstAdd;
			if (bIsFirstAdd) {
				renderLight = m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent];

				if (renderLight != nullptr) {
					bIsFirstAdd = false;
					blk::error("kbRenderer::AddLight - Warning, adding a render light that already exists");
				} else {
					renderLight = new kbRenderLight;
					m_pCurrentRenderWindow->m_RenderLightMap[m_LightList_GameThread[i].m_pLightComponent] = renderLight;
				}
			} else {

				std::map< const kbLightComponent*, kbRenderLight* >::iterator it = m_pCurrentRenderWindow->m_RenderLightMap.find(m_LightList_GameThread[i].m_pLightComponent);

				if (it == m_pCurrentRenderWindow->m_RenderLightMap.end() || it->second == nullptr) {
					blk::error("kbRenderer::UpdateLight - Error, Updating a RenderObject that doesn't exist");
				} else {
					renderLight = it->second;
				}
			}

			*renderLight = m_LightList_GameThread[i];
		}
	}
	m_LightList_GameThread.clear();

	// Particles
	for (int i = 0; i < m_ParticleList_GameThread.size(); i++) {
		const void* const pComponent = m_ParticleList_GameThread[i].m_pComponent;
		std::map<const void*, kbRenderObject*>& particleMap = m_pCurrentRenderWindow->m_RenderParticleMap;

		if (m_ParticleList_GameThread[i].m_bIsRemove) {
			kbRenderObject* renderParticle = particleMap[pComponent];
			particleMap.erase(pComponent);
			delete renderParticle;
		} else {
			kbRenderObject* renderParticle = nullptr;

			if (m_ParticleList_GameThread[i].m_bIsFirstAdd) {
				renderParticle = particleMap[pComponent];

				if (renderParticle != nullptr) {
					blk::warn("kbRenderer::AddParticle - Adding a particle that already exists");
				} else {
					renderParticle = new kbRenderObject;
					particleMap[pComponent] = renderParticle;
				}
			} else {
				std::map< const void*, kbRenderObject* >::iterator it = particleMap.find(pComponent);
				if (it == particleMap.end() || it->second == nullptr) {
					blk::warn("kbRenderer::UpdateRenderObject - Error, Updating a RenderObject that doesn't exist");
				}

				renderParticle = it->second;
			}

			*renderParticle = m_ParticleList_GameThread[i];
		}
	}
	m_ParticleList_GameThread.clear();

	// Light Shafts
	for (int i = 0; i < m_LightShafts_GameThread.size(); i++) {
		if (m_LightShafts_GameThread[i].m_Operation == ROO_Add) {
			bool bAlreadyExists = false;
			for (int j = 0; j < m_LightShafts_RenderThread.size(); j++) {
				if (m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent) {
					blk::warn("kbRenderer::SetReadyToRender() - Adding light shafts that already exist");
					bAlreadyExists = true;
					break;
				}
			}

			if (bAlreadyExists == false) {
				m_LightShafts_RenderThread.push_back(m_LightShafts_GameThread[i]);
			}
		} else if (m_LightShafts_GameThread[i].m_Operation == ROO_Remove) {
			bool bExists = false;
			for (int j = 0; j < m_LightShafts_RenderThread.size(); j++) {
				if (m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent) {
					m_LightShafts_RenderThread.erase(m_LightShafts_RenderThread.begin() + j);
					bExists = true;
					break;
				}
			}

			if (bExists == false) {
				blk::error("kbRenderer::SetReadyToRender() - Removing light shafts that do not exist");
			}
		} else {
			for (int j = 0; j < m_LightShafts_RenderThread.size(); j++) {
				if (m_LightShafts_RenderThread[j].m_pLightShaftsComponent = m_LightShafts_GameThread[i].m_pLightShaftsComponent) {
					m_LightShafts_RenderThread[j] = m_LightShafts_GameThread[i];
					break;
				}
			}
		}
	}

	m_LightShafts_GameThread.clear();

	for (int iPass = 0; iPass < NUM_RENDER_PASSES; iPass++) {
		std::vector<kbRenderHook*>& passHooks = m_RenderHooks[iPass];
		for (int iHook = 0; iHook < passHooks.size(); iHook++) {
			passHooks[iHook]->RenderSync();
		}
	}

	// Camera
	for (int i = 0; i < m_RenderWindowList.size(); i++) {

		m_RenderWindowList[i]->m_CameraPosition = m_RenderWindowList[i]->m_CameraPosition_GameThread;
		m_RenderWindowList[i]->m_CameraRotation = m_RenderWindowList[i]->m_CameraRotation_GameThread;

		m_RenderWindowList[i]->m_NearPlane_RenderThread = m_RenderWindowList[i]->m_NearPlane_GameThread;
		m_RenderWindowList[i]->m_FarPlane_RenderThread = m_RenderWindowList[i]->m_FarPlane_GameThread;
	}

	// Fog
	m_FogColor_RenderThread = m_FogColor_GameThread;
	m_FogStartDistance_RenderThread = m_FogStartDistance_GameThread;
	m_FogEndDistance_RenderThread = m_FogEndDistance_GameThread;

	m_GlobalModelScale_RenderThread = m_GlobalModelScale_GameThread;
	m_EditorIconScale_RenderThread = m_EditorIconScale_GameThread;

	RenderSync_Internal();
}

/**
 *	kbRenderer::DrawBillboard
 */
void kbRenderer::DrawBillboard(const Vec3& position, const Vec2& size, const int textureIndex, kbShader* const pShader, const int entityId) {
	debugDrawObject_t billboard;
	billboard.m_Position = position;
	billboard.m_Scale.set(size.x, size.y, size.x);
	billboard.m_pShader = pShader;
	billboard.m_TextureIndex = textureIndex;
	billboard.m_EntityId = entityId;

	m_DebugBillboards_GameThread.push_back(billboard);
}

/**
 *	kbRenderer::DrawModel
 */
void kbRenderer::DrawModel(const kbModel* const pModel, const std::vector<kbShaderParamOverrides_t>& materials, const Vec3& position, const Quat4& orientation, const Vec3& scale, const int entityId) {
	debugDrawObject_t model;
	model.m_Position = position;
	model.m_Orientation = orientation;
	model.m_Scale = scale;
	model.m_pModel = pModel;
	model.m_EntityId = entityId;
	model.m_Materials = materials;

	m_DebugModels_GameThread.push_back(model);
}

/**
 *	kbRenderer::DrawScreenSpaceQuad
 */
void kbRenderer::DrawScreenSpaceQuad(const int start_x, const int start_y, const int size_x, const int size_y, const int textureIndex, kbShader* const pShader) {
	ScreenSpaceQuad_t quadToAdd;
	quadToAdd.m_Pos.x = start_x;
	quadToAdd.m_Pos.y = start_y;
	quadToAdd.m_Size.x = size_x;
	quadToAdd.m_Size.y = size_y;
	quadToAdd.m_pShader = pShader;
	quadToAdd.m_TextureIndex = textureIndex;

	m_ScreenSpaceQuads_GameThread.push_back(quadToAdd);
}

#define AddVertDepthTest( vert ) drawVert.position = vert; m_DepthLines_GameThread.push_back( drawVert );
#define AddVertNoDepthTest( vert ) drawVert.position = vert; m_NoDepthLines_GameThread.push_back( drawVert );

/**
 *	kbRenderer::DrawLine
 */
void kbRenderer::DrawLine(const Vec3& start, const Vec3& end, const kbColor& color, const bool bDepthTest) {

	/*if ( m_DebugLines_GameThread.size() >= m_DebugLines_GameThread.capacity() - 2 ) {
		return;
	}*/

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor(color);

	if (bDepthTest) {
		AddVertDepthTest(start);
		AddVertDepthTest(end);
	} else {
		AddVertNoDepthTest(start);
		AddVertNoDepthTest(end);
	}
}

/**
 *	kbRenderer::DrawBox
 */
void kbRenderer::DrawBox(const kbBounds& bounds, const kbColor& color, const bool bDepthTest) {

	const Vec3 maxVert = bounds.Max();
	const Vec3 minVert = bounds.Min();

	const Vec3 LTF(minVert.x, maxVert.y, maxVert.z);
	const Vec3 RTF(maxVert.x, maxVert.y, maxVert.z);
	const Vec3 RBF(maxVert.x, minVert.y, maxVert.z);
	const Vec3 LBF(minVert.x, minVert.y, maxVert.z);
	const Vec3 LTB(minVert.x, maxVert.y, minVert.z);
	const Vec3 RTB(maxVert.x, maxVert.y, minVert.z);
	const Vec3 RBB(maxVert.x, minVert.y, minVert.z);
	const Vec3 LBB(minVert.x, minVert.y, minVert.z);

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor(color);

	if (bDepthTest) {
		AddVertDepthTest(LTF); AddVertDepthTest(RTF);
		AddVertDepthTest(RTF); AddVertDepthTest(RBF);
		AddVertDepthTest(RBF); AddVertDepthTest(LBF);
		AddVertDepthTest(LBF); AddVertDepthTest(LTF);

		AddVertDepthTest(LTB); AddVertDepthTest(RTB);
		AddVertDepthTest(RTB); AddVertDepthTest(RBB);
		AddVertDepthTest(RBB); AddVertDepthTest(LBB);
		AddVertDepthTest(LBB); AddVertDepthTest(LTB);

		AddVertDepthTest(LTF); AddVertDepthTest(LTB);
		AddVertDepthTest(RTF); AddVertDepthTest(RTB);
		AddVertDepthTest(LBF); AddVertDepthTest(LBB);
		AddVertDepthTest(RBF); AddVertDepthTest(RBB);
	} else {
		AddVertNoDepthTest(LTF); AddVertNoDepthTest(RTF);
		AddVertNoDepthTest(RTF); AddVertNoDepthTest(RBF);
		AddVertNoDepthTest(RBF); AddVertNoDepthTest(LBF);
		AddVertNoDepthTest(LBF); AddVertNoDepthTest(LTF);

		AddVertNoDepthTest(LTB); AddVertNoDepthTest(RTB);
		AddVertNoDepthTest(RTB); AddVertNoDepthTest(RBB);
		AddVertNoDepthTest(RBB); AddVertNoDepthTest(LBB);
		AddVertNoDepthTest(LBB); AddVertNoDepthTest(LTB);

		AddVertNoDepthTest(LTF); AddVertNoDepthTest(LTB);
		AddVertNoDepthTest(RTF); AddVertNoDepthTest(RTB);
		AddVertNoDepthTest(LBF); AddVertNoDepthTest(LBB);
		AddVertNoDepthTest(RBF); AddVertNoDepthTest(RBB);
	}
}

/**
 *	kbRenderer::DrawSphere
 */
void kbRenderer::DrawSphere(const Vec3& origin, const float radius, const int InNumSegments, const kbColor& color) {
	const int numSegments = max(InNumSegments, 4);
	const float angleInc = 2.0f * kbPI / (float)numSegments;
	float latitude = angleInc;
	float curSin = 0, curCos = 1.0f;
	//float cosX, sinX;
	Vec3 pt1, pt2, pt3, pt4;

	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor(color);

	for (int curYSeg = 0; curYSeg < numSegments; curYSeg++) {
		const float nextSin = sin(latitude);
		const float nextCos = cos(latitude);

		pt1 = Vec3(curSin, curCos, 0.0f) * radius + origin;
		pt3 = Vec3(nextSin, nextCos, 0.0f) * radius + origin;
		float longitude = angleInc;
		for (int curXSeg = 0; curXSeg < numSegments; curXSeg++) {
			float sinX = sin(longitude);
			float cosX = cos(longitude);

			pt2 = Vec3(cosX * curSin, curCos, sinX * curSin) * radius + origin;
			pt4 = Vec3(cosX * nextSin, nextCos, sinX * nextSin) * radius + origin;
			AddVertDepthTest(pt1); AddVertDepthTest(pt2);
			AddVertDepthTest(pt1); AddVertDepthTest(pt3);
			pt1 = pt2;
			pt3 = pt4;
			longitude += angleInc;
		}

		curSin = nextSin;
		curCos = nextCos;
		latitude += angleInc;
	}
}

/**
 *	kbRenderer::DrawPreTransformedLine
 */
void kbRenderer::DrawPreTransformedLine(const std::vector<Vec3>& vertList, const kbColor& color) {
	vertexLayout drawVert;

	drawVert.Clear();
	drawVert.SetColor(color);

	for (int i = 0; i < vertList.size(); i++) {
		drawVert.position = vertList[i];
		m_DebugPreTransformedLines.push_back(drawVert);
	}
}

/**
 *	kbRenderer::RT_GetRenderTexture
 */
kbRenderTexture* kbRenderer::RT_GetRenderTexture(const int width, const int height, const eTextureFormat texFormat, const bool bRequiresCPUAccess) {

	for (int i = NUM_RESERVED_RENDER_TARGETS; i < m_pRenderTargets.size(); i++) {
		kbRenderTexture* const pRT = m_pRenderTargets[i];
		if (pRT->m_bInUse == false && pRT->GetWidth() == width && pRT->GetHeight() == height && pRT->GetTextureFormat() == texFormat) {
			pRT->m_bInUse = true;
			return pRT;
		}
	}

	kbRenderTexture* const pRenderTexture = GetRenderTexture_Internal(width, height, texFormat, bRequiresCPUAccess);
	pRenderTexture->m_bInUse = true;
	return pRenderTexture;
}

/**
 *	kbRenderer::RT_ReturnRenderTexture
 */
void kbRenderer::RT_ReturnRenderTexture(kbRenderTexture* const pRenderTexture) {
	ReturnRenderTexture_Internal(pRenderTexture);
	pRenderTexture->m_bInUse = false;
}

/**
 *	kbRenderer::RegisterRenderHook
 */
void kbRenderer::RegisterRenderHook(kbRenderHook* const pRenderHook) {
	blk::error_check(pRenderHook != nullptr, "kbRenderer::RegisterRenderHook() - NULL render hook");

	m_RenderHooks[(int)pRenderHook->m_RenderPass].push_back(pRenderHook);
}

/**
 *	kbRenderer::UnregisterRenderHook
 */
void kbRenderer::UnregisterRenderHook(kbRenderHook* const pRenderHook) {
	blk::error_check(pRenderHook != nullptr, "kbRenderer::UnregisterRenderHook() - NULL render hook");

	blk::std_remove_swap(m_RenderHooks[(int)pRenderHook->m_RenderPass], pRenderHook);

}

/**
 *	kbRenderHook::kbRenderHook
 */
kbRenderHook::kbRenderHook(const ERenderPass renderPass) :
	m_RenderPass(renderPass) {
}

/**
 *	kbRenderHook::~kbRenderHook
 */
kbRenderHook::~kbRenderHook() {

}