/// renderer.h
///
/// 2025 blk 1.0

#pragma once

#include "kbVector.h"
#include "kbQuaternion.h"
#include "render_defs.h"

inline void check_result(HRESULT hr) {
	if (FAILED(hr)) {
		throw;
	}
}

class RenderBuffer;

///	Renderer
class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	static constexpr uint32_t max_frames() { return 2; }

	virtual void initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height);
	virtual void shut_down() = 0;

	virtual void set_camera_transform(const Vec3& position, const kbQuat& rotation);

	virtual void render() = 0;

	RenderBuffer* create_render_buffer();

	RenderPipeline* load_pipeline(const std::string& friendly_name, const std::wstring& path);
	RenderPipeline* get_pipeline(const std::string& friendly_name);

protected:
	RenderBuffer* get_render_buffer(const size_t& buffer_index) { return m_render_buffers[buffer_index]; }

private:
	virtual RenderPipeline* create_pipeline(const std::wstring& path) = 0;
	virtual RenderBuffer* create_render_buffer_internal() = 0;

protected:
	uint m_frame_width;
	uint m_frame_height;

	/// camera
	Vec3 m_camera_position;
	kbQuat m_camera_rotation;
	Mat4 m_camera_projection;

private:
	std::unordered_map<std::string, RenderPipeline*> m_pipelines;
	std::vector<class RenderBuffer*> m_render_buffers;
};

extern Renderer* g_renderer;
