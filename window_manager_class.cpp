#include "window_manager_class.h"
#include<iostream>
#include <utility>
#include<windowsx.h>


namespace yang
{

	void empty_mouse_function(WPARAM button_state, uint x, uint y)
	{
		std::cout << "down" << std::endl;
	}

	LRESULT WINAPI default_window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto& window_manager = yang::WindowManager::GetWindowManger();
		switch (msg)
		{
		case WM_RBUTTONDOWN:
			window_manager.GetMouseRightDownFunction()(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default: 
			return ::DefWindowProc(hWnd, msg, wParam, lParam);
		}

		
	}

	Window::Window(const HWND window_handler)
	{
		window_handler_ = window_handler;
		
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	HWND Window::GetWindowHandler()
	{
		return window_handler_;
	}

	WindowManager::WindowManager()
	{
		window_class_ = { sizeof(WNDCLASSEX), CS_CLASSDC, default_window_proc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Example", nullptr };
		window_handle_ = nullptr;
		window_ = nullptr;
		mouse_right_down_function_ = empty_mouse_function;

	}

	WindowManager::WindowManager(const WindowManager&) = default;

	WindowManager& WindowManager::operator=(const WindowManager&) = default;
	

	WindowManager& WindowManager::GetWindowManger()
	{
		static WindowManager window_manager;
		return window_manager;

	}

	Window* WindowManager::CreateYWindow(const char* name, uint width, uint height)
	{
		const auto w_name = ConvertCharArrayToLPCWSTR(name);
		// ReSharper disable once CppLocalVariableMayBeConst
		WNDCLASSEX window_class = { sizeof(WNDCLASSEX), CS_CLASSDC, default_window_proc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, w_name, nullptr};
		RegisterClassEx(&window_class);
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND window_handler = CreateWindow(window_class.lpszClassName, w_name, WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, window_class.hInstance, NULL);
		window_ = new Window(window_handler);
		return window_;
	}

	void WindowManager::ProcessMessage()
	{
		if(message_.message== WM_QUIT)
		{
			b_should_close_ = true;
			return;
		}

		ZeroMemory(&message_, sizeof(message_));
		if (PeekMessage(&message_, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&message_);
			DispatchMessage(&message_);
		}
	}

	bool WindowManager::ShouldClose() const
	{
		return b_should_close_;
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	void WindowManager::Terminate()
	{
		DestroyWindow(window_->GetWindowHandler());
		UnregisterClass(window_class_.lpszClassName, window_class_.hInstance);
	}

	void WindowManager::Show() const
	{
		ShowWindow(window_->GetWindowHandler(), SW_SHOWDEFAULT);
		UpdateWindow(window_->GetWindowHandler());

	}

	mouse_function WindowManager::GetMouseRightDownFunction()
	{
		return mouse_right_down_function_;
	}

	void WindowManager::SetMouseRightDownFunction(mouse_function function)
	{
		mouse_right_down_function_ = std::move(function);
	}
}
