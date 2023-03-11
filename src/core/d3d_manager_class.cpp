#include"core/d3d_manager_class.h"

#include <DirectXColors.h>
#include "core/game_timer_class.h"
#include "core/dx_exception_class.h"

namespace yang {
    void D3dManager::CreateFence() {
        THROW_IF_FAILED(d3d12_device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
    }

    void D3dManager::CreateDevice() {
        THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory_)));
        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12_device_)));
    }

    void D3dManager::GetDescriptorSize() {
        rtv_descriptor_size_ = d3d12_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        dsv_descriptor_size_ = d3d12_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        cbv_srv_uav_descriptor_size_ = d3d12_device_->
                GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    void D3dManager::SetMsaa() {
        msaa_quality_level_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaa_quality_level_.SampleCount = 1;
        msaa_quality_level_.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        msaa_quality_level_.NumQualityLevels = 0;
        THROW_IF_FAILED(
                d3d12_device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaa_quality_level_,
                                                   sizeof(
                                                           msaa_quality_level_)));
        assert(msaa_quality_level_.NumQualityLevels > 0);
    }

    void D3dManager::CreateCommandObject() {
        D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
        command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        THROW_IF_FAILED(d3d12_device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_)))
        THROW_IF_FAILED(
                d3d12_device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(&command_allocator_)))
        THROW_IF_FAILED(
                d3d12_device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr,
                                                 IID_PPV_ARGS(&
                                                                      command_list_)))
    }

    void D3dManager::CreateSwapChain() {
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

    void D3dManager::CreateDescriptorHeap() {
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

    void D3dManager::CreateRtv() {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());
        for (uint i = 0; i < kFrameCount; i++) {
            swap_chain_->GetBuffer(i, IID_PPV_ARGS(swap_chain_buffer_[i].GetAddressOf()));
            d3d12_device_->CreateRenderTargetView(swap_chain_buffer_[i].Get(), nullptr, rtv_heap_handle);
            rtv_heap_handle.Offset(1, rtv_descriptor_size_);
        }
    }

    void D3dManager::CreateDsv() {
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
                                                               dsv_resource_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear,
                                                       IID_PPV_ARGS(&depth_stencil_buffer_)));
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
//        command_list_->Close();
//		command_list_->ResourceBarrier(1, //Barrier���ϸ���
//		                               &trans_desc);
//		ID3D12CommandList* cmd_lists[] = {command_list_.Get()}; //���������������б�����
//		command_queue_->ExecuteCommandLists(_countof(cmd_lists), cmd_lists); //������������б����������
//		FlushCmdQueue();
    }

    void D3dManager::CreateViewPortAndScissorRect() {
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

    void D3dManager::FlushCmdQueue() {
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

    void D3dManager::Init() {
#if defined(DEBUG) || defined(_DEBUG)
        {
            ComPtr<ID3D12Debug5> debug_controller;
            THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
            debug_controller->EnableDebugLayer();
            debug_controller->SetEnableAutoName(true);
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

    void D3dManager::LoadAssets() {

        UINT obj_const_size = CalcConstantBufferByteSize(sizeof(ObjectConstants));
        D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc;
        cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbv_heap_desc.NumDescriptors = 1;
        cbv_heap_desc.NodeMask = 0;
        THROW_IF_FAILED(d3d12_device_->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&cbv_heap_)));



//���岢�������ĳ�����������Ȼ��õ����׵�ַ

//elementCountΪ1��1�������峣������Ԫ�أ���isConstantBufferΪture���ǳ�����������
        objCB = std::make_unique<UploadBuffer<ObjectConstants>>(d3d12_device_.Get(), 1, true);
//��ó����������׵�ַ
        D3D12_GPU_VIRTUAL_ADDRESS address;
        address = objCB->Resource()->GetGPUVirtualAddress();
//ͨ������������Ԫ��ƫ��ֵ�������յ�Ԫ�ص�ַ
        int cbElementIndex = 0;    //����������Ԫ���±�
        address += cbElementIndex * obj_const_size;
//����CBV������
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = address;
        cbvDesc.SizeInBytes = obj_const_size;
        d3d12_device_->CreateConstantBufferView(&cbvDesc, cbv_heap_->GetCPUDescriptorHandleForHeapStart());





//        THROW_IF_FAILED(command_list_->Close());
//        ID3D12CommandList* command_lists[] = {command_list_.Get()}; //���������������б�����
//        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //������������б����������
//        FlushCmdQueue();



    }

    ComPtr<ID3D12Resource> D3dManager::CreateDefaultBuffer(UINT64 byte_size, const void *init_data,
                                                           ComPtr<ID3D12Resource> &upload_buffer) {
        THROW_IF_FAILED(d3d12_device_->CreateCommittedResource(
                get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)), //�����ϴ������͵Ķ�
                D3D12_HEAP_FLAG_NONE,
                get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byte_size)),//����Ĺ��캯��������byteSize��������ΪĬ��ֵ������д
                D3D12_RESOURCE_STATE_GENERIC_READ,    //�ϴ��������Դ��Ҫ���Ƹ�Ĭ�϶ѣ������ǿɶ�״̬
                nullptr,    //�������ģ����Դ������ָ���Ż�ֵ
                IID_PPV_ARGS(&upload_buffer)));
        ComPtr<ID3D12Resource> defaultBuffer;
        THROW_IF_FAILED(d3d12_device_->CreateCommittedResource(
                get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),//����Ĭ�϶����͵Ķ�
                D3D12_HEAP_FLAG_NONE,
                get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byte_size)),
                D3D12_RESOURCE_STATE_COMMON,//Ĭ�϶�Ϊ���մ洢���ݵĵط���������ʱ��ʼ��Ϊ��ͨ״̬
                nullptr,
                IID_PPV_ARGS(&defaultBuffer)));

        //����Դ��COMMON״̬ת����COPY_DEST״̬��Ĭ�϶Ѵ�ʱ��Ϊ�������ݵ�Ŀ�꣩
        command_list_->ResourceBarrier(1,
                                       get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                                                           D3D12_RESOURCE_STATE_COMMON,
                                                                                           D3D12_RESOURCE_STATE_COPY_DEST)));

        //�����ݴ�CPU�ڴ濽����GPU����
        D3D12_SUBRESOURCE_DATA subResourceData;
        subResourceData.pData = init_data;
        subResourceData.RowPitch = byte_size;
        subResourceData.SlicePitch = subResourceData.RowPitch;
        //���ĺ���UpdateSubresources�������ݴ�CPU�ڴ濽�����ϴ��ѣ��ٴ��ϴ��ѿ�����Ĭ�϶ѡ�1����������Դ���±꣨ģ���ж��壬��Ϊ��2������Դ��
        UpdateSubresources<1>(command_list_.Get(), defaultBuffer.Get(), upload_buffer.Get(), 0, 0, 1, &subResourceData);

        //�ٴν���Դ��COPY_DEST״̬ת����GENERIC_READ״̬(����ֻ�ṩ����ɫ������)
        command_list_->ResourceBarrier(1,
                                       get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                                                                           D3D12_RESOURCE_STATE_GENERIC_READ)));

        return defaultBuffer;
    }

    D3dManager::D3dManager(Window *window) : window_(window) {
        Init();
        LoadAssets();
        BuildRootSignature();
        BuildByteCodeAndInputLayout();
        BuildGeometry();
        BuildPSO();

        THROW_IF_FAILED(command_list_->Close());
        ID3D12CommandList *command_list_s[] = {command_list_.Get()};
        command_queue_->ExecuteCommandLists(_countof(command_list_s), command_list_s);

        FlushCmdQueue();
    }

    void D3dManager::Draw() {
        THROW_IF_FAILED(command_allocator_->Reset()); //�ظ�ʹ�ü�¼���������ڴ�
        THROW_IF_FAILED(command_list_->Reset(command_allocator_.Get(), pipeline_state_obj_.Get())); //���������б����ڴ�
        UINT &ref_mCurrentBackBuffer = current_back_buffer_;
        command_list_->ResourceBarrier(1, get_rvalue_ptr(
                CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(), //ת����ԴΪ��̨��������Դ
                                                     D3D12_RESOURCE_STATE_PRESENT,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET))); //�ӳ��ֵ���ȾĿ��ת��
        command_list_->RSSetViewports(1, &view_port_);
        command_list_->RSSetScissorRects(1, &scissor_rect_);
        const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
                rtv_heap_->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtv_descriptor_size_);
        command_list_->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr); //���RT����ɫΪ���죬���Ҳ����òü�����
        const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                depth_stencil_buffer_.Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_DEPTH_WRITE);

        command_list_->ResourceBarrier(1, &barrier);

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
        ID3D12DescriptorHeap* descriHeaps[] = { cbv_heap_.Get() };//ע������֮���������飬����Ϊ�����ܰ���SRV��UAV������������ֻ�õ���CBV
        command_list_->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
