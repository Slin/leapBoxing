#include "stdafx.h"
#include "LBMesh.h"
#include "LBApplication.h"
#include "LBRenderer.h"

namespace LB
{
	Mesh *Mesh::WithTriangle()
	{
		Mesh *mesh = new Mesh();

		float data[] = { 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f};
		const UINT dataSize = sizeof(data);
		const UINT stride = 4 * 7;

		mesh->_vertexBuffer = Application::GetInstance().GetRenderer()->UploadVertexData(data, dataSize);

		// Initialize the vertex buffer views.
		mesh->_vertexBufferView.BufferLocation = mesh->_vertexBuffer->GetGPUVirtualAddress();
		mesh->_vertexBufferView.StrideInBytes = stride;
		mesh->_vertexBufferView.SizeInBytes = dataSize;

		mesh->_vertexCount = dataSize / stride;
		mesh->_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

		return mesh;
	}

	Mesh *Mesh::WithQuad()
	{
		Mesh *mesh = new Mesh();

		float data[] = { -1.0f, -1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f,   1.0f, -1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };
		const UINT dataSize = sizeof(data);
		const UINT stride = 4 * 7;

		mesh->_vertexBuffer = Application::GetInstance().GetRenderer()->UploadVertexData(data, dataSize);

		// Initialize the vertex buffer views.
		mesh->_vertexBufferView.BufferLocation = mesh->_vertexBuffer->GetGPUVirtualAddress();
		mesh->_vertexBufferView.StrideInBytes = stride;
		mesh->_vertexBufferView.SizeInBytes = dataSize;

		mesh->_vertexCount = dataSize / stride;
		mesh->_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

		return mesh;
	}

	Mesh *Mesh::WithCube()
	{
		Mesh *mesh = new Mesh();

		float data[] = { 1.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,
						 1.0f, -1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 
						 1.0f, 1.0f, -1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
						 1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
						 -1.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,
						 -1.0f, -1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,
						 -1.0f, 1.0f, -1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
						 -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f, 1.0f };
		const UINT dataSize = sizeof(data);
		const UINT stride = 4 * 7;

		mesh->_vertexBuffer = Application::GetInstance().GetRenderer()->UploadVertexData(data, dataSize);

		// Initialize the vertex buffer views.
		mesh->_vertexBufferView.BufferLocation = mesh->_vertexBuffer->GetGPUVirtualAddress();
		mesh->_vertexBufferView.StrideInBytes = stride;
		mesh->_vertexBufferView.SizeInBytes = dataSize;

		mesh->_vertexCount = dataSize / stride;
		mesh->_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		UINT16 indices[] = { 0, 1, 2, 1, 2, 3,   4, 5, 6, 5, 6, 7,   0, 2, 4, 2, 4, 6,   1, 3, 5, 3, 5, 7,   0, 4, 1, 1, 4, 5,   2, 3, 6, 6, 3, 7 };
		const UINT indicesSize = sizeof(indices);

		mesh->_indexBuffer = Application::GetInstance().GetRenderer()->UploadIndexData(indices, indicesSize);

		mesh->_indexBufferView.BufferLocation = mesh->_indexBuffer->GetGPUVirtualAddress();
		mesh->_indexBufferView.SizeInBytes = indicesSize;
		mesh->_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

		mesh->_indexCount = indicesSize / 2;

		return mesh;
	}
}
