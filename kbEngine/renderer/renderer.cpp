/// renderer.cpp
///
/// 2025 kbEngine 2.0

#include "kbCore.h"
#include "renderer.h"

Renderer* g_renderer = nullptr;

/// Renderer::Renderer
Renderer::Renderer() {
	g_renderer = this;
}

/// Renderer::~Renderer
Renderer::~Renderer() {
	shut_down();
}

/// Renderer::initialize
void Renderer::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {
	m_frame_width = frame_width;
	m_frame_height = frame_height;
}

/// Renderer::shut_down
void Renderer::shut_down() {
	for (auto& pipe : m_pipelines) {
		delete pipe.second;
	}
	m_pipelines.clear();

	for (size_t i = 0; i < m_render_buffers.size(); i++) {
		m_render_buffers[i]->release();
		delete m_render_buffers[i];
	}
	m_render_buffers.clear();
}

/// Renderer::create_render_buffer
RenderBuffer* Renderer::create_render_buffer() {
	RenderBuffer* const buffer = create_render_buffer_internal();
	m_render_buffers.push_back(buffer);
	return buffer;
}

/// Renderer::load_pipeline
RenderPipeline* Renderer::load_pipeline(const std::string& friendly_name, const std::wstring& path) {
	RenderPipeline* const new_pipeline = create_pipeline(path);
	if (new_pipeline == nullptr) {
		blk::warning("Unable to load pipeline %s", path.c_str());
		return nullptr;
	}
	m_pipelines[friendly_name] = new_pipeline;

	return new_pipeline;
}

/// Renderer::get_pipeline
RenderPipeline* Renderer::get_pipeline(const std::string& name) {
	if (m_pipelines.find(name) == m_pipelines.end()) {
		return nullptr;
	}

	return m_pipelines[name];
}