/// Renderer_Vk.h	
///
/// 2025 blk 1.0

#pragma once

#include "renderer.h"
#include <vulkan/vulkan.h>

#define check_vk(res) \
	if (res != VK_SUCCESS) { \
		blk::error("Fatal : VkResult is \" %d \" in %s at line %d",res, __FILE__, __LINE__); \
	}

///	Renderer_Vk
class Renderer_Vk : public Renderer {
public:
	virtual ~Renderer_Vk();

	virtual void render() override;

protected:
	virtual void initialize_internal(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;
	virtual void shut_down_internal() override;

private:
	virtual RenderPipeline* create_pipeline(const std::string& friendly_name, const std::string& path) override;
	virtual RenderBuffer* create_render_buffer_internal() override;

	VkDevice m_device;
	VkInstance m_instance = VK_NULL_HANDLE;
	std::vector<std::string> m_supported_extensions;

	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
};