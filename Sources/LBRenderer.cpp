#include "stdafx.h"
#include "LBRenderer.h"
#include "LBEntity.h"
#include "LBModel.h"
#include "LBMesh.h"
#include "LBMaterial.h"

namespace LB
{
	Renderer::Renderer(HWND hwnd, bool useWARPDevice) : _windowWidth(0), _windowHeight(0), _needsUpdateForWindowSizeChange(true), _windowVisible(true), _rtvDescriptorSize(0)
	{
		ZeroMemory(_fenceValues, sizeof(_fenceValues));
		CreatePipeline(hwnd, useWARPDevice);
		LoadAssets();
	}

	Renderer::~Renderer()
	{
		// Wait for the GPU to be done with all resources.
		WaitForGpu();

		// Fullscreen state should always be false before exiting the app.
		ThrowIfFailed(_swapChain->SetFullscreenState(FALSE, nullptr));

		CloseHandle(_fenceEvent);
	}

	// Render the scene.
	void Renderer::Render(const std::vector<Entity*> &entities)
	{
		if(!_windowVisible)
			return;

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(_commandAllocators[_frameIndex]->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(_commandList->Reset(_commandAllocators[_frameIndex].Get(), NULL));

		if(_needsUpdateForWindowSizeChange)
		{
			CreateFramebuffers();
		}

		// Set necessary state.
		_commandList->SetGraphicsRootSignature(_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { _cbvHeap.Get() };
		_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		_commandList->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());

		_commandList->RSSetViewports(1, &_viewport);
		_commandList->RSSetScissorRects(1, &_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize);
		_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.6f, 0.8f, 1.0f };
		_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		for(Entity *ent : entities)
		{
			_commandList->SetPipelineState(ent->_model->_material->_pipelineState.Get());
			_commandList->IASetPrimitiveTopology(ent->_model->_mesh->_topology);
			_commandList->IASetVertexBuffers(0, 1, &(ent->_model->_mesh->_vertexBufferView));
			if(ent->_model->_mesh->_indexBuffer)
			{
				_commandList->IASetIndexBuffer(&(ent->_model->_mesh->_indexBufferView));
				_commandList->DrawIndexedInstanced(ent->_model->_mesh->_indexCount, 1, 0, 0, 0);
			}
			else
			{
				_commandList->DrawInstanced(ent->_model->_mesh->_vertexCount, 1, 0, 0);
			}
		}

		// Indicate that the back buffer will now be used to present.
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		ThrowIfFailed(_commandList->Close());

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		ThrowIfFailed(_swapChain->Present(0, 0));

		MoveToNextFrame();
	}

	void Renderer::SetWindowSize(int width, int height, bool minimized)
	{
		// Determine if the swap buffers and other resources need to be resized or not.
		if((width != _windowWidth || height != _windowHeight) && !minimized)
		{
			_windowWidth = width;
			_windowHeight = height;

			// Flush all current GPU commands.
			WaitForGpu();

			// Release the resources holding references to the swap chain (requirement of
			// IDXGISwapChain::ResizeBuffers) and reset the frame fence values to the
			// current fence value.
			for(UINT n = 0; n < FrameCount; n++)
			{
				_renderTargets[n].Reset();
				_fenceValues[n] = _fenceValues[_frameIndex];
			}

			// Resize the swap chain to the desired dimensions.
			DXGI_SWAP_CHAIN_DESC desc = {};
			_swapChain->GetDesc(&desc);
			ThrowIfFailed(_swapChain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags));

			// Reset the frame index to the current back buffer index.
			_frameIndex = _swapChain->GetCurrentBackBufferIndex();

			_needsUpdateForWindowSizeChange = true;
		}

