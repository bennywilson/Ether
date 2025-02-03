/// d3d12_defs.cpp
///
/// 2025 kbEngine 2.0

#include "kbCore.h"
#include "renderer_d3d12.h"
#include "d3d12_defs.h"

/// RenderBuffer_D3D12::release
void RenderBuffer_D3D12::release() {
	m_vertex_buffer.Reset();
	m_index_buffer.Reset();
	/*if (m_vertex_buffer.Get()) {
		SAFE_RELEASE(m_vertex_buffer);
	}

	if (m_index_buffer.Get()) {
		SAFE_RELEASE(m_index_buffer);
	}*/
}

/// RenderBuffer_D3D12::write_vertex_buffer
void RenderBuffer_D3D12::write_vertex_buffer(const std::vector<vertexLayout>& vertices) {
	RenderBuffer::write_vertex_buffer(vertices);

	vector<vertexLayout> new_verts;
for (auto& vert: vertices) {
	vertexLayout new_vert = vert;
	new_vert.position.z *= -1;
	new_vert.position.x *= -1;
	new_vert.position.z += 1;
	new_verts.push_back(new_vert);
}
	auto device = ((RendererD3D12*)(g_renderer))->get_device();

	const uint32_t buffer_size = size_bytes();

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	check_result(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertex_buffer)));

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
	check_result(m_vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, new_verts.data(), buffer_size);
	m_vertex_buffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
	m_vertex_buffer_view.StrideInBytes = sizeof(vertexLayout);
	m_vertex_buffer_view.SizeInBytes = buffer_size;
}

/// RenderBuffer_D3D12::write_index_buffer
void RenderBuffer_D3D12::write_index_buffer(const std::vector<uint16_t>& indices) {
	RenderBuffer::write_index_buffer(indices);

	auto device = ((RendererD3D12*)(g_renderer))->get_device();

	const uint32_t buffer_size = size_bytes();

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	check_result(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_index_buffer)));

	// Copy the triangle data to the vertex buffer.
	UINT8* index_buffer = nullptr;
	CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
	check_result(m_index_buffer->Map(0, &read_range, reinterpret_cast<void**>(&index_buffer)));
	memcpy(index_buffer, indices.data(), buffer_size);
	m_index_buffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_index_buffer_view.BufferLocation = m_index_buffer->GetGPUVirtualAddress();
	m_index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
	m_index_buffer_view.SizeInBytes = buffer_size;
}
