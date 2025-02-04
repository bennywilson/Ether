/// d3d12_defs.h
///
/// 2025 kbEngine 2.0

#pragma once

#include <wrl/client.h>
#include "render_defs.h"

using namespace std;
using namespace Microsoft::WRL;

/// RenderPipeline_D3D12
class RenderPipeline_D3D12 : public RenderPipeline {
	friend class Renderer_D3D12;
	~RenderPipeline_D3D12() { m_pipeline_state.Reset(); }

	virtual void release() { m_pipeline_state.Reset();  }

	ComPtr<ID3D12PipelineState> m_pipeline_state;
};

/// RenderBuffer_D3D12
class RenderBuffer_D3D12 : public RenderBuffer {
public:
	RenderBuffer_D3D12() = default;

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices) override;
	virtual void write_index_buffer(const std::vector<uint16_t>& indices) override;

	virtual void release();

	const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view() const { return m_vertex_buffer_view; }
	const D3D12_INDEX_BUFFER_VIEW& index_buffer_view() const { return m_index_buffer_view; }

private:
	ComPtr<ID3D12Resource> m_vertex_buffer;	// todo: share this resource with m_index_buffer
	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;

	ComPtr<ID3D12Resource> m_index_buffer;
	D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;
};