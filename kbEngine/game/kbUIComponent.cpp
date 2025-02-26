/// kbUIComponent.cpp
///
/// 2019-2025 blk 1.0

#include "blk_core.h"
#include "blk_containers.h"
#include "Matrix.h"
#include "kbGameEntityHeader.h"
#include "kbUIComponent.h"
#include "kbRenderer.h"
#include "kbInputManager.h"

kbGameEntity& GetUIGameEntity() {
	static kbGameEntity m_GameEnt;
	return m_GameEnt;
}

/// kbUIComponent::Constructor
void kbUIComponent::Constructor() {
	m_AuthoredWidth = 128;
	m_AuthoredHeight = 128;
	m_NormalizedAnchorPt.set(0.05f, 0.05f, 0.0f);
	m_UIToScreenSizeRatio.set(0.1f, 0.0f, 0.0f);
	m_NormalizedScreenSize.set(0.f, 0.0f, 0.0f);

	m_pStaticRenderComponent = nullptr;
}

/// kbUIComponent::~kbUIComponent
kbUIComponent::~kbUIComponent() {
	m_AuthoredWidth = 128;
	m_AuthoredHeight = 128;
	m_NormalizedAnchorPt.set(0.05f, 0.05f, 0.0f);
	m_UIToScreenSizeRatio.set(0.1f, 0.0f, 0.0f);
}

/// kbUIComponent::RegisterEventListener
void kbUIComponent::RegisterEventListener(IUIWidgetListener* const pListener) {
	m_EventListeners.push_back(pListener);
}

/// kbUIComponent::UnregisterEventListener
void kbUIComponent::UnregisterEventListener(IUIWidgetListener* const pListener) {
	blk::std_remove_swap(m_EventListeners, pListener);
}

/// kbUIComponent::SetMaterialParamVector
void kbUIComponent::set_material_param_vec4(const std::string& paramName, const Vec4& paramValue) {
	blk::error_check(m_pStaticRenderComponent != nullptr, "bUIComponent::set_material_param_vec4() - m_pStaticRenderComponent is NULL");

	m_pStaticRenderComponent->set_material_param_vec4(0, paramName, paramValue);
}

/// kbUIComponent::SetMaterialParamTexture
void kbUIComponent::set_material_param_texture(const std::string& paramName, kbTexture* const pTexture) {
	blk::error_check(m_pStaticRenderComponent != nullptr, "bUIComponent::set_material_param_texture() - m_pStaticRenderComponent is NULL");

	m_pStaticRenderComponent->set_material_param_texture(0, paramName, pTexture);
}

/// kbUIComponent::FireEvent
void kbUIComponent::FireEvent(const kbInput_t* const pInput) {

	for (int i = 0; i < m_EventListeners.size(); i++) {
		m_EventListeners[i]->WidgetEventCB(nullptr, pInput);
	}
}

/// kbUIComponent::EditorChange
void kbUIComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);
	FindStaticRenderComponent();
	RefreshMaterial();
}

/// kbUIComponent::enable_internal
void kbUIComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	if (bEnable) {
		FindStaticRenderComponent();

		if (m_pStaticRenderComponent != nullptr) {
			m_pStaticRenderComponent->Enable(true);
		}
		RefreshMaterial();

		g_pInputManager->RegisterInputListener(this);
	} else {

		if (m_pStaticRenderComponent != nullptr) {
			m_pStaticRenderComponent->Enable(false);
			m_pStaticRenderComponent = nullptr;
		}

		g_pInputManager->UnregisterInputListener(this);
	}
}

/// kbUIComponent:FindStaticRenderComponent
void kbUIComponent::FindStaticRenderComponent() {
	m_pStaticRenderComponent = GetOwner()->GetComponent<RenderComponent>();
}

/// kbUIComponent:RefreshMaterial

