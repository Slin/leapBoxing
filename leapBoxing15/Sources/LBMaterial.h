#pragma once

namespace LB
{
	class Renderer;
	class Material
	{
	public:
		friend Renderer;
		Material(LPCWSTR shaderfile);
		~Material();

	private:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;
	};
}
