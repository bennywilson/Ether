/// Renderer_Sw.cpp
///
/// 2025 blk 1.0

#include <d3d12sdklayers.h>
#include "blk_core.h"
#include "Renderer_Sw.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include "d3d12_defs.h"
#include "kbGameEntityHeader.h"
#include "render_component.h"

using namespace std;

static const u64 CONSTANT_BUFFER_SIZE = 4096;
u8* CONSTANT_BUFFER = nullptr;

/// Renderer_Sw::~Renderer_Sw
Renderer_Sw::~Renderer_Sw() {
	shut_down();	// function is virtual but called in ~Renderer which is UB
}

/// Renderer_Sw::initialize_internal
void Renderer_Sw::initialize_internal(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {
	UINT dxgiFactoryFlags = 0;

	m_view_port = CD3DX12_VIEWPORT(0.f, 0.f, (float)frame_width, (float)frame_height);
	m_scissor_rect = CD3DX12_RECT(0, 0, frame_width, frame_height);

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	ComPtr<ID3D12Debug1> spDebugController1;
	debugController->QueryInterface(IID_PPV_ARGS(&spDebugController1));
	spDebugController1->SetEnableGPUBasedValidation(true);

#endif
	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

	ComPtr<IDXGIAdapter1> hw_adapter;
	get_hardware_adapter(factory.Get(), &hw_adapter, true);

	// Device
	blk::error_check(D3D12CreateDevice(
		hw_adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));

	// Queue
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	blk::error_check(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue)));

	// Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = Renderer::max_frames();
	swap_chain_desc.Width = m_frame_width;
	swap_chain_desc.Height = m_frame_height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swap_chain;
	blk::error_check(factory->CreateSwapChainForHwnd(
		m_queue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		hwnd,
		&swap_chain_desc, nullptr,
		nullptr,
		&swap_chain
	));
	blk::error_check(swap_chain.As(&m_swap_chain));
	m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

	// Disable fullscreen
	blk::error_check(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	// RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = Renderer::max_frames();
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	blk::error_check(m_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtv_heap)));
	m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// SRV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = 1 + 1;		// SRV + Constant buffer
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	blk::error_check(m_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&m_cbv_srv_heap)));
	m_cbv_srv_heap->SetName(L"Renderer_Sw::m_cbv_srv_heap");

	// Sampler heap
	D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc = {};
	sampler_heap_desc.NumDescriptors = 1;
	sampler_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	sampler_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	blk::error_check(m_device->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(&m_sampler_heap)));
	m_sampler_heap->SetName(L"Renderer_Sw::m_sampler_heap");

	// Sampler
	D3D12_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	m_device->CreateSampler(&sampler_desc, m_sampler_heap->GetCPUDescriptorHandleForHeapStart());

	// Constants
	const auto CBV_SRV_DESCRIPTOR_SIZE = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// cbv upload heap
	const auto cbv_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto cbv_buffer_size = CD3DX12_RESOURCE_DESC::Buffer(CONSTANT_BUFFER_SIZE);
	blk::error_check(m_device->CreateCommittedResource(
		&cbv_heap_props,
		D3D12_HEAP_FLAG_NONE,
		&cbv_buffer_size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cbv_upload_heap)));

	CD3DX12_RANGE readRange(0, 0);
	blk::error_check(m_cbv_upload_heap->Map(0, &readRange, reinterpret_cast<void**>(&CONSTANT_BUFFER)));

	// Constant buffer view
	UINT64 cb_offset = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(), 0, CBV_SRV_DESCRIPTOR_SIZE);    // Move past the SRVs.

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = m_cbv_upload_heap->GetGPUVirtualAddress() + cb_offset;
	cbv_desc.SizeInBytes = CONSTANT_BUFFER_SIZE;
	cb_offset += cbv_desc.SizeInBytes;

	m_device->CreateConstantBufferView(&cbv_desc, cbvSrvHandle);
	cbvSrvHandle.Offset(CBV_SRV_DESCRIPTOR_SIZE);


	// Frame resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (uint32_t i = 0; i < Renderer::max_frames(); i++) {
		blk::error_check(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
		m_device->CreateRenderTargetView(m_render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.Offset(1, m_rtv_descriptor_size);
	}

	blk::error_check(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)));
	blk::error_check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
	m_command_list->Close();

	// The root signature determines what kind of data the shader should expect.
	CD3DX12_DESCRIPTOR_RANGE1 ranges[3] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	// Root parameters are entries in the root signature
	CD3DX12_ROOT_PARAMETER1 root_parameters[4] = {};
	root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);	// scene_constants
	root_parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);		// sampler
	root_parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);		// srv
	root_parameters[3].InitAsConstants(1, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);				// scene_indices

	const D3D12_ROOT_SIGNATURE_FLAGS signature_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {};
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 0, nullptr, signature_flags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	if (!blk::warn_check(D3DX12SerializeVersionedRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
		blk::error("%s", error->GetBufferPointer());
	}
	blk::error_check(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		vertexLayout verts[6] = {};
		verts[0].position.set(-1.f, 1.f, 0.f);
		verts[0].uv.set(0.f, 0.f);

		verts[1].position.set(1.f, 1.f, 0.f);
		verts[1].uv.set(1.f, 0.f);

		verts[2].position.set(-1.f, -1.f, 0.f);
		verts[2].uv.set(0.f, 1.f);

		verts[3].position.set(1.f, 1.f, 0.f);
		verts[3].uv.set(1.f, 0.f);

		verts[4].position.set(1.f, -1.f, 0.f);
		verts[4].uv.set(1.f, 1.f);

		verts[5].position.set(-1.f, -1.f, 0.f);
		verts[5].uv.set(0.f, 1.f);

		const UINT vertexBufferSize = sizeof(verts);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		auto upload_flags = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		blk::error_check(m_device->CreateCommittedResource(
			&upload_flags,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertex_buffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		blk::error_check(m_vertex_buffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, verts, vertexBufferSize);
		m_vertex_buffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_screen_vertex_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
		m_screen_vertex_view.StrideInBytes = sizeof(vertexLayout);
		m_screen_vertex_view.SizeInBytes = vertexBufferSize;
	}

	// Fences	
	blk::error_check(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence_value = 1;

	// Create an event handle to use for frame synchronization.
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fence_event == nullptr) {
		blk::error_check(HRESULT_FROM_WIN32(GetLastError()));
	}

	auto pipe = (RenderPipeline_D3D12*)load_pipeline("screen_shader", L"C:/projects/Ether/dx12_updgrade/GameBase/assets/shaders/screen_shader.hlsl");

	blk::error_check(m_command_allocator->Reset());
	blk::error_check(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	// Load texture 
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = m_frame_width;
	textureDesc.Height = m_frame_height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	auto default_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	m_device->CreateCommittedResource(
		&default_heap_props,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&tex));
	// Create gpu upload buffer
	//const uint64_t upload_buff_size = GetRequiredIntermediateSize(tex.Get(), 0, (uint32_t)subresources.size());
	const UINT64 upload_buff_size = GetRequiredIntermediateSize(tex.Get(), 0, 1);

	auto upload_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto upload_heap_buff_size = CD3DX12_RESOURCE_DESC::Buffer(upload_buff_size);
	blk::error_check(
		m_device->CreateCommittedResource(
			&upload_heap_props,
			D3D12_HEAP_FLAG_NONE,
			&upload_heap_buff_size,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&upload_resource)));


	std::vector<uint8_t> tex_data;
	for (size_t i = 0; i < (size_t)m_frame_width * m_frame_height * 4; i += 4) {
		tex_data.push_back(0);
		tex_data.push_back(255);
		tex_data.push_back(255);
		tex_data.push_back(255);
	}

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &tex_data[0];
	textureData.RowPitch = (u64)(m_frame_width * 4);
	textureData.SlicePitch = textureData.RowPitch * m_frame_height;
	UpdateSubresources(m_command_list.Get(), tex.Get(), upload_resource.Get(),
		0, 0, 1, &textureData);

	auto tex_barrier = CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_command_list->ResourceBarrier(1, &tex_barrier);

	// Texture srv
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE texHandle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(), 1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
	m_device->CreateShaderResourceView(tex.Get(), &srv_desc, texHandle);

	// Close the command list and execute it to begin the initial GPU setup.
	blk::error_check(m_command_list->Close());
	ID3D12CommandList* ppCommandLists[] = { m_command_list.Get() };
	m_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	wait_on_fence();

	blk::log("Renderer_Sw initialized");
}

/// Renderer_Sw::shut_down_internal
void Renderer_Sw::shut_down_internal() {
	wait_on_fence();

	m_cbv_upload_heap->Unmap(0, nullptr);

	m_root_signature.Reset();
	m_cbv_upload_heap.Reset();
	m_cbv_srv_heap.Reset();
	m_sampler_heap.Reset();
	m_rtv_heap.Reset();

	m_vertex_buffer.Reset();

	m_command_allocator.Reset();
	m_command_list.Reset();

	m_swap_chain.Reset();
	m_render_targets[0].Reset();
	m_render_targets[1].Reset();
	tex.Reset();

	m_fence.Reset();
	m_queue.Reset();

	m_device.Reset();
}

/// Renderer_Sw::get_hardware_adapter
void Renderer_Sw::get_hardware_adapter(
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

/// Renderer_Sw::create_render_buffer_internal
RenderBuffer* Renderer_Sw::create_render_buffer_internal() {
	return new RenderBuffer_D3D12();
}

/// Renderer_Sw::render
void Renderer_Sw::render() {
	blk::error_check(m_command_allocator->Reset());
	blk::error_check(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	render_software_rasterization();


	blk::error_check(m_command_allocator->Reset());
	blk::error_check(m_command_list->Reset(m_command_allocator.Get(), nullptr));

	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	m_command_list->RSSetViewports(1, &m_view_port);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	auto rt_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_render_targets[m_frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_command_list->ResourceBarrier(1, &rt_barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), m_frame_index, m_rtv_descriptor_size);
	m_command_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

	const float clear_color[] = { 0.7f, 0.8f, 0.f, 1.0f };
	m_command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);


	RenderPipeline_D3D12* const pipe = (RenderPipeline_D3D12*)get_pipeline("screen_shader");
	m_command_list->SetPipelineState(pipe->m_pipeline_state.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbv_srv_heap.Get(), m_sampler_heap.Get() };
	m_command_list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_command_list->SetGraphicsRootDescriptorTable(0, m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart());
	m_command_list->SetGraphicsRootDescriptorTable(1, m_sampler_heap->GetGPUDescriptorHandleForHeapStart());
	auto descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart(), 1, descriptor_size);
	m_command_list->SetGraphicsRootDescriptorTable(2, gpu_handle);

	m_command_list->IASetVertexBuffers(0, 1, &m_screen_vertex_view);
	m_command_list->DrawInstanced(6, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	auto res_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_render_targets[m_frame_index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_command_list->ResourceBarrier(1, &res_barrier);
	blk::error_check(m_command_list->Close());

	// Execute command lists
	ID3D12CommandList* const command_lists[] = { m_command_list.Get() };
	m_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

	// Present
	blk::error_check(m_swap_chain->Present(1, 0));

	// Wait for previous frame (todo)
	wait_on_fence();

	m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
}

/// Renderer_Sw::render_software_rasterization
void Renderer_Sw::render_software_rasterization() {
	// Update constant buffer
	m_camera_projection.make_identity();
	m_camera_projection.create_perspective_matrix(
		kbToRadians(50.),
		1197.f / (float)854,
		1.f, 20000.f
	);

	const Mat4 trans = Mat4::make_translation(-m_camera_position);
	Mat4 rot = m_camera_rotation.to_mat4();
	rot.transpose_self();

	Mat4 view_matrix = trans * rot;
	Mat4 vp_matrix =
		view_matrix *
		m_camera_projection;


	std::vector<u8> tex_data;
	tex_data.resize((size_t)m_frame_width * m_frame_height * 4);

	// Temp
	memset(&tex_data[0], 0xffffffff, sizeof(m_frame_width * m_frame_height * 4));

	for (auto render_comp : this->render_components()) {
		if (render_comp->IsA(kbStaticModelComponent::GetType())) {
			const kbStaticModelComponent* const skel_comp = (kbStaticModelComponent*)render_comp;
			const kbModel* const model = skel_comp->model();
		//	const kbModel::mesh_t& mesh = model->GetMeshes()[0];

			Mat4 world_mat;
			world_mat.make_scale(render_comp->owner_scale());
			world_mat *= render_comp->owner_rotation().to_mat4();
			world_mat[3] = render_comp->owner_position();

			Mat4 final_mat = world_mat * vp_matrix;
			const auto& meshes = model->GetMeshes();
			const auto& vertices = model->GetCPUVertices();
			const auto& indices = model->GetCPUIndices();

			for (size_t i = 0; i < indices.size(); i += 3) {
				
				
				for (size_t idx = 0; idx < 3; idx++) {
					const auto& v1 = vertices[indices[i + idx]];
					const Vec4 vertex_pos = v1.position.extend(1.f).transform_point(final_mat, true);
					const u32 x = (vertex_pos.x * 0.5f + 0.5f) * m_frame_width;
					const u32 y = (vertex_pos.y * -0.5f + 0.5f) * m_frame_height;
					const size_t screen_idx = (size_t)(x + y * m_frame_width) * 4;
					if (screen_idx >= tex_data.size()) {
						continue;
					}

					tex_data[screen_idx + 0] = 0xff;
					tex_data[screen_idx + 1] = 0xff;
					tex_data[screen_idx + 2] = 0xff;
					tex_data[screen_idx + 3] = 0xff;
				}
			}
			//const auto& vertices = mesh.m_Vertices;
			//for (size_t i = 0; i < 
			/*const auto& indices = mesh.m_TriangleIndices;

			for (size_t i = 0; i < mesh.m_Vertices.size(); i++) {
				const Vec4 vertex_pos = mesh.m_Vertices[i].extend(1.f).transform_point(final_mat, true);
				const u32 x = (vertex_pos.x * 0.5f + 0.5f) * m_frame_width;
				const u32 y = (vertex_pos.y * -0.5f + 0.5f) * m_frame_height;
				const size_t idx = (size_t)(x + y * m_frame_width) * 4;
				if (idx >= tex_data.size()) {
					continue;
				}

				tex_data[idx + 0] = 0xff;
				tex_data[idx + 1] = 0xff;
				tex_data[idx + 2] = 0xff;
				tex_data[idx + 3] = 0xff;
			}*/
		}
	}

	// Uplodate
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &tex_data[0];
	textureData.RowPitch = (u64)(m_frame_width * 4);
	textureData.SlicePitch = textureData.RowPitch * m_frame_height;

	auto tex_barrier = CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	m_command_list->ResourceBarrier(1, &tex_barrier);

	UpdateSubresources(m_command_list.Get(), tex.Get(), upload_resource.Get(),
		0, 0, 1, &textureData);

	tex_barrier = CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_command_list->ResourceBarrier(1, &tex_barrier);

	blk::error_check(m_command_list->Close());

	// Execute command lists
	ID3D12CommandList* const command_lists[] = { m_command_list.Get() };
	m_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

	wait_on_fence();
}

/// Renderer_Sw::create_pipeline
RenderPipeline* Renderer_Sw::create_pipeline(const wstring& path) {
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
	UINT compileFlags = 0;
#endif

	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	ComPtr<ID3DBlob> vertex_shader;
	if (!blk::warn_check(D3DCompileFromFile(
		path.c_str(),
		nullptr,
		nullptr,
		"vertex_shader",
		"vs_5_1",
		compileFlags,
		0,
		&vertex_shader,
		&errors))) {
		blk::error("%s", errors->GetBufferPointer());
	}

	ComPtr<ID3DBlob> pixel_shader;
	if (!blk::error_check(
		D3DCompileFromFile(
			path.c_str(),
			nullptr,
			nullptr,
			"pixel_shader",
			"ps_5_1",
			compileFlags,
			0,
			&pixel_shader,
			&errors))) {
		blk::error("%s", errors->GetBufferPointer());
	}

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
	//psoDesc.DepthStencilState = 0;//CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	RenderPipeline_D3D12* const pipe = new RenderPipeline_D3D12();
	blk::error_check(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipe->m_pipeline_state)));

	return (RenderPipeline*)pipe;
	return nullptr;
}

/// Renderer_Sw::wait_on_fence
void Renderer_Sw::wait_on_fence() {
	// Wait for previous frame (todo)
	const uint64_t fence = m_fence_value;
	blk::error_check(m_queue->Signal(m_fence.Get(), fence));
	m_fence_value++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence) {
		blk::error_check(m_fence->SetEventOnCompletion(fence, m_fence_event));
		WaitForSingleObject(m_fence_event, INFINITE);
	}
}