void kbUIComponent::RefreshMaterial() {

	FindStaticRenderComponent();
	if (m_pStaticRenderComponent == nullptr) {
		return;
	}

	const float ScreenPixelWidth = (float)g_pRenderer->GetBackBufferWidth();
	const float ScreenPixelHeight = (float)g_pRenderer->GetBackBufferHeight();

	const float aspectRatio = (float)GetAuthoredWidth() / (float)GetAuthoredHeight();
	m_NormalizedScreenSize.x = GetUIToScreenSizeRatio().x;
	const float screenWidthPixel = m_NormalizedScreenSize.x * ScreenPixelWidth;
	const float screenHeightPixel = screenWidthPixel / aspectRatio;
	m_NormalizedScreenSize.y = screenHeightPixel / ScreenPixelHeight;
	m_NormalizedScreenSize.z = 1.0f;

	//	blk::log( "%f %f %f %f", m_NormalizedScreenSize.x, m_NormalizedScreenSize.y,m_NormalizedAnchorPt.x - m_NormalizedScreenSize.x * 0.5f,m_NormalizedAnchorPt.y - m_NormalizedScreenSize.y * 0.5f);
	static kbString normalizedScreenSize_Anchor("normalizedScreenSize_Anchor");

	const Vec4 sizeAndPos = Vec4(m_NormalizedScreenSize.x,
		m_NormalizedScreenSize.y,
		m_NormalizedAnchorPt.x + m_NormalizedScreenSize.x * 0.5f,		// Upper left corner to anchor
		m_NormalizedAnchorPt.y + m_NormalizedScreenSize.y * 0.5f);		// Upper left corner to anchor

	m_pStaticRenderComponent->set_material_param_vec4(0, normalizedScreenSize_Anchor.stl_str(), sizeAndPos);
}


/// kbUIWidgetComponent::Constructor
void kbUIWidgetComponent::Constructor() {

	m_StartingPosition.set(0.0f, 0.0f, 0.0f);
	m_StartingSize.set(0.5f, 0.5f, 1.0f);

	m_Anchor = kbUIWidgetComponent::MiddleLeft;
	m_AxisLock = kbUIWidgetComponent::LockAll;

	m_RelativePosition.set(0.0f, 0.0f, 0.0f);
	m_RelativeSize.set(0.5f, 0.5f, 1.0f);

	m_AbsolutePosition.set(0.0f, 0.0f, 0.0f);
	m_AbsoluteSize.set(0.5f, 0.5f, 1.0f);

	m_model = nullptr;

	m_bHasFocus = false;
}

/// kbUIWidgetComponent::RegisterEventListener
void kbUIWidgetComponent::RegisterEventListener(IUIWidgetListener* const pListener) {
	m_EventListeners.push_back(pListener);
}

/// kbUIWidgetComponent::UnregisterEventListener
void kbUIWidgetComponent::UnregisterEventListener(IUIWidgetListener* const pListener) {
	blk::std_remove_swap(m_EventListeners, pListener);
}

/// kbUIWidgetComponent::SetAdditiveTextureFactor
void kbUIWidgetComponent::SetAdditiveTextureFactor(const float factor) {

	static const kbString additiveTextureParams("additiveTextureParams");
	m_model->set_material_param_vec4(0, additiveTextureParams.stl_str(), Vec4(factor, 0.0f, 0.0f, 0.0f));
}

/// kbUIWidgetComponent::FireEvent
void kbUIWidgetComponent::FireEvent(const kbInput_t* const pInput) {

	for (int i = 0; i < m_EventListeners.size(); i++) {
		m_EventListeners[i]->WidgetEventCB(this, pInput);
	}
}

/// kbUIWidgetComponent::EditorChange
void kbUIWidgetComponent::editor_change(const std::string& propertyName) {

	Super::editor_change(propertyName);

}

/// kbUIWidgetComponent::InputCB
void kbUIWidgetComponent::InputCB(const kbInput_t& input) {

}

/// kbUIComponent::SetFocus
void kbUIWidgetComponent::SetFocus(const bool bHasFocus) {
	m_bHasFocus = bHasFocus;
}

/// kbUIWidgetComponent::SetRelativePosition
void kbUIWidgetComponent::SetRelativePosition(const Vec3& newPos) {
	m_RelativePosition = newPos;
	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;

	for (size_t i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].Recalculate(this, false);
	}
}

/// kbUIWidgetComponent::SetRelativeSize
void kbUIWidgetComponent::SetRelativeSize(const Vec3& newSize) {

	m_RelativeSize = newSize;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for (size_t i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].Recalculate(this, false);
	}
}

/// kbUIWidgetComponent::RecalculateOld
void kbUIWidgetComponent::RecalculateOld(const kbUIComponent* const pParent, const bool bFull) {
	blk::error_check(pParent != nullptr, "kbUIWidgetComponent::UpdateFromParent() - null parent");

	/*	if ( m_model != nullptr && pParent != nullptr && pParent->GetStaticRenderComponent() != nullptr ) {
			blk::log( "Setting render oreder bias to %f", pParent->GetStaticRenderComponent()->render_order_bias() - 1.0f );
			m_model->set_render_order_bias( pParent->GetStaticRenderComponent()->render_order_bias() - 1.0f );
		}*/

	m_CachedParentPosition = pParent->GetNormalizedAnchorPt();
	m_CachedParentSize = pParent->GetNormalizedScreenSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	set_render_order_bias(pParent->GetStaticRenderComponent()->render_order_bias() - 1.0f);

	for (int i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].RecalculateOld(pParent, bFull);
	}
}

