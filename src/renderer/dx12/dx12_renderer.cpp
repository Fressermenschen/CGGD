#include "dx12_renderer.h"

#include "utils/com_error_handler.h"
#include "utils/window.h"

#include <DirectXColors.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <d3dcompiler.h>
#include <filesystem>

void cg::renderer::dx12_renderer::init()
{
	UINT debugFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
	debugFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(debugFlags, IID_PPV_ARGS(&factory)));

	HRESULT hresult = D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device));

	if (FAILED(hresult))
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

		ThrowIfFailed(D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&device)));
	}

#ifdef _DEBUG
	UINT adapterIdx = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (factory->EnumAdapters(adapterIdx, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"-Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		UINT adapterOutputIdx = 0;
		IDXGIOutput* output = nullptr;
		while (adapter->EnumOutputs(adapterOutputIdx, &output) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);

			std::wstring text = L"|--Output: ";
			text += desc.DeviceName;
			text += L"\n";
			OutputDebugString(text.c_str());

			output->Release();
			output = nullptr;

			++adapterOutputIdx;
		}
		adapter->Release();
		++adapterIdx;
	}
#endif

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&command_queue)));

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(command_allocator.GetAddressOf())));

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), nullptr, IID_PPV_ARGS(command_list.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = settings->width;
	swapChainDesc.BufferDesc.Height = settings->height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = frame_number;
	swapChainDesc.OutputWindow = utils::window::hwnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(factory->CreateSwapChain(command_queue.Get(), &swapChainDesc, swap_chain.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = frame_number;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtv_heap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsv_heap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i != frame_number; ++i)
	{
		swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));
		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = settings->width;
	depthStencilDesc.Height = settings->height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(depth_stencil_buffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(depth_stencil_buffer.Get(), &dsvDesc,
								   dsv_heap->GetCPUDescriptorHandleForHeapStart());

	command_list->ResourceBarrier(1,
								  &CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_buffer.Get(),	D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<float>(settings->width);
	view_port.Height = static_cast<float>(settings->height);
	view_port.MinDepth = 0.0f;
	view_port.MaxDepth = 1.0f;

	scissor_rect = CD3DX12_RECT{
			0, 0,
			static_cast<LONG>(settings->width),
			static_cast<LONG>(settings->height)};

	ThrowIfFailed(command_list->Close());
	command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(
												  command_list.GetAddressOf()));
	wait_for_gpu();
	ThrowIfFailed(command_list->Reset(command_allocator.Get(), nullptr));

	current_render_target_idx = 0;

	load_assets();

	load_pipeline();
}

void cg::renderer::dx12_renderer::destroy()
{
	wait_for_gpu();
}

void cg::renderer::dx12_renderer::update()
{
	using namespace DirectX;
	const XMMATRIX world = model->get_world_matrix();
	const XMMATRIX view = camera->get_view_matrix();
	const XMMATRIX projection = camera->get_projection_matrix();

	const XMMATRIX wvp = XMMatrixMultiply(XMMatrixMultiply(world, view), projection);
	XMStoreFloat4x4(&world_view_projection, XMMatrixTranspose(wvp));

	CopyMemory(constant_buffer_location, &world_view_projection, sizeof(XMFLOAT4X4));
}

void cg::renderer::dx12_renderer::render()
{
	command_allocator->Reset();
	command_list->Reset(command_allocator.Get(), pipeline_state.Get());

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = vertex_stride;
	vbv.SizeInBytes = vertex_buffer_stride;

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = index_buffer->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = index_buffer_size;

	command_list->IASetVertexBuffers(0, 1, &vbv);
	command_list->IASetIndexBuffer(&ibv);
	command_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	command_list->SetDescriptorHeaps(1, cbv_heap.GetAddressOf());
	command_list->SetGraphicsRootSignature(root_signature.Get());
	command_list->SetGraphicsRootDescriptorTable(0, cbv_heap->GetGPUDescriptorHandleForHeapStart());

	command_list->RSSetViewports(1, &view_port);
	command_list->RSSetScissorRects(1, &scissor_rect);

	CD3DX12_RESOURCE_BARRIER presentToRender = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[current_render_target_idx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	command_list->ResourceBarrier(1, &presentToRender);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtv_heap->GetCPUDescriptorHandleForHeapStart(),	current_render_target_idx, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heap->GetCPUDescriptorHandleForHeapStart();

	command_list->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

	command_list->ClearRenderTargetView(rtvHandle, DirectX::Colors::Aqua, 0, nullptr);

	command_list->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);

	CD3DX12_RESOURCE_BARRIER renderToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
			render_targets[current_render_target_idx].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
	command_list->ResourceBarrier(1, &renderToPresent);

	ThrowIfFailed(command_list->Close());

	command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(command_list.GetAddressOf()));

	wait_for_gpu();

	ThrowIfFailed(swap_chain->Present(1, 0));

	current_render_target_idx = ++current_render_target_idx % frame_number;
}

