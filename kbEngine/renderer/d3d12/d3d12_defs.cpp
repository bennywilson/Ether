/// d3d12_defs.cpp
///
/// 2025 blk 1.0

#include "blk_core.h"
#include "renderer_Dx12.h"
#include "d3d12_defs.h"

/// RenderBuffer_Dx12::release
void RenderBuffer_Dx12::release() {
	m_buffer.Reset();
}

/// RenderBuffer_Dx12::create_internal
void RenderBuffer_Dx12::create_internal() {
	auto device = ((Renderer_Dx12*)(g_renderer))->get_device();

	const uint32_t buffer_size = size_bytes();

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	auto vert_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto vert_buffer_size = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);
	blk::error_check(device->CreateCommittedResource(
		&vert_heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&vert_buffer_size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_buffer)));
}

/// RenderBuffer_Dx12::map
u8* RenderBuffer_Dx12::map() {
	u8* buf = nullptr;
	CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
	blk::error_check(m_buffer->Map(0, &read_range, reinterpret_cast<void**>(&buf)));

	return buf;
}

/// RenderBuffer_Dx12::unmap
void RenderBuffer_Dx12::unmap() {
	m_buffer->Unmap(0, nullptr);
/*	if (m_buffer_type == 1) {
		m_vertex_buffer_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
		m_vertex_buffer_view.StrideInBytes = sizeof(vertexLayout);
		m_vertex_buffer_view.SizeInBytes = size_bytes();
	} else {
		m_index_buffer_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
		m_index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
		m_index_buffer_view.SizeInBytes = size_bytes();
	}*/
}

/// RenderBuffer_Dx12::vertex_buffer_view
D3D12_VERTEX_BUFFER_VIEW RenderBuffer_Dx12::vertex_buffer_view() const {
	D3D12_VERTEX_BUFFER_VIEW view = {};
	view.BufferLocation = m_buffer->GetGPUVirtualAddress();
	view.StrideInBytes = sizeof(vertexLayout);
	view.SizeInBytes = size_bytes();

	return view;
}

/// RenderBuffer_Dx12::index_buffer_view
D3D12_INDEX_BUFFER_VIEW RenderBuffer_Dx12::index_buffer_view() const {
	D3D12_INDEX_BUFFER_VIEW view = {};
	view.BufferLocation = m_buffer->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R16_UINT;
	view.SizeInBytes = size_bytes();

	return view;
}

