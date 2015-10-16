#pragma once

#include <vector>

namespace LB
{
	class Entity;
	class Application;
	class Scene
	{
	public:
		friend Application;
		Scene();

	private:
		std::vector<Entity *>_entities;
	};
}
