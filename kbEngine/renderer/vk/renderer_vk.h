/// renderer_vk.h	
///
/// 2025 kbEngine 2.0

#pragma once

#include "renderer.h"
#include <vulkan/vulkan.h>

///	Renderer_VK
class Renderer_VK : public Renderer {
public:
	virtual ~Renderer_VK();

	virtual void shut_down() override;

	virtual void render() override;

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;

private:
	virtual RenderPipeline* create_pipeline(const std::wstring& path) override;
	virtual RenderBuffer* create_render_buffer_internal() override;

	VkInstance instance = VK_NULL_HANDLE;
	std::vector<std::string> m_supported_extensions;
};