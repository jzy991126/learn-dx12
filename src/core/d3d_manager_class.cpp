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
		                                                       //ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
		                                                       D3D12_RESOURCE_STATE_DEPTH_WRITE);
		FlushCmdQueue();
		command_list_->Reset(command_allocator_.Get(), nullptr);
		command_list_->ResourceBarrier(1, //Barrier���ϸ���
		                               &trans_desc);
		THROW_IF_FAILED(command_list_->Close()); //������������ر�
		ID3D12CommandList* cmd_lists[] = {command_list_.Get()}; //���������������б�����
		command_queue_->ExecuteCommandLists(_countof(cmd_lists), cmd_lists); //������������б����������
		FlushCmdQueue();
	}

	void D3dManager::CreateViewPortAndScissorRect()
	{
		//�ӿ�����
		view_port_.TopLeftX = 0;
		view_port_.TopLeftY = 0;
		view_port_.Width = window_->width();
		view_port_.Height = window_->height();
		view_port_.MaxDepth = 1.0f;
		view_port_.MinDepth = 0.0f;
		//�ü��������ã�����������ض������޳���
		//ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
		scissor_rect_.left = 0;
		scissor_rect_.top = 0;
		scissor_rect_.right = window_->width();
		scissor_rect_.bottom = window_->height();
	}

	void D3dManager::FlushCmdQueue()
	{
		current_fence_++; //CPU��������رպ󣬽���ǰΧ��ֵ+1
		command_queue_->Signal(fence_.Get(), current_fence_);
		//��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
		if (fence_->GetCompletedValue() < current_fence_) //���С�ڣ�˵��GPUû�д�������������
		{
			HANDLE event_handle = CreateEvent(nullptr, false, false, L"FenceSetDone"); //�����¼�
			fence_->SetEventOnCompletion(current_fence_, event_handle);
			//��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
			WaitForSingleObject(event_handle, INFINITE); //�ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
			//���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
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
		THROW_IF_FAILED(command_allocator_->Reset()); //�ظ�ʹ�ü�¼���������ڴ�
		THROW_IF_FAILED(command_list_->Reset(command_allocator_.Get(), nullptr)); //���������б����ڴ�
		UINT& ref_mCurrentBackBuffer = current_back_buffer_;
		auto rr = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(), //ת����ԴΪ��̨��������Դ
		                                               D3D12_RESOURCE_STATE_PRESENT,
		                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
		command_list_->ResourceBarrier(1, &rr); //�ӳ��ֵ���ȾĿ��ת��
		command_list_->RSSetViewports(1, &view_port_);
		command_list_->RSSetScissorRects(1, &scissor_rect_);
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			rtv_heap_->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtv_descriptor_size_);
		command_list_->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr); //���RT����ɫΪ���죬���Ҳ����òü�����
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
		command_list_->ClearDepthStencilView(dsv_handle, //DSV���������
		                                     D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //FLAG
		                                     1.0f, //Ĭ�����ֵ
		                                     0, //Ĭ��ģ��ֵ
		                                     0, //�ü���������
		                                     nullptr); //�ü�����ָ��
		command_list_->OMSetRenderTargets(1, //���󶨵�RTV����
		                                  &rtv_handle, //ָ��RTV�����ָ��
		                                  true, //RTV�����ڶ��ڴ�����������ŵ�
		                                  &dsv_handle); //ָ��DSV��ָ��
		const auto trans_desc = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(),
		                                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
		                                                             D3D12_RESOURCE_STATE_PRESENT);
		command_list_->ResourceBarrier(1, &trans_desc); //����ȾĿ�굽����
		//�������ļ�¼�ر������б�
		THROW_IF_FAILED(command_list_->Close());
		ID3D12CommandList* command_lists[] = {command_list_.Get()}; //���������������б�����
		command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //������������б����������
		THROW_IF_FAILED(swap_chain_->Present(0, 0));
		ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
		FlushCmdQueue();
	}
}
