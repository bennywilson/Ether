//==============================================================================
// RendererDx12.cpp
//
// 2025 kbEngine 2.0
//==============================================================================
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "kbCore.h"
#include "renderer_dx12.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbConsole.h"
#include <d3dcommon.h>
#include "d3d12/d3dx12.h"

using namespace std;

Renderer* g_renderer = nullptr;

Renderer::Renderer() {

}

Renderer::~Renderer() {
	shut_down();
}

void Renderer::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {
	m_frame_width = frame_width;
	m_frame_height = frame_height;
}


void Renderer::shut_down() {

}

pipeline* Renderer::load_pipeline(const std::string& friendly_name, const std::wstring& path) {
	pipeline* const new_pipeline = create_pipeline(path);
	if (new_pipeline == nullptr) {
		kbWarning("Unable to load pipeline %s", path.c_str());
		return nullptr;
	}
	m_pipelines[friendly_name] = new_pipeline;

	return new_pipeline;
}

pipeline* Renderer::get_pipeline(const std::string& name) {
	if (m_pipelines.find(name) == m_pipelines.end()) {
		return nullptr;
	}

	return m_pipelines[name];
}

void RendererDx12::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {

	UINT dxgiFactoryFlags = 0;

	m_view_port = CD3DX12_VIEWPORT(0.f, 0.f, (float)frame_width, (float)frame_height);
	m_scissor_rect = CD3DX12_RECT(0, 0, frame_width, frame_height);

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

	ComPtr<IDXGIAdapter1> hw_adapter;
	get_hardware_adapter(factory.Get(), &hw_adapter, true);

	// Device
	check_result(D3D12CreateDevice(
		hw_adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));

	// Queue
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	check_result(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue)));

	// Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = RendererDx12::frame_count;
	swap_chain_desc.Width = m_frame_width;
	swap_chain_desc.Height = m_frame_height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swap_chain;
	check_result(factory->CreateSwapChainForHwnd(
		m_queue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		hwnd,
		&swap_chain_desc, nullptr,
		nullptr,
		&swap_chain
	));

	check_result(swap_chain.As(&m_swap_chain));
	m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

	// Disable fullscreen
	check_result(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	// Render target view descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = RendererDx12::frame_count;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	check_result(m_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtv_heap)));

	m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Frame resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (uint32_t i = 0; i < RendererDx12::frame_count; i++) {
		check_result(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
		m_device->CreateRenderTargetView(m_render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.Offset(1, m_rtv_descriptor_size);
	}

	// Command List
	check_result(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)));
	check_result(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
	m_command_list->Close();

	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	check_result(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	check_result(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

	// Fences	
	check_result(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence_value = 1;

	// Create an event handle to use for frame synchronization.
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fence_event == nullptr) {
		check_result(HRESULT_FROM_WIN32(GetLastError()));
	}

	auto pipe = (pipeline_dx12*)load_pipeline("test_shader", L"C:/projects/Ether/dx12_updgrade/GameBase/assets/shaders/test_shader.hlsl");

	kbLog("RendererDx12 initialized");
	todo_create_vertices();
}

RendererDx12::~RendererDx12() {
}

void RendererDx12::shut_down() {
	Renderer::shut_down();
}

void RendererDx12::get_hardware_adapter(
	IDXGIFactory1* const factory,
	IDXGIAdapter1** const out_adapter,
	bool request_high_performance) {
	*out_adapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
		for (
			UINT adapter_index = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapter_index,
				request_high_performance == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
				++adapter_index)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
				break;
			}
		}
	}

	if (adapter.Get() == nullptr) {
		for (UINT adapter_index = 0; SUCCEEDED(factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
				break;
			}
		}
	}

	*out_adapter = adapter.Detach();
}

void RendererDx12::render() {
	check_result(m_command_allocator->Reset());
	check_result(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	m_command_list->RSSetViewports(1, &m_view_port);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_render_targets[m_frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), m_frame_index, m_rtv_descriptor_size);
	m_command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	m_command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

	pipeline_dx12* const pipe = (pipeline_dx12*)get_pipeline("test_shader");
	m_command_list->SetPipelineState(pipe->m_pipeline_state.Get());

	m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
	m_command_list->DrawInstanced(3, 1, 0, 0);


	// Indicate that the back buffer will now be used to present.
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_render_targets[m_frame_index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	check_result(m_command_list->Close());

	// Execute command lists
	ID3D12CommandList* const command_lists[] = { m_command_list.Get() };
	m_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

	// Present
	check_result(m_swap_chain->Present(1, 0));

	// Wait for previous frame (todo)
	const uint64_t fence = m_fence_value;
	check_result(m_queue->Signal(m_fence.Get(), fence));
	m_fence_value++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence) {
		check_result(m_fence->SetEventOnCompletion(fence, m_fence_event));
		WaitForSingleObject(m_fence_event, INFINITE);
	}

	m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
}

pipeline* RendererDx12::create_pipeline(const wstring& path) {
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ID3DBlob** ppErrorMsgs = nullptr;

	ComPtr<ID3DBlob> vertex_shader;
	check_result(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "vertex_shader", "vs_5_0", compileFlags, 0, &vertex_shader, ppErrorMsgs));

	ComPtr<ID3DBlob> pixel_shader;
	check_result(D3DCompileFromFile(path.c_str(), nullptr, nullptr, "pixel_shader", "ps_5_0", compileFlags, 0, &pixel_shader, nullptr));

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_root_signature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	pipeline_dx12* const pipe = new pipeline_dx12();
	check_result(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipe->m_pipeline_state)));

	return (pipeline*)pipe;
}

///
/// 
///
void RendererDx12::todo_create_vertices() {
	// Create the vertex buffer.
	{
		const float aspect = 10.f / 9.f;
		// Define the geometry for a triangle.
		FVertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		check_result(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertex_buffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE read_range(0, 0);        // We do not intend to read from this resource on the CPU.
		check_result(m_vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertex_buffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
		m_vertex_buffer_view.StrideInBytes = sizeof(FVertex);
		m_vertex_buffer_view.SizeInBytes = vertexBufferSize;
	}
}
