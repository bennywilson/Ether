/// vk_defs.h
///
/// 2025 kbEngine 2.0

#pragma once

#include <wrl/client.h>
#include "render_defs.h"

using namespace std;
using namespace Microsoft::WRL;

/// RenderPipeline_VK
class RenderPipeline_VK : public RenderPipeline {
	~RenderPipeline_VK() { }

	virtual void release() { }
};

/// RenderBuffer_VK
class RenderBuffer_VK : public RenderBuffer {
public:
	RenderBuffer_VK() = default;

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices) override {}
	virtual void write_index_buffer(const std::vector<uint16_t>& indices) override {}

	virtual void release() {}
private:

};