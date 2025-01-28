//==============================================================================
// renderer_dx12.h
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

#include "kbRenderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

inline void check_result(HRESULT hr) {
	if (FAILED(hr)) {
		throw;
	}
}

///
///	renderer
///
class renderer {
public:
	renderer();
	~renderer();

	virtual void initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height);
	virtual void shut_down() = 0;

	virtual void render() = 0;

protected:
	uint m_frame_width = 0;
	uint m_frame_height = 0;
};

///
///	renderer_dx12
///
class renderer_dx12 : public renderer {
public:
	~renderer_dx12();

	virtual void shut_down() override;

	virtual void render() override;

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;

private:
	void get_hardware_adapter(
		IDXGIFactory1* const factory,
		IDXGIAdapter1** const out_adapter,
		bool request_high_performance);

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_queue;
	ComPtr<IDXGISwapChain3> m_swap_chain;
	uint32_t m_frame_index = 0;

	ComPtr<ID3D12CommandAllocator> m_command_allocator;
	ComPtr<ID3D12GraphicsCommandList> m_command_list;

	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	uint32_t m_rtv_descriptor_size = 0;

	static const UINT frame_count = 2;
	ComPtr<ID3D12Resource> m_renderTargets[frame_count];

	ComPtr<ID3D12PipelineState> m_pipeline_state;

	// Fences
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fence_value = 0;
	HANDLE m_fence_event;
};

extern renderer* g_renderer;