#pragma once

#include "LBSceneNode.h"

namespace LB
{
	class Model;
	class Renderer;
	class Entity : public SceneNode
	{
	public:
		friend Renderer;
		Entity(Model *model);

	private:
		Model *_model;
	};
}
