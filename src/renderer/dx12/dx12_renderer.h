#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "renderer/renderer.h"

#include <DirectXMath.h>
#include <Windows.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>


using namespace Microsoft::WRL;

namespace cg::renderer
{
	class dx12_renderer : public renderer
	{
	public:
		virtual void init();
		virtual void destroy();

		virtual void update();
		virtual void render();

	protected:
		static const UINT frame_number = 2;

		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> command_queue;
		ComPtr<IDXGISwapChain> swap_chain;
		ComPtr<ID3D12DescriptorHeap> rtv_heap;
		ComPtr<ID3D12DescriptorHeap> dsv_heap;
		ComPtr<ID3D12DescriptorHeap> cbv_heap;
		ComPtr<ID3D12Resource> render_targets[frame_number];
		ComPtr<ID3D12Resource> depth_stencil_buffer;
		ComPtr<ID3D12CommandAllocator> command_allocator;
		ComPtr<ID3D12PipelineState> pipeline_state;
		ComPtr<ID3D12GraphicsCommandList> command_list;

		ComPtr<ID3D12RootSignature> root_signature;
		CD3DX12_VIEWPORT view_port;
		CD3DX12_RECT scissor_rect;
		UINT current_render_target_idx;

		ComPtr<ID3D12Resource> vertex_buffer;
		ComPtr<ID3D12Resource> index_buffer;
		UINT vertex_stride;
		UINT vertex_buffer_stride;

		DirectX::XMFLOAT4X4 world_view_projection;
		ComPtr<ID3D12Resource> constant_buffer;
		UINT8* constant_buffer_location;

		UINT frame_index;
		HANDLE fence_event;
		ComPtr<ID3D12Fence> fence;

		UINT index_buffer_size;
		UINT index_count;

		void load_pipeline();
		void load_assets();
		void populate_command_list();

		void wait_for_gpu();
	};
}// namespace cg::renderer