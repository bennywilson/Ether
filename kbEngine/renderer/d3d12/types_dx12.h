#pragma once
#include <wrl/client.h>
#include "d3dx12_core.h"
#include "render_buffer.h"

using namespace std;

class RenderBuffer_D3D12 : public RenderBuffer {
public:
	RenderBuffer_D3D12() = default;

	virtual void write(const std::vector<vertexLayout>& vertices);

	virtual void release();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer;
};