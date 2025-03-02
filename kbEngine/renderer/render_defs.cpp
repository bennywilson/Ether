/// render_defs.cpp
///
/// 2025 blk 1.0

#include <vector>
#include "blk_core.h"
#include "render_defs.h"

using namespace std;

/// RenderBuffer::create_vertex_buffer
void RenderBuffer::create_vertex_buffer(const u32 num_verts) {
	m_num_elements = num_verts;
	m_size_bytes = m_num_elements * sizeof(vertexLayout);
	create_internal();
}

/// RenderBuffer::write_vertex_buffer
void RenderBuffer::write_vertex_buffer(const vector<vertexLayout>& vertices) {
	m_num_elements = (uint32_t)vertices.size();
	m_size_bytes = m_num_elements * sizeof(vertexLayout);
	create_internal();

	u8* const vb = map();
	memcpy(vb, vertices.data(), m_size_bytes);
	unmap();
}

/// RenderBuffer::create_index_buffer
void RenderBuffer::create_index_buffer(const u32 num_indices) {
	m_num_elements = num_indices;
	m_size_bytes = m_num_elements * sizeof(u16);
	create_internal();
}

/// RenderBuffer::write_index_buffer
void RenderBuffer::write_index_buffer(const vector<u16>& indices) {
	m_num_elements = (uint32_t)indices.size();
	m_size_bytes = m_num_elements * sizeof(u16);
	create_internal();

	u8* const ib = map();
	memcpy(ib, indices.data(), m_size_bytes);
	unmap();
}