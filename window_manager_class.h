#pragma once
#include <Windows.h>
#include <functional>
#include "common.h"

namespace yang
{
	using mouse_function = std::function<void(WPARAM button_state, uint x, uint y)>;

	class Window
	{
	private:
		HWND window_handler_;
	public:
		explicit Window(HWND window_handler);
		[[nodiscard]] HWND window_handler();
	};


	class WindowManager
	{

	private:
		HWND window_handle_;
		WNDCLASSEX window_class_{};

		MSG message_{};
		bool b_should_close_{ false };

	private:
		Window* window_;

		mouse_function mouse_right_down_func_;

		WindowManager();
		WindowManager(const WindowManager&);
		WindowManager& operator=(const WindowManager&);
	public:
		static WindowManager& GetInstance();
		Window* CreateYWindow(const char* name,uint width,uint height);
		void ProcessMessage();
		[[nodiscard]] bool ShouldClose() const;
		void Terminate();
		void Show() const;
		mouse_function mouse_right_down_func();
		void set_mouse_right_down_func(mouse_function function);

	};
}