void cg::renderer::dx12_renderer::load_pipeline()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbv_heap)));

	size_t cbSize = sizeof(DirectX::XMFLOAT4X4) + 255 & ~255;

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(cbSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constant_buffer)));

	ThrowIfFailed(constant_buffer->Map(0, nullptr, reinterpret_cast<void**>(&constant_buffer_location)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = constant_buffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(cbSize);

	device->CreateConstantBufferView(&cbvDesc, cbv_heap->GetCPUDescriptorHandleForHeapStart());

	CD3DX12_ROOT_PARAMETER slotRootParameter;

	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter.InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, &slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr)
	{
		OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),	serializedRootSig->GetBufferSize(),	IID_PPV_ARGS(&root_signature)));

	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> vsByteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	HRESULT hr;
	hr = D3DCompileFromFile(L"shaders\\shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vsByteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
	}
	ThrowIfFailed(hr);

	ComPtr<ID3DBlob> psByteCode = nullptr;
	hr = D3DCompileFromFile(L"shaders\\shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &psByteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
	}
	ThrowIfFailed(hr);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

	};

	std::vector<d3d_vertex> vertices;
	std::vector<UINT> indices;
	UINT index_offset = 0;
	for (size_t shape_idx = 0; shape_idx != model->get_index_buffers().size(); ++shape_idx)
	{
		for (size_t i = 0; i != model->get_vertex_buffers()[shape_idx]->get_number_of_elements(); ++i)
		{
			vertex& v = model->get_vertex_buffers()[shape_idx]->item(i);
			DirectX::XMFLOAT3 bary(i % 3 == 0, i % 3 == 1, i % 3 == 2);
			d3d_vertex vert = {
					DirectX::XMFLOAT4(v.position.x, v.position.y, v.position.z, 1.0f),
					DirectX::XMFLOAT4(v.normal.x, v.normal.y, v.normal.z, 0.0f),
					DirectX::XMFLOAT4(v.ambient.x, v.ambient.y, v.ambient.z, 1.0f),
					DirectX::XMFLOAT4(v.diffuse.x, v.diffuse.y, v.diffuse.z, 1.0f),
					DirectX::XMFLOAT4(v.emissive.x, v.emissive.y, v.emissive.z, 1.0f),
					bary};
			vertices.emplace_back(vert);
		}
		for (size_t i = 0; i != model->get_index_buffers()[shape_idx]->get_number_of_elements(); ++i)
		{
			UINT& ind = model->get_index_buffers()[shape_idx]->item(i);
			indices.push_back(ind + index_offset);
		}
		index_offset += static_cast<UINT>(model->get_vertex_buffers()[shape_idx]->get_number_of_elements());
	}

	const UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(d3d_vertex);
	const UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(UINT);
	vertex_stride = sizeof(d3d_vertex);
	vertex_buffer_stride = vbByteSize;
	index_buffer_size = ibByteSize;

	index_count = (UINT) indices.size();

	ComPtr<ID3DBlob> vertexBufferCpu = nullptr;
	ComPtr<ID3DBlob> indexBufferCpu = nullptr;

	ComPtr<ID3D12Resource> vertexBufferInt = nullptr;
	ComPtr<ID3D12Resource> indexBufferInt = nullptr;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));
	CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));
	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vbByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(vertex_buffer.GetAddressOf())));

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vbByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBufferInt.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = vertices.data();
	subResourceData.RowPitch = vbByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(command_list.Get(), vertex_buffer.Get(), vertexBufferInt.Get(), 0, 0, 1, &subResourceData);
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(ibByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(index_buffer.GetAddressOf())));

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(ibByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(indexBufferInt.GetAddressOf())));

	subResourceData.pData = indices.data();
	subResourceData.RowPitch = ibByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(index_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(command_list.Get(), index_buffer.Get(), indexBufferInt.Get(), 0, 0, 1, &subResourceData);
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(index_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = {inputLayout.data(), static_cast<UINT>(inputLayout.size())};
	psoDesc.pRootSignature = root_signature.Get();
	psoDesc.VS = {static_cast<BYTE*>(vsByteCode->GetBufferPointer()), vsByteCode->GetBufferSize()};
	psoDesc.PS = {static_cast<BYTE*>(psByteCode->GetBufferPointer()), psByteCode->GetBufferSize()};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state)));

	ThrowIfFailed(command_list->Close());
	command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(command_list.GetAddressOf()));
	wait_for_gpu();
}

void cg::renderer::dx12_renderer::load_assets()
{
	camera = std::make_shared<world::camera>();
	camera->set_position(DirectX::XMLoadFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(settings->camera_position.data())));
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_height(static_cast<float>(settings->height));
	camera->set_width(static_cast<float>(settings->width));
	camera->set_theta(settings->camera_theta);
	camera->set_phi(settings->camera_phi);
	camera->set_z_near(settings->camera_z_near);
	camera->set_z_far(settings->camera_z_far);

	model = std::make_shared<world::model>();
	model->load_obj(settings->model_path);
}

void cg::renderer::dx12_renderer::wait_for_gpu()
{
	++frame_index;

	ThrowIfFailed(command_queue->Signal(fence.Get(), frame_index));

	if (fence->GetCompletedValue() < frame_index)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(fence->SetEventOnCompletion(frame_index, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}