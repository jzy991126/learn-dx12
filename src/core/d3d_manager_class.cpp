#include"core/d3d_manager_class.h"

#include <DirectXColors.h>

#include "core/dx_exception_class.h"

namespace yang
{
	void D3dManager::CreateFence()
	{
		THROW_IF_FAILED(d3d12_device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
	}

	void D3dManager::CreateDevice()
	{
		THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory_)));
		THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12_device_)));
	}

	void D3dManager::GetDescriptorSize()
	{
		rtv_descriptor_size_ = d3d12_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		dsv_descriptor_size_ = d3d12_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		cbv_srv_uav_descriptor_size_ = d3d12_device_->
			GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void D3dManager::SetMsaa()
	{
		msaa_quality_level_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		msaa_quality_level_.SampleCount = 1;
		msaa_quality_level_.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msaa_quality_level_.NumQualityLevels = 0;
		THROW_IF_FAILED(
			d3d12_device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaa_quality_level_, sizeof(
				msaa_quality_level_)));
		assert(msaa_quality_level_.NumQualityLevels > 0);
	}

	void D3dManager::CreateCommandObject()
	{
		D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
		command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		THROW_IF_FAILED(d3d12_device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_)))
		THROW_IF_FAILED(
			d3d12_device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_)))
		THROW_IF_FAILED(
			d3d12_device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr,
				IID_PPV_ARGS(&
					command_list_)))
		command_list_->Close();
	}

	void D3dManager::CreateSwapChain()
	{
		swap_chain_.Reset();


		DXGI_SWAP_CHAIN_DESC swap_chain_desc;
		swap_chain_desc.BufferDesc.Width = window_->width();
		swap_chain_desc.BufferDesc.Height = window_->height();
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
		swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.OutputWindow = window_->window_handler();
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.Windowed = true;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swap_chain_desc.BufferCount = 2;
		swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		THROW_IF_FAILED(
			dxgi_factory_->CreateSwapChain(command_queue_.Get(), &swap_chain_desc, swap_chain_.GetAddressOf()))
	}

	void D3dManager::CreateDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtv_desc;
		rtv_desc.NumDescriptors = kFrameCount;
		rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtv_desc.NodeMask = 0;
		THROW_IF_FAILED(d3d12_device_->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&rtv_heap_)))

		D3D12_DESCRIPTOR_HEAP_DESC dsv_desc;
		dsv_desc.NumDescriptors = 1;
		dsv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsv_desc.NodeMask = 0;
		THROW_IF_FAILED(d3d12_device_->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(&dsv_heap_)))
	}

	void D3dManager::CreateRtv()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());
		for (uint i = 0; i < kFrameCount; i++)
		{
			swap_chain_->GetBuffer(i, IID_PPV_ARGS(swap_chain_buffer_[i].GetAddressOf()));
			d3d12_device_->CreateRenderTargetView(swap_chain_buffer_[i].Get(), nullptr, rtv_heap_handle);
			rtv_heap_handle.Offset(1, rtv_descriptor_size_);
		}
	}

	void D3dManager::CreateDsv()
	{
		D3D12_RESOURCE_DESC dsv_resource_desc;
		dsv_resource_desc.Alignment = 0;
		dsv_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dsv_resource_desc.DepthOrArraySize = 1;
		dsv_resource_desc.Width = window_->width();
		dsv_resource_desc.Height = window_->height();
		dsv_resource_desc.MipLevels = 1;
		dsv_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		dsv_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		dsv_resource_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsv_resource_desc.SampleDesc.Count = 1;
		dsv_resource_desc.SampleDesc.Quality = 0;
		CD3DX12_CLEAR_VALUE opt_clear;
		opt_clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		opt_clear.DepthStencil.Depth = 1;
		opt_clear.DepthStencil.Stencil = 0;
		auto s = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		THROW_IF_FAILED(
			d3d12_device_->CreateCommittedResource(&s, D3D12_HEAP_FLAG_NONE, &
				dsv_resource_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear, IID_PPV_ARGS(&depth_stencil_buffer_)));
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Texture2D.MipSlice = 0;
		d3d12_device_->CreateDepthStencilView(depth_stencil_buffer_.Get(),
		                                      &dsv_desc, dsv_heap_->GetCPUDescriptorHandleForHeapStart());
		auto trans_desc = CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_buffer_.Get(),
		                                                       D3D12_RESOURCE_STATE_COMMON,
		                                                       //转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
		                                                       D3D12_RESOURCE_STATE_DEPTH_WRITE);
		FlushCmdQueue();
		command_list_->Reset(command_allocator_.Get(), nullptr);
		command_list_->ResourceBarrier(1, //Barrier屏障个数
		                               &trans_desc);
		THROW_IF_FAILED(command_list_->Close()); //命令添加完后将其关闭
		ID3D12CommandList* cmd_lists[] = {command_list_.Get()}; //声明并定义命令列表数组
		command_queue_->ExecuteCommandLists(_countof(cmd_lists), cmd_lists); //将命令从命令列表传至命令队列
		FlushCmdQueue();
	}

	void D3dManager::CreateViewPortAndScissorRect()
	{
		//视口设置
		view_port_.TopLeftX = 0;
		view_port_.TopLeftY = 0;
		view_port_.Width = window_->width();
		view_port_.Height = window_->height();
		view_port_.MaxDepth = 1.0f;
		view_port_.MinDepth = 0.0f;
		//裁剪矩形设置（矩形外的像素都将被剔除）
		//前两个为左上点坐标，后两个为右下点坐标
		scissor_rect_.left = 0;
		scissor_rect_.top = 0;
		scissor_rect_.right = window_->width();
		scissor_rect_.bottom = window_->height();
	}

	void D3dManager::FlushCmdQueue()
	{
		current_fence_++; //CPU传完命令并关闭后，将当前围栏值+1
		command_queue_->Signal(fence_.Get(), current_fence_);
		//当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
		if (fence_->GetCompletedValue() < current_fence_) //如果小于，说明GPU没有处理完所有命令
		{
			HANDLE event_handle = CreateEvent(nullptr, false, false, L"FenceSetDone"); //创建事件
			fence_->SetEventOnCompletion(current_fence_, event_handle);
			//当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
			WaitForSingleObject(event_handle, INFINITE); //等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Enent需先设置再等待，
			//如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
			CloseHandle(event_handle);
		}
	}

	void D3dManager::Init()
	{
#if defined(DEBUG) || defined(_DEBUG)
		{
			ComPtr<ID3D12Debug> debug_controller;
			THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
			debug_controller->EnableDebugLayer();
		}
#endif

		CreateDevice();
		CreateFence();
		GetDescriptorSize();
		SetMsaa();
		CreateCommandObject();
		CreateSwapChain();
		CreateDescriptorHeap();
		CreateRtv();
		CreateDsv();
		CreateViewPortAndScissorRect();
	}

	D3dManager::D3dManager(Window* window): window_(window)
	{
		Init();
	}

	void D3dManager::Draw()
	{
		THROW_IF_FAILED(command_allocator_->Reset()); //重复使用记录命令的相关内存
		THROW_IF_FAILED(command_list_->Reset(command_allocator_.Get(), nullptr)); //复用命令列表及其内存
		UINT& ref_mCurrentBackBuffer = current_back_buffer_;
		auto rr = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(), //转换资源为后台缓冲区资源
		                                               D3D12_RESOURCE_STATE_PRESENT,
		                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
		command_list_->ResourceBarrier(1, &rr); //从呈现到渲染目标转换
		command_list_->RSSetViewports(1, &view_port_);
		command_list_->RSSetScissorRects(1, &scissor_rect_);
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			rtv_heap_->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtv_descriptor_size_);
		command_list_->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr); //清除RT背景色为暗红，并且不设置裁剪矩形
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
		command_list_->ClearDepthStencilView(dsv_handle, //DSV描述符句柄
		                                     D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //FLAG
		                                     1.0f, //默认深度值
		                                     0, //默认模板值
		                                     0, //裁剪矩形数量
		                                     nullptr); //裁剪矩形指针
		command_list_->OMSetRenderTargets(1, //待绑定的RTV数量
		                                  &rtv_handle, //指向RTV数组的指针
		                                  true, //RTV对象在堆内存中是连续存放的
		                                  &dsv_handle); //指向DSV的指针
		const auto trans_desc = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(),
		                                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
		                                                             D3D12_RESOURCE_STATE_PRESENT);
		command_list_->ResourceBarrier(1, &trans_desc); //从渲染目标到呈现
		//完成命令的记录关闭命令列表
		THROW_IF_FAILED(command_list_->Close());
		ID3D12CommandList* command_lists[] = {command_list_.Get()}; //声明并定义命令列表数组
		command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //将命令从命令列表传至命令队列
		THROW_IF_FAILED(swap_chain_->Present(0, 0));
		ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
		FlushCmdQueue();
	}
}
