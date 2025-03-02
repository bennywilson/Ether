/// vk_defs.h
///
/// 2025 blk 1.0

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

	virtual void release() {}
private:
	virtual void create_internal() override {}
};