//���ø�ǩ��
        command_list_->SetGraphicsRootSignature(root_signature_.Get());
//���ö��㻺����
        command_list_->IASetVertexBuffers(0, 1, get_rvalue_ptr(GetVbv()));
        command_list_->IASetIndexBuffer(get_rvalue_ptr(GetIbv()));
//��ͼԪ�������ʹ�����ˮ��
        command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//���ø���������
        command_list_->SetGraphicsRootDescriptorTable(0, //����������ʼ����
                                                cbv_heap_->GetGPUDescriptorHandleForHeapStart());

//���ƶ��㣨ͨ���������������ƣ�
        command_list_->DrawIndexedInstanced(indices.size(), //ÿ��ʵ��Ҫ���Ƶ�������
                                      1,	//ʵ��������
                                      0,	//��ʼ����λ��
                                      0,	//��������ʼ������ȫ�������е�λ��
                                      0);	//ʵ�����ĸ߼���������ʱ����Ϊ0

        const auto trans_desc = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(),
                                                                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                     D3D12_RESOURCE_STATE_PRESENT);
        command_list_->ResourceBarrier(1, &trans_desc); //����ȾĿ�굽����
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                depth_stencil_buffer_.Get(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                D3D12_RESOURCE_STATE_PRESENT);
        command_list_->ResourceBarrier(1, &barrier);
        //�������ļ�¼�ر������б�
        THROW_IF_FAILED(command_list_->Close());
        ID3D12CommandList *command_lists[] = {command_list_.Get()}; //���������������б�����
        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //������������б����������
        THROW_IF_FAILED(swap_chain_->Present(0, 0));
        ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
        FlushCmdQueue();
    }

    void D3dManager::BuildRootSignature() {
        CD3DX12_ROOT_PARAMETER slot_root_parameter[1];
        CD3DX12_DESCRIPTOR_RANGE cbv_table;
        cbv_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        slot_root_parameter[0].InitAsDescriptorTable(1, &cbv_table);
        CD3DX12_ROOT_SIGNATURE_DESC root_sig(1, slot_root_parameter, 0, nullptr,
                                             D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> serialized_root_sig = nullptr;
        ComPtr<ID3DBlob> error_blob = nullptr;

        HRESULT hr = D3D12SerializeRootSignature(&root_sig, D3D_ROOT_SIGNATURE_VERSION_1, &serialized_root_sig,
                                                 &error_blob);
        if (error_blob != nullptr) {
            OutputDebugStringA((char *) error_blob->GetBufferPointer());
        }
        THROW_IF_FAILED(hr);

        THROW_IF_FAILED(d3d12_device_->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(),
                                                           serialized_root_sig->GetBufferSize(),
                                                           IID_PPV_ARGS(&root_signature_)));


    }

    void D3dManager::BuildByteCodeAndInputLayout() {

        inputLayoutDesc =
                {
                        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
                };

        HRESULT hr = S_OK;
        vsBytecode = CompileShader(L"..\\shaders\\1\\color.hlsl", nullptr, "VS", "vs_5_0");
        psBytecode = CompileShader(L"..\\shaders\\1\\color.hlsl", nullptr, "PS", "ps_5_0");

    }

    void D3dManager::BuildGeometry() {


        auto vbByteSize = sizeof(Vertex) * vertices.size();
        auto ibByteSize = sizeof(std::uint16_t) * indices.size();
        THROW_IF_FAILED(D3DCreateBlob(vbByteSize, &vertex_buffer_cpu_));    //�������������ڴ�ռ�
        THROW_IF_FAILED(D3DCreateBlob(ibByteSize, &index_buffer_cpu_));    //�������������ڴ�ռ�
        CopyMemory(vertex_buffer_cpu_->GetBufferPointer(), vertices.data(), vbByteSize);    //���������ݿ���������ϵͳ�ڴ���
        CopyMemory(index_buffer_cpu_->GetBufferPointer(), indices.data(), ibByteSize);    //���������ݿ���������ϵͳ�ڴ���
        vertex_buffer_ = CreateDefaultBuffer(vbByteSize, vertices.data(), vertex_upload_buffer);
        index_buffer_ = CreateDefaultBuffer(ibByteSize, indices.data(), index_upload_buffer);

    }

    void D3dManager::BuildPSO() {

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        psoDesc.InputLayout = {inputLayoutDesc.data(), (UINT) inputLayoutDesc.size()};
        psoDesc.pRootSignature = root_signature_.Get();
        psoDesc.VS = {reinterpret_cast<BYTE *>(vsBytecode->GetBufferPointer()), vsBytecode->GetBufferSize()};
        psoDesc.PS = {reinterpret_cast<BYTE *>(psBytecode->GetBufferPointer()), psBytecode->GetBufferSize()};
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;    //0xffffffff,ȫ��������û������
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;    //��һ�����޷�������
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;    //��ʹ��4XMSAA
        psoDesc.SampleDesc.Quality = 0;    ////��ʹ��4XMSAA

        THROW_IF_FAILED(d3d12_device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state_obj_)));

    }

    D3D12_VERTEX_BUFFER_VIEW D3dManager::GetVbv()const  {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();//���㻺������Դ�����ַ
        vbv.SizeInBytes = sizeof(Vertex) * vertices.size();    //���㻺������С�����ж������ݴ�С��
        vbv.StrideInBytes = sizeof(Vertex);    //ÿ������Ԫ����ռ�õ��ֽ���

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW D3dManager::GetIbv() const {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = index_buffer_->GetGPUVirtualAddress();
        ibv.Format = DXGI_FORMAT_R16_UINT;
        ibv.SizeInBytes = sizeof(std::uint16_t)*indices.size();

        return ibv;
    }

    void D3dManager::Update() {

        ObjectConstants objConstants;

        auto& gt = yang::GameTimer::GetInstance();
        //�����۲����
        float x = 5.0f;
        float y = 5.0f;
        float z = 5.0f;
        float r = 5.0f;
        x *= sinf(gt.total_time());
        z = sqrt(r * r - x * x);
        XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
        XMVECTOR target = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

        //����ͶӰ����
        XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, float(window_->width()) / window_->height(), 1.0f, 1000.0f);
        //XMStoreFloat4x4(&proj, p);
        //�����������
//        Id
//        XMMATRIX w
        //�������
        XMMATRIX WVP_Matrix = v * p;
        //XMMATRIX��ֵ��XMFLOAT4X4
        XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(WVP_Matrix));
        //�����ݿ�����GPU����
        objCB->CopyData(0, objConstants);

    }
}
