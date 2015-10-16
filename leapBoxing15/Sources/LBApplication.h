#pragma once

namespace LB
{
	class Renderer;
	class Scene;

	class Application
	{
	public:
		static Application &GetInstance()
		{
			static Application instance;
			return instance;
		}

		int Run(HINSTANCE hInstance, int nCmdShow);

		Renderer *GetRenderer();
		std::wstring GetPathForAsset(LPCWSTR filename);

		void SetScene(Scene *scene);

	private:
		Application();
		~Application();

		Application(Application const&) = delete;
		void operator=(Application const&) = delete;

		bool OnEvent(MSG);
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		UINT _width;
		UINT _height;
		float _aspectRatio;
		HWND _hwnd;
		std::wstring _title;
		std::wstring _assetsPath;

		Renderer *_renderer;
		Scene *_scene;
	};
}