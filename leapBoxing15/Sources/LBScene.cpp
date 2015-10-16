#include "stdafx.h"
#include "LBScene.h"
#include "LBEntity.h"
#include "LBModel.h"
#include "LBMaterial.h"
#include "LBMesh.h"
#include "LBApplication.h"

namespace LB
{
	Scene::Scene()
	{
		Material *material = new Material(L"shaders.hlsl");
		Mesh *mesh = Mesh::WithCube();
		Model *model = new Model(mesh, material);
		Entity *entity = new Entity(model);

		_entities.push_back(entity);
	}
}