		_windowVisible = !minimized;
	}

	void Renderer::ToggleFullscreen()
	{
		BOOL fullscreenState;
		ThrowIfFailed(_swapChain->GetFullscreenState(&fullscreenState, nullptr));
		if(FAILED(_swapChain->SetFullscreenState(!fullscreenState, nullptr)))
		{
			// Transitions to fullscreen mode can fail when running apps over
			// terminal services or for some other unexpected reason.  Consider
			// notifying the user in some way when this happens.
			OutputDebugString(L"Fullscreen transition failed");
			assert(false);
		}
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> Renderer::GetPSOForDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc)
	{
		desc->pRootSignature = _rootSignature.Get();
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(_device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&pso)));
		return pso;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::UploadVertexData(void *data, long dataSize)
	{
		WaitForGpu();

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(_commandAllocators[_frameIndex]->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(_commandList->Reset(_commandAllocators[_frameIndex].Get(), NULL));

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUpload;

		ThrowIfFailed(_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)));

		ThrowIfFailed(_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUpload)));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		UINT8* pVertexDataBegin;
		ThrowIfFailed(vertexBufferUpload->Map(0, &CD3DX12_RANGE(0, dataSize), reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, data, dataSize);
		vertexBufferUpload->Unmap(0, nullptr);

		_commandList->CopyBufferRegion(vertexBuffer.Get(), 0, vertexBufferUpload.Get(), 0, dataSize);
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForGpu();

		return vertexBuffer;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::UploadIndexData(void *data, long dataSize)
	{
		WaitForGpu();

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(_commandAllocators[_frameIndex]->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(_commandList->Reset(_commandAllocators[_frameIndex].Get(), NULL));

		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUpload;

		ThrowIfFailed(_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&indexBuffer)));

		ThrowIfFailed(_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBufferUpload)));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		UINT8* pIndexDataBegin;
		ThrowIfFailed(indexBufferUpload->Map(0, &CD3DX12_RANGE(0, dataSize), reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, data, dataSize);
		indexBufferUpload->Unmap(0, nullptr);

		_commandList->CopyBufferRegion(indexBuffer.Get(), 0, indexBufferUpload.Get(), 0, dataSize);
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForGpu();

		return indexBuffer;
	}

	Renderer::CreateConstantBufferView()
	{
		ComPtr<ID3D12Resource> constantBuffer;
	ConstantBuffer m_constantBufferData;
	UINT8* m_pCbvDataBegin;

		// Create the constant buffer.
		{
			ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&constantBuffer)));

			 // Describe and create a constant buffer view.
			 D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			 cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
			 cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;	// CB size is required to be 256-byte aligned.
			 m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

			 // Initialize and map the constant buffers. We don't unmap this until the
			 // app closes. Keeping things mapped for the lifetime of the resource is okay.
			 ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));
			 ThrowIfFailed(constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pCbvDataBegin)));
			 memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
		}
	}

	void Renderer::CreateFramebuffers()
	{
		_viewport.Width = static_cast<float>(_windowWidth);
		_viewport.Height = static_cast<float>(_windowHeight);
		_viewport.MaxDepth = 1.0f;

		_scissorRect.right = static_cast<LONG>(_windowWidth);
		_scissorRect.bottom = static_cast<LONG>(_windowHeight);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for(UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(&_renderTargets[n])));
			_device->CreateRenderTargetView(_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, _rtvDescriptorSize);
		}

		_needsUpdateForWindowSizeChange = false;
	}

	void Renderer::CreatePipeline(HWND hwnd, bool useWARPDevice)
	{
		using namespace Microsoft::WRL;

#ifdef _DEBUG
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif

		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

		if(useWARPDevice)
		{
			ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&_device)
				));
		}
		else
		{
			ComPtr<IDXGIAdapter1> hardwareAdapter;
			GetHardwareAdapter(factory.Get(), &hardwareAdapter);

			ThrowIfFailed(D3D12CreateDevice(
				hardwareAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&_device)
				));
		}

		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));

		RECT windowRect;
		GetClientRect(hwnd, &windowRect);
		_windowWidth = windowRect.right - windowRect.left;
		_windowHeight = windowRect.bottom - windowRect.top;

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.BufferDesc.Width = _windowWidth;
		swapChainDesc.BufferDesc.Height = _windowHeight;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;

		ComPtr<IDXGISwapChain> swapChain;
		ThrowIfFailed(factory->CreateSwapChain(
			_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
			&swapChainDesc,
			&swapChain
			));

		ThrowIfFailed(swapChain.As(&_swapChain));

		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

			_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			ThrowIfFailed(_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap)));
		}

		// Create a command allocator for each frame.
		for(UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocators[n])));
		}
	}

	// Load the sample assets.
	void Renderer::LoadAssets()
	{
		// Create an empty root signature.
		{
			using namespace Microsoft::WRL;

			CD3DX12_DESCRIPTOR_RANGE ranges[1];
			CD3DX12_ROOT_PARAMETER rootParameters[1];

			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			ThrowIfFailed(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
		}

		// Create the command list.
		ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocators[_frameIndex].Get(), nullptr, IID_PPV_ARGS(&_commandList)));

		CreateFramebuffers();

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Create synchronization objects and wait until assets have been uploaded to the GPU.
		{
			ThrowIfFailed(_device->CreateFence(_fenceValues[_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
			_fenceValues[_frameIndex]++;

			// Create an event handle to use for frame synchronization.
			_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if(_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			// Wait for the command list to execute; we are reusing the same command 
			// list in our main loop but for now, we just want to wait for setup to 
			// complete before continuing.
			WaitForGpu();
		}
	}

	// Wait for pending GPU work to complete.
	void Renderer::WaitForGpu()
	{
		// Schedule a Signal command in the queue.
		ThrowIfFailed(_commandQueue->Signal(_fence.Get(), _fenceValues[_frameIndex]));

		// Wait until the fence has been processed.
		ThrowIfFailed(_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent));
		WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);

		// Increment the fence value for the current frame.
		_fenceValues[_frameIndex]++;
	}

	// Prepare to render the next frame.
	void Renderer::MoveToNextFrame()
	{
		// Schedule a Signal command in the queue.
		const UINT64 currentFenceValue = _fenceValues[_frameIndex];
		ThrowIfFailed(_commandQueue->Signal(_fence.Get(), currentFenceValue));

		// Update the frame index.
		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		// If the next frame is not ready to be rendered yet, wait until it is ready.
		if(_fence->GetCompletedValue() < _fenceValues[_frameIndex])
		{
			ThrowIfFailed(_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent));
			WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		_fenceValues[_frameIndex] = currentFenceValue + 1;
	}

	// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
	// If no such adapter can be found, *ppAdapter will be set to nullptr.
	void Renderer::GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
	{
		IDXGIAdapter1* pAdapter = nullptr;
		*ppAdapter = nullptr;

		for(UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);

			if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if(SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*ppAdapter = pAdapter;
	}
}
