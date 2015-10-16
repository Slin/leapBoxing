#pragma once

#include "vector"

namespace LB
{
	class Entity;
	class Renderer
	{
	public:
		Renderer(HWND hwnd, bool useWARPDevice);
		~Renderer();

		void Render(const std::vector<Entity*> &entities);

		void SetWindowSize(int width, int height, bool minimized);
		void ToggleFullscreen();
		Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSOForDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc);
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadVertexData(void *data, long dataSize);
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadIndexData(void *data, long dataSize);

	private:
		void CreatePipeline(HWND hwnd, bool useWARPDevice);
		void LoadAssets();
		void CreateFramebuffers();
		void GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
		void WaitForGpu();
		void MoveToNextFrame();

		static const UINT FrameCount = 2;

		int _windowWidth;
		int _windowHeight;

		bool _needsUpdateForWindowSizeChange;
		bool _windowVisible;

		D3D12_VIEWPORT _viewport;
		D3D12_RECT _scissorRect;

		// Pipeline objects.
		Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
		Microsoft::WRL::ComPtr<ID3D12Device> _device;
		Microsoft::WRL::ComPtr<ID3D12Resource> _renderTargets[FrameCount];
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocators[FrameCount];
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _cbvHeap;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
		UINT _rtvDescriptorSize;

		// Synchronization objects.
		UINT _frameIndex;
		HANDLE _fenceEvent;
		Microsoft::WRL::ComPtr<ID3D12Fence> _fence;
		UINT64 _fenceValues[FrameCount];
	};
}