#pragma once
#include <Windows.h>
#include "common.h"

namespace yang
{


	class Window
	{
	private:
		HWND window_handler_;
	public:
		explicit Window(HWND window_handler);
		[[nodiscard]] HWND GetWindowHandler();
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

		WindowManager();
	public:
		static WindowManager& GetWindowManger();
		Window* CreateYWindow(const char* name,uint width,uint height);
		void ProcessMessage();
		[[nodiscard]] bool ShouldClose() const;
		void Terminate();
		void Show() const;

	};
}

