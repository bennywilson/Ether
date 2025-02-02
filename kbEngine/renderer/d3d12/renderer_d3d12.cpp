/// renderer_d3d12.cpp
///
/// 2025 kbEngine 2.0

#include "kbCore.h"
#include "renderer_d3d12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include "DDSTextureLoader12.h"
#include "d3d12_defs.h"

using namespace std;

Renderer* Renderer::create() {
	return new RendererD3D12();
}

void Renderer::shut_down() {
	for (auto& pipe : m_pipelines) {
		delete pipe.second;
	}
	m_pipelines.clear();

	for (size_t i = 0; i < m_render_buffers.size(); i++) {
		m_render_buffers[i]->release();
		delete m_render_buffers[i];
	}
	m_render_buffers.clear();
}

RenderBuffer* Renderer::create_render_buffer() {
	RenderBuffer* const buffer = create_render_buffer_internal();
	m_render_buffers.push_back(buffer);
	return buffer;
}

RenderPipeline* Renderer::load_pipeline(const std::string& friendly_name, const std::wstring& path) {
	RenderPipeline* const new_pipeline = create_pipeline(path);
	if (new_pipeline == nullptr) {
		kbWarning("Unable to load pipeline %s", path.c_str());
		return nullptr;
	}
	m_pipelines[friendly_name] = new_pipeline;

	return new_pipeline;
}

RenderPipeline* Renderer::get_pipeline(const std::string& name) {
	if (m_pipelines.find(name) == m_pipelines.end()) {
		return nullptr;
	}

	return m_pipelines[name];
}

void RendererD3D12::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {

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
	swap_chain_desc.BufferCount = RendererD3D12::frame_count;
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
	rtv_heap_desc.NumDescriptors = RendererD3D12::frame_count;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	check_result(m_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtv_heap)));

	m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Describe and create a shader resource view (SRV) heap for the texture.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	check_result(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_cbv_srv_heap)));
	m_cbv_srv_heap->SetName(L"m_cbv_srv_heap");

	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = 1;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	check_result(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_sampler_heap)));

	// Frame resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (uint32_t i = 0; i < RendererD3D12::frame_count; i++) {
		check_result(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
		m_device->CreateRenderTargetView(m_render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.Offset(1, m_rtv_descriptor_size);
	}

	// Command List
	check_result(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)));
	check_result(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
	m_command_list->Close();



	CD3DX12_DESCRIPTOR_RANGE1 ranges[3] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3] = {};
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	check_result(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	check_result(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

	/*
			//Create an empty root signature.
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		check_result(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		check_result(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
	*/
	// Fences	
	check_result(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence_value = 1;

	// Create an event handle to use for frame synchronization.
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fence_event == nullptr) {
		check_result(HRESULT_FROM_WIN32(GetLastError()));
	}

	auto pipe = (RenderPipeline_D3D12*)load_pipeline("test_shader", L"C:/projects/Ether/dx12_updgrade/GameBase/assets/shaders/test_shader.hlsl");

	kbLog("RendererD3D12 initialized");
	todo_create_texture();
}

RendererD3D12::~RendererD3D12() {
}

void RendererD3D12::shut_down() {
	Renderer::shut_down();
}

void RendererD3D12::get_hardware_adapter(
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

RenderBuffer* RendererD3D12::create_render_buffer_internal() {
	return new RenderBuffer_D3D12();
}



struct Constant {
	kbMat4 mvp;
	kbMat4 padding[3];
};
Constant buffer;
Constant* pBuffer;

void RendererD3D12::render() {


	pBuffer->mvp.MakeIdentity();
	pBuffer->padding[0].MakeIdentity();
	pBuffer->padding[1].MakeIdentity();
	pBuffer->padding[2].MakeIdentity();

	/*buffer.mvp.MakeIdentity();
	buffer.padding[0].MakeIdentity();
	buffer.padding[1].MakeIdentity();
	buffer.padding[2].MakeIdentity();
	memcpy(m_pCbvDataBegin, &buffer, sizeof(buffer));*/

	check_result(m_command_allocator->Reset());
	check_result(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	m_command_list->RSSetViewports(1, &m_view_port);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_render_targets[m_frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), m_frame_index, m_rtv_descriptor_size);
	m_command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

	const float clear_color[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	m_command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

	RenderPipeline_D3D12* const pipe = (RenderPipeline_D3D12*)get_pipeline("test_shader");
	m_command_list->SetPipelineState(pipe->m_pipeline_state.Get());



	auto vertex_buffer = (RenderBuffer_D3D12*)get_render_buffer(0);
	auto index_buffer = (RenderBuffer_D3D12*)get_render_buffer(1);

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbv_srv_heap.Get(), m_sampler_heap.Get() };
	m_command_list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list->IASetIndexBuffer(&index_buffer->index_buffer_view());
	m_command_list->IASetVertexBuffers(0, 1, &vertex_buffer->vertex_buffer_view());

	m_command_list->SetGraphicsRootDescriptorTable(0, m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart());
	m_command_list->SetGraphicsRootDescriptorTable(1, m_sampler_heap->GetGPUDescriptorHandleForHeapStart());
	auto descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart(), 1, descriptor_size);
	m_command_list->SetGraphicsRootDescriptorTable(2, gpu_handle);




	m_command_list->DrawIndexedInstanced(index_buffer->num_elements(), 1, 0, 0, 0);

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

RenderPipeline* RendererD3D12::create_pipeline(const wstring& path) {
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
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	auto raster = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	raster.CullMode = D3D12_CULL_MODE_BACK;

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_root_signature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
	psoDesc.RasterizerState = raster;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	RenderPipeline_D3D12* const pipe = new RenderPipeline_D3D12();
	check_result(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipe->m_pipeline_state)));

	return (RenderPipeline*)pipe;
}

