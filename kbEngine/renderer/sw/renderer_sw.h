/// Renderer_Dx12.h	
///
/// 2025 blk 1.0

#pragma once

#include "d3dx12_core.h"
#include <wrl/client.h>
#include "renderer.h"

//using namespace DirectX;
using Microsoft::WRL::ComPtr;

class RenderPipeline_Dx12;
class TrianglePipeline;
class KuwaharaPipeline;

///	Renderer_Sw
class Renderer_Sw : public Renderer {
public:
	~Renderer_Sw();

	ComPtr<ID3D12Device> get_device() const { return m_device; }

	virtual bool software_renderer() const override {
		return true;
	};

protected:
	void todo_create_texture();
	ComPtr<ID3D12Resource> tex;

private:
	virtual void initialize_internal(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;

	virtual void shut_down_internal() override;

	void get_hardware_adapter(
		struct IDXGIFactory1* const factory,
		struct IDXGIAdapter1** const out_adapter,
		bool request_high_performance);

	virtual RenderPipeline* create_pipeline(const std::string& friendly_name, const std::wstring& path) override;
	virtual RenderBuffer* create_render_buffer_internal() override;

	// For blitting the final image to the screen
	void create_blit_pipeline();

	virtual void render() override;
	void render_software_rasterization();

	void wait_on_fence();

	CD3DX12_VIEWPORT m_view_port;
	CD3DX12_RECT m_scissor_rect;

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_queue;

	ComPtr<struct IDXGISwapChain3> m_swap_chain;
	uint32_t m_frame_index = 0;

	ComPtr<ID3D12CommandAllocator> m_command_allocator;
	ComPtr<ID3D12GraphicsCommandList> m_command_list;

	ComPtr<ID3D12RootSignature> m_root_signature;

	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	uint32_t m_rtv_descriptor_size = 0;

	ComPtr<ID3D12DescriptorHeap> m_sampler_heap;

	ComPtr<ID3D12DescriptorHeap> m_cbv_srv_heap;
	ComPtr<ID3D12Resource> m_cbv_upload_heap;

	ComPtr<ID3D12Resource> m_render_targets[Renderer::max_frames()];

	ComPtr<ID3D12Resource> m_vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW m_screen_vertex_view;
	ComPtr<ID3D12Resource> upload_resource;

	RenderPipeline_Dx12* m_blit_pipeline;

	// Fences
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fence_value = 0;
	HANDLE m_fence_event;
};
