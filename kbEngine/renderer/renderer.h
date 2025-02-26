/// renderer.h
///
/// 2025 blk 1.0

#pragma once

#include <set>
#include "Matrix.h"
#include "Quaternion.h"
#include "render_defs.h"

class RenderComponent;
class RenderBuffer;

///	Renderer
class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	static constexpr uint32_t max_frames() { return 2; }

	void initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height);
	void shut_down();

	virtual bool software_renderer() const {
		return false;
	}

	virtual void render() = 0;

	RenderBuffer* create_render_buffer();

	RenderPipeline* load_pipeline(const std::string& friendly_name, const std::string& path);
	RenderPipeline* get_pipeline(const std::string& friendly_name);

	virtual u32 load_texture(const std::string& path) = 0;

	void set_camera_transform(const Vec3& position, const Quat4& rotation);

	void add_render_component(const RenderComponent* const);
	void remove_render_component(const RenderComponent* const);

protected:
	RenderBuffer* get_render_buffer(const size_t& buffer_index) { return m_render_buffers[buffer_index]; }

	// todo make const
	std::set<const RenderComponent*> render_components() {
		return m_render_components;
	}

private:
	virtual void initialize_internal(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) = 0;
	virtual void shut_down_internal() = 0;

	virtual RenderPipeline* create_pipeline(const std::string& friendly_name, const std::string& path) = 0;
	virtual RenderBuffer* create_render_buffer_internal() = 0;

protected:
	u32 m_frame_width;
	u32 m_frame_height;

	/// camera
	Vec3 m_camera_position;
	Quat4 m_camera_rotation;
	Mat4 m_camera_projection;

private:
	std::unordered_map<std::string, RenderPipeline*> m_pipelines;
	std::vector<class RenderBuffer*> m_render_buffers;
	std::unordered_map<std::string, u32> m_textures;

	std::set<const RenderComponent*> m_render_components;
};

extern Renderer* g_renderer;
