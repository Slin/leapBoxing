#pragma once

namespace LB
{
	class Renderer;
	class Mesh
	{
	public:
		friend Renderer;
		static Mesh *WithTriangle();
		static Mesh *WithQuad();
		static Mesh *WithCube();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW _indexBufferView;
		D3D_PRIMITIVE_TOPOLOGY _topology;
		int _vertexCount;
		int _indexCount;
	};
}
