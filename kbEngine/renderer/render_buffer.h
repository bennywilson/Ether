/// render_buffer.h
///
/// 2025 kbEngine 2.0

class RenderBuffer {
public:
	RenderBuffer() = default;

	~RenderBuffer() {
		release();
	}

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices);
	virtual void write_index_buffer(const std::vector<uint16_t>& indices);

	virtual void release() {};

	uint32_t num_elements() const { return m_num_elements; }
	uint32_t size_bytes() const { return m_size_bytes; }

private:
	uint32_t m_num_elements = 0;
	uint32_t m_size_bytes = 0;
};