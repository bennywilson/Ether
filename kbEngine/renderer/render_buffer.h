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

	virtual void write(const std::vector<vertexLayout>& vertices) = 0;
	virtual void release() {};
};