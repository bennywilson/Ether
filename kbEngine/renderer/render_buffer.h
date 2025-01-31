///
/// RenderBuffer
///
/// 2025
/// 
class RenderBuffer {
public:
	RenderBuffer() = default;

	~RenderBuffer() {
		release();
	}

	virtual void write_vertex_buffer(const std::vector<vertexLayout>& vertices) = 0;
	virtual void write_index_buffer(const std::vector<uint16_t>& indices) = 0;

	virtual void release() {};
};