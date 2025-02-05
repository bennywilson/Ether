/// render_defs.cpp
///
/// 2025 blk 1.0

#include <vector>
#include "kbCore.h"
#include "kbRenderer_defs.h"
#include "render_defs.h"

/// RenderBuffer::write_vertex_buffer
void RenderBuffer::write_vertex_buffer(const std::vector<vertexLayout>& vertices) {
	m_num_elements = (uint32_t)vertices.size();
	m_size_bytes = m_num_elements * sizeof(vertexLayout);
	write_vb_internal(vertices);
}

/// RenderBuffer::write_index_buffer
void RenderBuffer::write_index_buffer(const std::vector<uint16_t>& indices) {
	m_num_elements = (uint32_t)indices.size();
	m_size_bytes = m_num_elements * sizeof(uint16_t);
	write_ib_internal(indices);
}