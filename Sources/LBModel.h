#pragma once

namespace LB
{
	class Mesh;
	class Material;
	class Renderer;
	class Model
	{
	public:
		friend Renderer;
		Model::Model(Mesh *mesh, Material *material);

	private:
		Mesh *_mesh;
		Material *_material;
	};
}