/// kbUIWidgetComponent::Recalculate
void kbUIWidgetComponent::Recalculate(const kbUIWidgetComponent* const pParent, const bool bFull) {

	if (pParent != nullptr) {
		m_CachedParentPosition = pParent->GetAbsolutePosition();
		m_CachedParentSize = pParent->GetAbsoluteSize();
		set_render_order_bias(pParent->render_order_bias() - 1.0f);
	} else {
		m_CachedParentPosition = Vec3::zero;
		m_CachedParentSize.set(1.0f, 1.0f, 1.0f);
		set_render_order_bias(0.0f);
	}

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for (int i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].Recalculate(this, bFull);
	}
}

/// kbUIWidgetComponent::SetRenderOrderBias
void kbUIWidgetComponent::set_render_order_bias(const float bias) {

	if (m_model != nullptr) {
		m_model->set_render_order_bias(bias);
	}
}

/// kbUIWidgetComponent::GetRenderOrderBias
float kbUIWidgetComponent::render_order_bias() const {

	if (m_model == nullptr) {
		return 0.0f;
	}

	return m_model->render_order_bias();
}

/// kbUIWidgetComponent::GetBaseTextureDimensions
Vec2i kbUIWidgetComponent::GetBaseTextureDimensions() const {
	Vec2i retDim(-1, -1);
	if (m_model == nullptr) {
		return retDim;
	}

	const kbShaderParamComponent* const pComp = m_model->shader_param_component(0, kbString("baseTexture"));
	if (pComp == nullptr || pComp->texture() == nullptr) {
		return retDim;
	}

	retDim.x = pComp->texture()->width();
	retDim.y = pComp->texture()->height();
	return retDim;
}

/// kbUIWidgetComponent::enable_internal
void kbUIWidgetComponent::enable_internal(const bool bEnable) {
	Super::enable_internal(bEnable);

	static kbModel* pUnitQuad = nullptr;
	if (pUnitQuad == nullptr) {
		pUnitQuad = (kbModel*)g_ResourceManager.GetResource("../../kbEngine/assets/Models/UnitQuad.ms3d", true, true);
	}

	if (GetOwner() == nullptr) {
		return;
	}

	if (bEnable) {

		m_RelativePosition = m_StartingPosition;
		m_RelativeSize = m_StartingSize;

		if (m_model == nullptr) {
			m_model = new kbStaticModelComponent();
			GetUIGameEntity().AddComponent(m_model);
		}

		m_model->set_model(pUnitQuad);
		m_model->set_materials(m_Materials);
		m_model->set_render_pass(RP_UI);
		m_model->Enable(false);
		m_model->Enable(true);

		for (size_t i = 0; i < m_ChildWidgets.size(); i++) {
			GetUIGameEntity().AddComponent(&m_ChildWidgets[i]);		// Note these children are responsible for removing themselves when disabled (see code block below)
			m_ChildWidgets[i].Enable(false);
			m_ChildWidgets[i].Enable(true);
		}

		g_pInputManager->RegisterInputListener(this);

		if (GetOwner() != nullptr) {
			Recalculate(nullptr, true);
		}

	} else {
		if (m_model != nullptr) {
			m_model->Enable(false);
		}

		for (size_t i = 0; i < m_ChildWidgets.size(); i++) {
			m_ChildWidgets[i].Enable(false);
		}

		GetUIGameEntity().RemoveComponent(this);
		GetUIGameEntity().RemoveComponent(m_model);

		g_pInputManager->UnregisterInputListener(this);
	}
}

