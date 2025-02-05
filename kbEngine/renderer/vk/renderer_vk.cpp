/// renderer_vk.cpp
///
/// 2025 kbEngine 2.0

#include "kbCore.h"
#include "renderer_vk.h"
#include "vk_defs.h"

/// Renderer_VK::~Renderer_VK
Renderer_VK::~Renderer_VK() { }

/// Renderer_VK::shut_down
void Renderer_VK::shut_down() { }

/// Renderer_VK::initialize
void Renderer_VK::initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) {
	// Get supported extensions
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount > 0) {
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
			for (VkExtensionProperties& extension : extensions) {
				m_supported_extensions.push_back(extension.extensionName);
			}
		}
	}

	// Enabled requested instance extensions
	std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "blk Renderer";
	appInfo.pEngineName = "blk Renderer";
	appInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
	if (true) {
		debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugUtilsMessengerCI.pfnUserCallback = nullptr;
		debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
		//instanceCreateInfo.pNext = &debugUtilsMessengerCI;
	}

	// Enable the debug utils extension if available (e.g. when debugging tools are present)
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();


	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	{
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties& layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		} else {
			//	std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

	//  Instance
	check_vk(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

	// Pipeline
/*	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	check_vk(vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));*/
}

/// Renderer_VK::create_pipeline
RenderPipeline* Renderer_VK::create_pipeline(const std::wstring& path) {
	RenderPipeline_VK* const pipe = new RenderPipeline_VK();
	return pipe;
}

/// Renderer_VK::create_render_buffer_internal
RenderBuffer* Renderer_VK::create_render_buffer_internal() {
	RenderBuffer_VK* const buffer = new RenderBuffer_VK();
	return buffer;
}

/// Renderer_VK::render
void Renderer_VK::render() { }

