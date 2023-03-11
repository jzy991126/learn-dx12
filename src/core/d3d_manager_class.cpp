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
                //转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
                                                               D3D12_RESOURCE_STATE_DEPTH_WRITE);
//        command_list_->Close();
//		command_list_->ResourceBarrier(1, //Barrier屏障个数
//		                               &trans_desc);
//		ID3D12CommandList* cmd_lists[] = {command_list_.Get()}; //声明并定义命令列表数组
//		command_queue_->ExecuteCommandLists(_countof(cmd_lists), cmd_lists); //将命令从命令列表传至命令队列
//		FlushCmdQueue();
    }

    void D3dManager::CreateViewPortAndScissorRect() {
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

    void D3dManager::FlushCmdQueue() {
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



//定义并获得物体的常量缓冲区，然后得到其首地址

//elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
        objCB = std::make_unique<UploadBuffer<ObjectConstants>>(d3d12_device_.Get(), 1, true);
//获得常量缓冲区首地址
        D3D12_GPU_VIRTUAL_ADDRESS address;
        address = objCB->Resource()->GetGPUVirtualAddress();
//通过常量缓冲区元素偏移值计算最终的元素地址
        int cbElementIndex = 0;    //常量缓冲区元素下标
        address += cbElementIndex * obj_const_size;
//创建CBV描述符
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = address;
        cbvDesc.SizeInBytes = obj_const_size;
        d3d12_device_->CreateConstantBufferView(&cbvDesc, cbv_heap_->GetCPUDescriptorHandleForHeapStart());





//        THROW_IF_FAILED(command_list_->Close());
//        ID3D12CommandList* command_lists[] = {command_list_.Get()}; //声明并定义命令列表数组
//        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //将命令从命令列表传至命令队列
//        FlushCmdQueue();



    }

    ComPtr<ID3D12Resource> D3dManager::CreateDefaultBuffer(UINT64 byte_size, const void *init_data,
                                                           ComPtr<ID3D12Resource> &upload_buffer) {
        THROW_IF_FAILED(d3d12_device_->CreateCommittedResource(
                get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)), //创建上传堆类型的堆
                D3D12_HEAP_FLAG_NONE,
                get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byte_size)),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
                D3D12_RESOURCE_STATE_GENERIC_READ,    //上传堆里的资源需要复制给默认堆，所以是可读状态
                nullptr,    //不是深度模板资源，不用指定优化值
                IID_PPV_ARGS(&upload_buffer)));
        ComPtr<ID3D12Resource> defaultBuffer;
        THROW_IF_FAILED(d3d12_device_->CreateCommittedResource(
                get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),//创建默认堆类型的堆
                D3D12_HEAP_FLAG_NONE,
                get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byte_size)),
                D3D12_RESOURCE_STATE_COMMON,//默认堆为最终存储数据的地方，所以暂时初始化为普通状态
                nullptr,
                IID_PPV_ARGS(&defaultBuffer)));

        //将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
        command_list_->ResourceBarrier(1,
                                       get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                                                           D3D12_RESOURCE_STATE_COMMON,
                                                                                           D3D12_RESOURCE_STATE_COPY_DEST)));

        //将数据从CPU内存拷贝到GPU缓存
        D3D12_SUBRESOURCE_DATA subResourceData;
        subResourceData.pData = init_data;
        subResourceData.RowPitch = byte_size;
        subResourceData.SlicePitch = subResourceData.RowPitch;
        //核心函数UpdateSubresources，将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆。1是最大的子资源的下标（模板中定义，意为有2个子资源）
        UpdateSubresources<1>(command_list_.Get(), defaultBuffer.Get(), upload_buffer.Get(), 0, 0, 1, &subResourceData);

        //再次将资源从COPY_DEST状态转换到GENERIC_READ状态(现在只提供给着色器访问)
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
        THROW_IF_FAILED(command_allocator_->Reset()); //重复使用记录命令的相关内存
        THROW_IF_FAILED(command_list_->Reset(command_allocator_.Get(), pipeline_state_obj_.Get())); //复用命令列表及其内存
        UINT &ref_mCurrentBackBuffer = current_back_buffer_;
        command_list_->ResourceBarrier(1, get_rvalue_ptr(
                CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(), //转换资源为后台缓冲区资源
                                                     D3D12_RESOURCE_STATE_PRESENT,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET))); //从呈现到渲染目标转换
        command_list_->RSSetViewports(1, &view_port_);
        command_list_->RSSetScissorRects(1, &scissor_rect_);
        const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
                rtv_heap_->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtv_descriptor_size_);
        command_list_->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr); //清除RT背景色为暗红，并且不设置裁剪矩形
        const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                depth_stencil_buffer_.Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_DEPTH_WRITE);

        command_list_->ResourceBarrier(1, &barrier);

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
        ID3D12DescriptorHeap* descriHeaps[] = { cbv_heap_.Get() };//注意这里之所以是数组，是因为还可能包含SRV和UAV，而这里我们只用到了CBV
        command_list_->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
