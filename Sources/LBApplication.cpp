#include "stdafx.h"
#include "LBApplication.h"
#include "LBRenderer.h"
#include "LBScene.h"

namespace LB
{
	Application::Application() : _width(1280), _height(720), _title(L"leapBoxing 15"), _renderer(nullptr)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));
		_assetsPath = assetsPath;
	}

	Application::~Application()
	{

	}

	bool Application::OnEvent(MSG)
	{
		return false;
	}

	int Application::Run(HINSTANCE hInstance, int nCmdShow)
	{
		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"LBMainWindowClass";
		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// Create the window and store a handle to it.
		_hwnd = CreateWindowEx(NULL,
			L"LBMainWindowClass",
			_title.c_str(),
			WS_OVERLAPPEDWINDOW,
			300,
			300,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			NULL,		// We have no parent window, NULL.
			NULL,		// We aren't using menus, NULL.
			hInstance,
			this);		// We aren't using multiple windows, NULL.

		ShowWindow(_hwnd, nCmdShow);

		_renderer = new Renderer(_hwnd, false);

		SetScene(new LB::Scene());

		// Main sample loop.
		MSG msg = { 0 };
		while(true)
		{
			// Process any messages in the queue.
			if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if(msg.message == WM_QUIT)
					break;

				// Pass events into our sample.
				OnEvent(msg);
			}
		}

		delete _renderer;
		_renderer = nullptr;

		// Return this part of the WM_QUIT message to Windows.
		return static_cast<char>(msg.wParam);
	}

	Renderer *Application::GetRenderer()
	{
		return _renderer;
	}

	std::wstring Application::GetPathForAsset(LPCWSTR filename)
	{
		return _assetsPath + filename;
	}

	void Application::SetScene(Scene *scene)
	{
		_scene = scene;
	}

	// Main message handler for the sample.
	LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Application* application = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch(message)
		{
		case WM_CREATE:
		{
			// Save a pointer to the DXSample passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

		case WM_PAINT:
			if(application && application->GetRenderer() && application->_scene)
			{
				application->GetRenderer()->Render(application->_scene->_entities);
			}
			return 0;

		case WM_SIZE:
			if(application && application->GetRenderer())
			{
				RECT clientRect = {};
				GetClientRect(hWnd, &clientRect);
				application->GetRenderer()->SetWindowSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, wParam == SIZE_MINIMIZED);
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		// Handle any messages the switch statement didn't.
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}