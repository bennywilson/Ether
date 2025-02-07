/// d3d12_defs.cpp
///
/// 2025 blk 1.0

#include "blk_core.h"
#include "renderer_Dx12.h"
#include "d3d12_defs.h"

/// Texture_D3D12::load_internal
bool Texture_D3D12::load_internal() {
	return true;
}

/// Texture_D3D12::release_internal
void Texture_D3D12::release_internal() {

}


/// RenderBuffer_D3D12::release
void RenderBuffer_D3D12::release() {
	m_buffer.Reset();
}

/// RenderBuffer_D3D12::write_vb_internal
void RenderBuffer_D3D12::write_vb_internal(const std::vector<vertexLayout>& vertices) {
	vector<vertexLayout> new_verts;
for (auto& vert: vertices) {
	vertexLayout new_vert = vert;
	new_vert.position.z *= -1;
	new_vert.position.x *= -1;
	new_vert.position.z += 1;
	new_verts.push_back(new_vert);
}
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

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
	blk::error_check(m_buffer->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, new_verts.data(), buffer_size);
	m_buffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_vertex_buffer_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
	m_vertex_buffer_view.StrideInBytes = sizeof(vertexLayout);
	m_vertex_buffer_view.SizeInBytes = buffer_size;
}

/// RenderBuffer_D3D12::write_ib_internal
void RenderBuffer_D3D12::write_ib_internal(const std::vector<uint16_t>& indices) {
	auto device = ((Renderer_Dx12*)(g_renderer))->get_device();

	const uint32_t buffer_size = size_bytes();

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	auto ib_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto ib_buff_size = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);
	blk::error_check(device->CreateCommittedResource(
		&ib_heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&ib_buff_size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_buffer)));

	// Copy the triangle data to the vertex buffer.
	UINT8* index_buffer = nullptr;
	CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
	blk::error_check(m_buffer->Map(0, &read_range, reinterpret_cast<void**>(&index_buffer)));
	memcpy(index_buffer, indices.data(), buffer_size);
	m_buffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	m_index_buffer_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
	m_index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
	m_index_buffer_view.SizeInBytes = buffer_size;
}

