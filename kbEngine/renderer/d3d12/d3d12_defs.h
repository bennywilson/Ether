/// d3d12_defs.h
///
/// 2025 blk 1.0

#pragma once

#include <wrl/client.h>
#include "render_defs.h"
#include "kbMaterial.h"

using namespace std;
using namespace Microsoft::WRL;

/// RenderPipeline_Dx12
class RenderPipeline_Dx12 : public RenderPipeline {
	friend class Renderer_Dx12;
	friend class Renderer_Sw;

	~RenderPipeline_Dx12() { m_pipeline_state.Reset(); }

	virtual void release() { m_pipeline_state.Reset();  }

	ComPtr<ID3D12PipelineState> m_pipeline_state;
};

/// RenderBuffer_Dx12
class RenderBuffer_Dx12 : public RenderBuffer {
public:
	RenderBuffer_Dx12() = default;
	virtual ~RenderBuffer_Dx12() {}

	virtual void release();

	virtual u8* map() override;
	virtual void unmap() override;

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view() const;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view() const;

private:
	virtual void create_internal() override;

	ComPtr<ID3D12Resource> m_buffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
	D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;

	int m_buffer_type = 0;
};