//设置根签名
        command_list_->SetGraphicsRootSignature(root_signature_.Get());
//设置顶点缓冲区
        command_list_->IASetVertexBuffers(0, 1, get_rvalue_ptr(GetVbv()));
        command_list_->IASetIndexBuffer(get_rvalue_ptr(GetIbv()));
//将图元拓扑类型传入流水线
        command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//设置根描述符表
        command_list_->SetGraphicsRootDescriptorTable(0, //根参数的起始索引
                                                cbv_heap_->GetGPUDescriptorHandleForHeapStart());

//绘制顶点（通过索引缓冲区绘制）
        command_list_->DrawIndexedInstanced(indices.size(), //每个实例要绘制的索引数
                                      1,	//实例化个数
                                      0,	//起始索引位置
                                      0,	//子物体起始索引在全局索引中的位置
                                      0);	//实例化的高级技术，暂时设置为0

        const auto trans_desc = CD3DX12_RESOURCE_BARRIER::Transition(swap_chain_buffer_[ref_mCurrentBackBuffer].Get(),
                                                                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                     D3D12_RESOURCE_STATE_PRESENT);
        command_list_->ResourceBarrier(1, &trans_desc); //从渲染目标到呈现
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                depth_stencil_buffer_.Get(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                D3D12_RESOURCE_STATE_PRESENT);
        command_list_->ResourceBarrier(1, &barrier);
        //完成命令的记录关闭命令列表
        THROW_IF_FAILED(command_list_->Close());
        ID3D12CommandList *command_lists[] = {command_list_.Get()}; //声明并定义命令列表数组
        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists); //将命令从命令列表传至命令队列
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
        THROW_IF_FAILED(D3DCreateBlob(vbByteSize, &vertex_buffer_cpu_));    //创建顶点数据内存空间
        THROW_IF_FAILED(D3DCreateBlob(ibByteSize, &index_buffer_cpu_));    //创建索引数据内存空间
        CopyMemory(vertex_buffer_cpu_->GetBufferPointer(), vertices.data(), vbByteSize);    //将顶点数据拷贝至顶点系统内存中
        CopyMemory(index_buffer_cpu_->GetBufferPointer(), indices.data(), ibByteSize);    //将索引数据拷贝至索引系统内存中
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
        psoDesc.SampleMask = UINT_MAX;    //0xffffffff,全部采样，没有遮罩
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;    //归一化的无符号整型
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;    //不使用4XMSAA
        psoDesc.SampleDesc.Quality = 0;    ////不使用4XMSAA

        THROW_IF_FAILED(d3d12_device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state_obj_)));

    }

    D3D12_VERTEX_BUFFER_VIEW D3dManager::GetVbv()const  {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
        vbv.SizeInBytes = sizeof(Vertex) * vertices.size();    //顶点缓冲区大小（所有顶点数据大小）
        vbv.StrideInBytes = sizeof(Vertex);    //每个顶点元素所占用的字节数

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
        //构建观察矩阵
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

        //构建投影矩阵
        XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, float(window_->width()) / window_->height(), 1.0f, 1000.0f);
        //XMStoreFloat4x4(&proj, p);
        //构建世界矩阵
//        Id
//        XMMATRIX w
        //矩阵计算
        XMMATRIX WVP_Matrix = v * p;
        //XMMATRIX赋值给XMFLOAT4X4
        XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(WVP_Matrix));
        //将数据拷贝至GPU缓存
        objCB->CopyData(0, objConstants);

    }
}
