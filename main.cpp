#include "window_manager_class.h"
#include "game_timer_class.h"
#include <iostream>
#include <functional>
#include <wrl.h>
#include "d3dx12.h"
#include <dxgi.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include "dx_exception_class.h"
#include <DirectXColors.h>

using namespace Microsoft::WRL;

ComPtr<IDXGIFactory4> dxgi_factory;
ComPtr<ID3D12Device> d3d12_device;
ComPtr<ID3D12Fence> fence;
ComPtr<ID3D12DescriptorHeap> rtv_heap, dsv_heap;
ComPtr<ID3D12Resource> swap_chain_buffer[2];
ComPtr<IDXGISwapChain> swap_chain;
ComPtr<ID3D12CommandAllocator> cmd_allocator;
ComPtr<ID3D12GraphicsCommandList> cmd_list;
ComPtr<ID3D12CommandQueue> cmd_queue;
ComPtr<ID3D12Resource> depth_stencil_buffer;

D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaa_quality_level;


HWND main_window;
D3D12_VIEWPORT view_port;
D3D12_RECT scissor_rect;
UINT mCurrentBackBuffer = 0;


uint rtv_descriptor_size, dsv_descriptor_size, cbv_srv_uav_descriptor_size;
int current_fence = 0; //��ʼCPU�ϵ�Χ����Ϊ0
void FlushCmdQueue();

void CreateDevice()
{
	THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));
	THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12_device)));
}

void CreateFence()
{
	THROW_IF_FAILED(d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void GetDescriptorSize()
{
	rtv_descriptor_size = d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsv_descriptor_size = d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uav_descriptor_size = d3d12_device->
		GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SetMsaa()
{
	msaa_quality_level.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msaa_quality_level.SampleCount = 1;
	msaa_quality_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaa_quality_level.NumQualityLevels = 0;
	THROW_IF_FAILED(
		d3d12_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaa_quality_level, sizeof(
			msaa_quality_level)));
	assert(msaa_quality_level.NumQualityLevels > 0);
}

void CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	THROW_IF_FAILED(d3d12_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&cmd_queue)))
	THROW_IF_FAILED(d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocator)))
	THROW_IF_FAILED(
		d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocator.Get(), nullptr, IID_PPV_ARGS(&
			cmd_list)))
	cmd_list->Close();
}


void CreateSwapChain()
{
	swap_chain.Reset();


	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	swap_chain_desc.BufferDesc.Width = 1280;
	swap_chain_desc.BufferDesc.Height = 720;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = main_window;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	THROW_IF_FAILED(dxgi_factory->CreateSwapChain(cmd_queue.Get(), &swap_chain_desc, swap_chain.GetAddressOf()))
}

void CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtv_desc;
	rtv_desc.NumDescriptors = 2;
	rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_desc.NodeMask = 0;
	THROW_IF_FAILED(d3d12_device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&rtv_heap)))

	D3D12_DESCRIPTOR_HEAP_DESC dsv_desc;
	dsv_desc.NumDescriptors = 1;
	dsv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_desc.NodeMask = 0;
	THROW_IF_FAILED(d3d12_device->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(&dsv_heap)))
}

void CreateRtv()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
	for (uint i = 0; i < 2; i++)
	{
		swap_chain->GetBuffer(i, IID_PPV_ARGS(swap_chain_buffer[i].GetAddressOf()));
		d3d12_device->CreateRenderTargetView(swap_chain_buffer[i].Get(), nullptr, rtv_heap_handle);
		rtv_heap_handle.Offset(1, rtv_descriptor_size);
	}
}