void RendererD3D12::todo_create_texture() {

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	check_result(m_command_allocator->Reset());
	check_result(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	check_result(LoadDDSTextureFromFile(
		m_device.Get(),
		L"C:/projects/Ether/dx12_updgrade/GameBase/assets/Test/diablo.dds",
		tex.ReleaseAndGetAddressOf(),
		ddsData,
		subresources));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(tex.Get(), 0,
		static_cast<UINT>(subresources.size()));

	// Create the GPU upload buffer.
	ComPtr<ID3D12Resource> uploadRes;
	check_result(
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadRes)));

	UpdateSubresources(m_command_list.Get(), tex.Get(), uploadRes.Get(),
		0, 0, static_cast<UINT>(subresources.size()), subresources.data());
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Create SRV 
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_BC1_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(tex.Get(), &srvDesc, m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart());




	check_result(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(buffer)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cbvUploadHeap)));

	// Map the constant buffers. Note that unlike D3D11, the resource 
	// does not need to be unmapped for use by the GPU. In this sample, 
	// the resource stays 'permenantly' mapped to avoid overhead with 
	// mapping/unmapping each frame.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	pBuffer = nullptr;
	check_result(m_cbvUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&pBuffer)));
	pBuffer->mvp.MakeIdentity();
	pBuffer->padding[0].MakeIdentity();
	pBuffer->padding[1].MakeIdentity();
	pBuffer->padding[2].MakeIdentity();

	////////////////////////////////////////////////////////
	auto m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);    // Move past the SRVs.
//	for (UINT i = 0; i < FrameCount; i++)
	{
		//FrameResource* pFrameResource = new FrameResource(m_device.Get(), CityRowCount, CityColumnCount, CityMaterialCount, CitySpacingInterval);

		UINT64 cbOffset = 0;
		//for (UINT j = 0; j < CityRowCount; j++)
		{
		//	for (UINT k = 0; k < CityColumnCount; k++)
			{
				// Describe and create a constant buffer view (CBV).
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = m_cbvUploadHeap->GetGPUVirtualAddress() + cbOffset;
				cbvDesc.SizeInBytes = sizeof(buffer);
				cbOffset += cbvDesc.SizeInBytes;
				m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
				cbvSrvHandle.Offset(m_cbvSrvDescriptorSize);
			}
		}
	}
	////////////////////////////////////////////////////////



	// Close the command list and execute it to begin the initial GPU setup.
	check_result(m_command_list->Close());
	ID3D12CommandList* ppCommandLists[] = { m_command_list.Get() };
	m_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	check_result(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence_value = 1;

	// Create an event handle to use for frame synchronization.
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fence_event == nullptr)
	{
		check_result(HRESULT_FROM_WIN32(GetLastError()));
	}

	// Wait for previous frame (todo)
	const uint64_t fence = m_fence_value;
	check_result(m_queue->Signal(m_fence.Get(), fence));
	m_fence_value++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence) {
		check_result(m_fence->SetEventOnCompletion(fence, m_fence_event));
		WaitForSingleObject(m_fence_event, INFINITE);
	}


//	pBuffer->mvp.MakeIdentity();
}