/// kbUIWidgetComponent::update_internal
void kbUIWidgetComponent::update_internal(const float dt) {
	Super::update_internal(dt);

	if (m_model == nullptr) {
		return;
	}

	static const kbString normalizedScreenSize_Anchor("normalizedScreenSize_Anchor");

	Vec3 parentEnd = Vec3::one;
	Vec3 widgetAbsPos = m_AbsolutePosition;
	Vec3 widgetAbsSize = m_AbsoluteSize;
	//	float renderOrderBias = 0.0f;

	f32 aspectRatio = 1.0f;
	const kbShaderParamComponent* const pComp = m_model->shader_param_component(0, kbString("baseTexture"));
	if (pComp != nullptr) {
		const kbTexture* const pTex = pComp->texture();
		if (pTex != nullptr) {
			aspectRatio = (f32)pTex->width() / (f32)pTex->height();
		}
	}

	const f32 BackBufferWidth = (f32)g_pRenderer->GetBackBufferWidth();
	const f32 BackBufferHeight = (f32)g_pRenderer->GetBackBufferHeight();

	if (m_AxisLock == LockYAxis) {
		const f32 widgetPixelHeight = widgetAbsSize.y * BackBufferHeight;
		const f32 widgetPixelWidth = widgetPixelHeight * aspectRatio;
		widgetAbsSize.x = widgetPixelWidth / BackBufferWidth;
	}

	if (m_Anchor == kbUIWidgetComponent::MiddleRight) {
		widgetAbsPos.x -= widgetAbsSize.x;
	}

	m_model->set_material_param_vec4(0, normalizedScreenSize_Anchor.stl_str(),
		Vec4(widgetAbsSize.x,
			widgetAbsSize.y,
			widgetAbsPos.x + widgetAbsSize.x * 0.5f,
			widgetAbsPos.y + widgetAbsSize.y * 0.5f));

	m_model->refresh_materials(true);

	for (size_t i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].update_internal(dt);
	}

	if (HasFocus()) {
		const kbInput_t& input = g_pInputManager->get_input();
		if (input.GamepadButtonStates[12].m_Action == kbInput_t::KA_JustPressed || input.WasNonCharKeyJustPressed(kbInput_t::Return)) {
			FireEvent(&input);
		}
	}
}

/// kbUISlider::Constructor
void kbUISlider::Constructor() {
	m_SliderBoundsMin.set(0.0f, 0.0f, 0.0f);
	m_SliderBoundsMax.set(1.0f, 1.0f, 1.0f);

	m_CalculatedSliderBoundsMin.set(0.0f, 0.0f, 0.0f);
	m_CalculatedSliderBoundsMax.set(1.0f, 1.0f, 1.0f);
}

/// kbUISlider::enable_internal
void kbUISlider::enable_internal(const bool bEnable) {

	Super::enable_internal(bEnable);
}

/// kbUISlider::RecalculateOld
void kbUISlider::RecalculateOld(const kbUIComponent* const pParent, const bool bFull) {

	blk::error_check(pParent != nullptr, "kbUIWidgetComponent::UpdateFromParent() - null parent");

	if (m_model != nullptr && pParent != nullptr && pParent->GetStaticRenderComponent() != nullptr) {

		m_model->set_render_order_bias(pParent->GetStaticRenderComponent()->render_order_bias() - 1.0f);
	}

	m_CachedParentPosition = pParent->GetNormalizedAnchorPt();
	m_CachedParentSize = pParent->GetNormalizedScreenSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	set_render_order_bias(pParent->GetStaticRenderComponent()->render_order_bias() - 1.0f);

	for (int i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].RecalculateOld(pParent, bFull);

		m_ChildWidgets[i].set_render_order_bias(pParent->GetStaticRenderComponent()->render_order_bias() - (1.0f + (float)i));
	}

	const float spaceBetweenLabelAndSlider = 0.05f;

	if (bFull) {
		Vec3 pos = m_RelativePosition;
		pos.x = m_RelativePosition.x + m_RelativeSize.x + spaceBetweenLabelAndSlider;
		m_ChildWidgets[1].SetRelativePosition(pos);
	} else {
		Vec3 pos = m_ChildWidgets[0].GetRelativePosition();
		pos.y = m_RelativePosition.y;

		pos = m_ChildWidgets[1].GetRelativePosition();
		pos.y = m_RelativePosition.y;
		m_ChildWidgets[1].SetRelativePosition(pos);
	}

	if (m_ChildWidgets.size() < 2) {
		m_CalculatedSliderBoundsMin.set(0.0f, 0.0f, 0.0f);
		m_CalculatedSliderBoundsMax.set(0.0f, 0.0f, 0.0f);
	} else {
		m_CalculatedSliderBoundsMin = GetRelativePosition() + spaceBetweenLabelAndSlider;
		m_CalculatedSliderBoundsMax = m_CalculatedSliderBoundsMin + m_ChildWidgets[0].GetRelativeSize() * 0.9f;	// Hack

		m_ChildWidgets[0].SetRelativePosition(GetRelativePosition() + Vec3(spaceBetweenLabelAndSlider, 0.0f, 0.0f));

		if (bFull) {
			m_ChildWidgets[1].SetRelativePosition(GetRelativePosition() + Vec3(GetRelativeSize().x + 0.05f, 0.0f, 0.0f));
		}
	}
}

