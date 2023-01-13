#include "window_manager_class.h"
int main()
{
	auto window_manager = yang::WindowManager::GetWindowManger();
	auto window = window_manager.CreateYWindow("test", 800, 800);
	window_manager.Show();
	while (!window_manager.ShouldClose())
	{
		window_manager.ProcessMessage();
	}
	window_manager.Terminate();
	return 0;
}