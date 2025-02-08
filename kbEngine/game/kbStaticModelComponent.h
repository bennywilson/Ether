/// render_component.h
///
/// 2016-2025 blk 1.0

#pragma once

#include "render_component.h"

/// RenderComponent
class kbStaticModelComponent : public RenderComponent {
	KB_DECLARE_COMPONENT(kbStaticModelComponent, RenderComponent);

public:
	virtual ~kbStaticModelComponent();

	void SetModel(class kbModel* pModel) { m_pModel = pModel; }
	const kbModel* GetModel() const { return m_pModel; }

	virtual void EditorChange(const std::string& propertyName);

protected:
	virtual void SetEnable_Internal(const bool isEnabled) override;
	virtual void Update_Internal(const float DeltaTime) override;

private:
	class kbModel* m_pModel;
};