/// kbUISlider::Recalculate
void kbUISlider::Recalculate(const kbUIWidgetComponent* const pParent, const bool bFull) {

	if (pParent == nullptr) {
		return;
	}

	// blk::error_check( pParent != nullptr, "kbUIWidgetComponent::UpdateFromParent() - null parent" );

	if (m_model != nullptr && pParent != nullptr && pParent->GetStaticModel() != nullptr) {
		m_model->set_render_order_bias(pParent->GetStaticModel()->render_order_bias() - 1.0f);
		blk::log("Slider: Setting render oreder bias to %f", pParent->GetStaticModel()->render_order_bias() - 1.0f);

	}

	m_CachedParentPosition = pParent->GetAbsolutePosition();
	m_CachedParentSize = pParent->GetAbsoluteSize();

	m_AbsolutePosition = m_CachedParentPosition + m_CachedParentSize * m_RelativePosition;
	m_AbsoluteSize = m_CachedParentSize * m_RelativeSize;

	for (int i = 0; i < m_ChildWidgets.size(); i++) {
		m_ChildWidgets[i].Recalculate(pParent, bFull);
	}

	if (bFull) {
		Vec3 pos = m_RelativePosition;
		pos.x = m_RelativePosition.x + m_RelativeSize.x + 0.05f;

		m_ChildWidgets[0].SetRelativePosition(pos);
		m_ChildWidgets[1].SetRelativePosition(pos);
	} else {
		Vec3 pos = m_ChildWidgets[0].GetRelativePosition();
		pos.y = m_RelativePosition.y;

		pos = m_ChildWidgets[1].GetRelativePosition();
		pos.y = m_RelativePosition.y;
		m_ChildWidgets[1].SetRelativePosition(pos);
	}

	if (m_ChildWidgets.size() < 2) {
		m_CalculatedSliderBoundsMin.set(0.0f, 0.0f, 0.0f);
		m_CalculatedSliderBoundsMax.set(0.0f, 0.0f, 0.0f);
	} else {
		m_CalculatedSliderBoundsMin = GetRelativePosition() + GetRelativeSize() + 0.05f;
		m_CalculatedSliderBoundsMax = m_CalculatedSliderBoundsMin + m_ChildWidgets[0].GetRelativeSize();

		m_ChildWidgets[0].SetRelativePosition(GetRelativePosition() + Vec3(GetRelativeSize().x + 0.05f, 0.0f, 0.0f));

		if (bFull) {
			m_ChildWidgets[1].SetRelativePosition(GetRelativePosition() + Vec3(GetRelativeSize().x + 0.05f, 0.0f, 0.0f));
		}
	}
}

/// kbUISlider::update_internal
void kbUISlider::update_internal(const float dt) {

	Super::update_internal(dt);

	if (HasFocus()) {
		if (m_ChildWidgets.size() > 1) {
			Vec3 curPos = m_ChildWidgets[1].GetRelativePosition();
			bool bMove = 0.0f;

			bool bFireEvent = false;
			const kbInput_t& input = g_pInputManager->get_input();
			if (input.IsArrowPressedOrDown(kbInput_t::Left) || input.IsKeyPressedOrDown('A') || input.m_LeftStick.x < -0.5f) {
				curPos.x -= 0.01f;
				bFireEvent = true;
			}

			if (input.IsArrowPressedOrDown(kbInput_t::Right) || input.IsKeyPressedOrDown('D') || input.m_LeftStick.x > 0.5f) {
				curPos.x += 0.01f;
				bFireEvent = true;
			}

			curPos.x = kbClamp(curPos.x, m_CalculatedSliderBoundsMin.x, m_CalculatedSliderBoundsMax.x);
			m_ChildWidgets[1].SetRelativePosition(curPos);

			if (bFireEvent) {
				FireEvent();
			}
		}
	}
}


/// kbUISlider::GetNormalizedValue
float kbUISlider::GetNormalizedValue() {

	if (m_ChildWidgets.size() < 2) {
		return 0.0f;
	}

	const float sliderPos = m_ChildWidgets[1].GetRelativePosition().x;
	return (sliderPos - m_CalculatedSliderBoundsMin.x) / (m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x);
}


/// kbUISlider::SetNormalizedValue
void kbUISlider::SetNormalizedValue(const float newValue) {

	if (m_ChildWidgets.size() < 2) {
		return;
	}


	Vec3 relativePos = m_ChildWidgets[1].GetRelativePosition();
	relativePos.x = m_CalculatedSliderBoundsMin.x + (m_CalculatedSliderBoundsMax.x - m_CalculatedSliderBoundsMin.x) * newValue;

	m_ChildWidgets[1].SetRelativePosition(relativePos);
}
