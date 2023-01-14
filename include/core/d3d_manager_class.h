#pragma once
#include <Windows.h>
#include "core/window_manager_class.h"
#include <wrl.h>
#include "dependency/d3dx12.h"
#include <dxgi.h>
#include <dxgi1_6.h>
#include <d3d12.h>
using namespace Microsoft::WRL;
namespace yang
{
	class D3dManager
	{
		static constexpr uint kFrameCount = 2;
	private:
		Window* window_;
	private:
		ComPtr<IDXGIFactory4> dxgi_factory_;
		ComPtr<ID3D12Device> d3d12_device_;
		ComPtr<ID3D12Fence> fence_;
		ComPtr<ID3D12DescriptorHeap> rtv_heap_, dsv_heap_;
		ComPtr<ID3D12Resource> swap_chain_buffer_[kFrameCount];
		ComPtr<IDXGISwapChain> swap_chain_;
		ComPtr<ID3D12CommandAllocator> command_allocator_;
		ComPtr<ID3D12GraphicsCommandList> command_list_;
		ComPtr<ID3D12CommandQueue> command_queue_;
		ComPtr<ID3D12Resource> depth_stencil_buffer_;


		D3D12_VIEWPORT view_port_{};
		D3D12_RECT scissor_rect_{};
		UINT current_back_buffer_ = 0;

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaa_quality_level_{};
		uint rtv_descriptor_size_{}, dsv_descriptor_size_{}, cbv_srv_uav_descriptor_size_{};
		int current_fence_ = 0; 


	private:
		void CreateFence();
		void CreateDevice();
		void GetDescriptorSize();
		void SetMsaa();
		void CreateCommandObject();
		void CreateSwapChain();
		void CreateDescriptorHeap();
		void CreateRtv();
		void CreateDsv();
		void CreateViewPortAndScissorRect();
		void FlushCmdQueue();
		void Init();

	public:
		explicit D3dManager(Window* window);
		void Draw();
	};

}
