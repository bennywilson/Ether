/// renderer_d3d12.h	
///
/// 2025 kbEngine 2.0

#pragma once

#include "d3dx12_core.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include "renderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

///	RendererD3D12
class RendererD3D12 : public Renderer {
public:
	~RendererD3D12();

	virtual void shut_down() override;

	virtual void render() override;

	ComPtr<ID3D12Device> get_device() const { return m_device; }

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;

	void todo_create_texture();
	ComPtr<ID3D12Resource> tex;

private:
	void get_hardware_adapter(
		struct IDXGIFactory1* const factory,
		struct IDXGIAdapter1** const out_adapter,
		bool request_high_performance);

	virtual RenderPipeline* create_pipeline(const std::wstring& path) override;
	virtual RenderBuffer* create_render_buffer_internal() override;

	static const UINT frame_count = 2;

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_queue;

	ComPtr<struct IDXGISwapChain3> m_swap_chain;
	uint32_t m_frame_index = 0;

	CD3DX12_VIEWPORT m_view_port;
	CD3DX12_RECT m_scissor_rect;

	ComPtr<ID3D12DescriptorHeap> m_sampler_heap;

	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_pCbvDataBegin;

	ComPtr<ID3D12DescriptorHeap> m_cbv_srv_heap;
	ComPtr<ID3D12Resource> m_cbv_upload_heap;
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
};
