/// renderer.cpp
///
/// 2025 blk 1.0

#include "kbCore.h"
#include "renderer.h"

Renderer* g_renderer = nullptr;

/// Renderer::Renderer
Renderer::Renderer() :
	m_frame_width(0),
	m_frame_height(0),
	m_camera_position(0.f, 0.f, 0.f),
	m_camera_rotation(0.f, 0.f, 0.f, 1.f) {
	g_renderer = this;
	m_camera_projection.make_identity();
}

/// Renderer::~Renderer
Renderer::~Renderer() {
	shut_down();
}

/// Renderer::initialize
void Renderer::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {
	m_frame_width = frame_width;
	m_frame_height = frame_height;

	m_camera_projection.create_perspective_matrix(
		kbToRadians(50.f),
		m_frame_width / (float)m_frame_height,
		0.1f, 10000.f
	);
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

/// Renderer::set_camera_transform
void Renderer::set_camera_transform(const Vec3& position, const kbQuat& rotation) {
	m_camera_position = position;
	m_camera_rotation = rotation;
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