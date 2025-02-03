/// renderer_vk.h	
///
/// 2025 kbEngine 2.0

#pragma once

#include "renderer.h"

///	RendererVk
class RendererVk : public Renderer {
public:
	virtual ~RendererVk();

	virtual void shut_down() override;

	virtual void render() override;

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;
};