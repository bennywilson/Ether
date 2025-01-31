//==============================================================================
// RendererDx12.h
//
// Renderer implementation using DX12 API
//
// 2025 kbEngine 2.0
//==============================================================================
#pragma once

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "kbVector.h"
#include "d3dx12_core.h"
#include "kbRenderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

inline void check_result(HRESULT hr) {
	if (FAILED(hr)) {
		throw;
	}
}

class RenderBuffer;

///
/// FVertex
///
struct FVertex {
	XMFLOAT3 position;
	XMFLOAT4 color;
};

///
/// pipeline
///
class pipeline {
public:
	virtual void release() = 0;

private:
	std::string name;
};

///
///	Renderer
///
class Renderer {
public:
	Renderer();
	~Renderer();

	virtual void initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height);
	virtual void shut_down() = 0;

	virtual void render() = 0;

	RenderBuffer* create_render_buffer();

	pipeline* load_pipeline(const std::string& friendly_name, const std::wstring& path);
	pipeline* get_pipeline(const std::string& friendly_name);

protected:
	RenderBuffer* get_render_buffer(const size_t& buffer_index) { return m_render_buffers[buffer_index]; }

private:
	virtual pipeline* create_pipeline(const std::wstring& path) = 0;
	virtual RenderBuffer* create_render_buffer_internal() = 0;

protected:
	uint m_frame_width = 0;
	uint m_frame_height = 0;

private:
	std::unordered_map<std::string, pipeline*> m_pipelines;
	std::vector<class RenderBuffer*> m_render_buffers;
};

class pipeline_dx12 : public pipeline {
	friend class RendererDx12;
	virtual void release() {}

	ComPtr<ID3D12PipelineState> m_pipeline_state;
};

///
///	RendererDx12
///
class RendererDx12 : public Renderer {
public:
	~RendererDx12();

	virtual void shut_down() override;

	virtual void render() override;

	ComPtr<ID3D12Device> get_device() const { return m_device; }

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;

	void todo_create_vertices();

private:
	void get_hardware_adapter(
		IDXGIFactory1* const factory,
		IDXGIAdapter1** const out_adapter,
		bool request_high_performance);

	virtual pipeline* create_pipeline(const std::wstring& path) override;
	virtual RenderBuffer* create_render_buffer_internal() override;

	static const UINT frame_count = 2;


	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_queue;

	ComPtr<IDXGISwapChain3> m_swap_chain;
	uint32_t m_frame_index = 0;

	CD3DX12_VIEWPORT m_view_port;
	CD3DX12_RECT m_scissor_rect;

	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	uint32_t m_rtv_descriptor_size = 0;

	ComPtr<ID3D12CommandAllocator> m_command_allocator;
	ComPtr<ID3D12GraphicsCommandList> m_command_list;

	ComPtr<ID3D12Resource> m_render_targets[frame_count];

	ComPtr<ID3D12RootSignature> m_root_signature;

	// Fences
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fence_value = 0;
	HANDLE m_fence_event;

	ComPtr<ID3D12Resource> m_vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
};

extern Renderer* g_renderer;