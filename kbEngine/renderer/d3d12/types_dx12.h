#pragma once
#include <wrl/client.h>
#include "d3dx12_core.h"
#include "render_buffer.h"

using namespace std;
using namespace Microsoft::WRL;

class RenderBuffer_D3D12 : public RenderBuffer {
public:
	RenderBuffer_D3D12() = default;

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices) override;
	virtual void write_index_buffer(const std::vector<uint16_t>& indices) override;

	virtual void release();

	
	const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view() const { return m_vertex_buffer_view; }
	const D3D12_INDEX_BUFFER_VIEW& index_buffer_view() const { return m_index_buffer_view; }

private:
	ComPtr<ID3D12Resource> m_vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;

	ComPtr<ID3D12Resource> m_index_buffer;
	D3D12_INDEX_BUFFER_VIEW m_index_buffer_view;
};