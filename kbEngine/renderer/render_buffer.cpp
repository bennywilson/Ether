/// render_buffer.cpp
///
/// 2025 kbEngine 2.0

#include <Windows.h>
#include <vector>
#include "kbCore.h"
#include "kbRenderer_defs.h"
#include "render_buffer.h"

void RenderBuffer::write_vertex_buffer(const std::vector<vertexLayout>& vertices) {
	m_num_elements = vertices.size();
	m_size_bytes = m_num_elements * vertices.size();
}

void RenderBuffer::write_index_buffer(const std::vector<uint16_t>& indices) {
	m_num_elements = indices.size();
	m_size_bytes = m_num_elements * indices.size();
}