void CreateDsv()
{
	D3D12_RESOURCE_DESC dsv_resource_desc;
	dsv_resource_desc.Alignment = 0;
	dsv_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsv_resource_desc.DepthOrArraySize = 1;
	dsv_resource_desc.Width = 1280;
	dsv_resource_desc.Height = 720;
	dsv_resource_desc.MipLevels = 1;
	dsv_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsv_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	dsv_resource_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsv_resource_desc.SampleDesc.Count = 1;
	dsv_resource_desc.SampleDesc.Quality = 0;
	CD3DX12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1;
	optClear.DepthStencil.Stencil = 0;
	auto s = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	THROW_IF_FAILED(
		d3d12_device->CreateCommittedResource(&s, D3D12_HEAP_FLAG_NONE, &
			dsv_resource_desc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&depth_stencil_buffer)));
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;
	d3d12_device->CreateDepthStencilView(depth_stencil_buffer.Get(),
	                                     &dsv_desc, dsv_heap->GetCPUDescriptorHandleForHeapStart());
	auto ss = CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_buffer.Get(),
	                                               D3D12_RESOURCE_STATE_COMMON,
	                                               //ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
	                                               D3D12_RESOURCE_STATE_DEPTH_WRITE);
	FlushCmdQueue();
	cmd_list->Reset(cmd_allocator.Get(), nullptr);
	cmd_list->ResourceBarrier(1, //Barrier���ϸ���
	                          &ss);
	THROW_IF_FAILED(cmd_list->Close()); //������������ر�
	ID3D12CommandList* cmd_lists[] = {cmd_list.Get()}; //���������������б�����
	cmd_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists); //������������б����������
	FlushCmdQueue();
}

void FlushCmdQueue()
{
	current_fence++; //CPU��������رպ󣬽���ǰΧ��ֵ+1
	cmd_queue->Signal(fence.Get(), current_fence); //��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
	if (fence->GetCompletedValue() < current_fence) //���С�ڣ�˵��GPUû�д�������������
	{
		HANDLE event_handle = CreateEvent(nullptr, false, false, L"FenceSetDone"); //�����¼�
		fence->SetEventOnCompletion(current_fence, event_handle);
		//��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
		WaitForSingleObject(event_handle, INFINITE); //�ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
		//���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
		CloseHandle(event_handle);
	}
}

void CreateViewPortAndScissorRect()
{
	//�ӿ�����
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = 1280;
	view_port.Height = 720;
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	//�ü��������ã�����������ض������޳���
	//ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = 1280;
	scissor_rect.bottom = 720;
}

void InitDirect3D()
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

void Draw()
{
	THROW_IF_FAILED(cmd_allocator->Reset()); //�ظ�ʹ�ü�¼���������ڴ�
	THROW_IF_FAILED(cmd_list->Reset(cmd_allocator.Get(), nullptr)); //���������б����ڴ�
	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	auto rr = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer[ref_mCurrentBackBuffer].Get(), //ת����ԴΪ��̨��������Դ
	                                               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmd_list->ResourceBarrier(1, &rr); //�ӳ��ֵ���ȾĿ��ת��
	cmd_list->RSSetViewports(1, &view_port);
	cmd_list->RSSetScissorRects(1, &scissor_rect);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtv_heap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtv_descriptor_size);
	cmd_list->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr); //���RT����ɫΪ���죬���Ҳ����òü�����
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->ClearDepthStencilView(dsvHandle, //DSV���������
	                                D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //FLAG
	                                1.0f, //Ĭ�����ֵ
	                                0, //Ĭ��ģ��ֵ
	                                0, //�ü���������
	                                nullptr); //�ü�����ָ��
	cmd_list->OMSetRenderTargets(1, //���󶨵�RTV����
	                             &rtvHandle, //ָ��RTV�����ָ��
	                             true, //RTV�����ڶ��ڴ�����������ŵ�
	                             &dsvHandle); //ָ��DSV��ָ��
	auto uu = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer[ref_mCurrentBackBuffer].Get(),
	                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmd_list->ResourceBarrier(1, &uu); //����ȾĿ�굽����
	//�������ļ�¼�ر������б�
	THROW_IF_FAILED(cmd_list->Close());
	ID3D12CommandList* commandLists[] = {cmd_list.Get()}; //���������������б�����
	cmd_queue->ExecuteCommandLists(_countof(commandLists), commandLists); //������������б����������
	THROW_IF_FAILED(swap_chain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
	FlushCmdQueue();
}

void down(WPARAM a, uint x, uint y)
{
	std::cout << x << " " << y << std::endl;
}

int main()
{
	auto& window_manager = yang::WindowManager::GetInstance();
	window_manager.set_mouse_right_down_func(down);
	const auto window = window_manager.CreateYWindow("test", 800, 800);
	main_window = window->window_handler();
	auto game_timer = yang::GameTimer::GetInstance();

	InitDirect3D();
	window_manager.Show();
	game_timer.Reset();
	while (!yang::WindowManager::GetInstance().ShouldClose())
	{
		game_timer.Tick();
		window_manager.ProcessMessage();

		Draw();
	}
	window_manager.Terminate();
	return 0;
}
