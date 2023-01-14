#include "window_manager_class.h"
#include "game_timer_class.h"
#include <iostream>
#include <functional>


void down(WPARAM a,uint x,uint y)
{
	std::cout << x<<" "<<y<<std::endl;
}

int main()
{
	auto& window_manager = yang::WindowManager::GetWindowManger();
	window_manager.SetMouseRightDownFunction(down);
	auto window = window_manager.CreateYWindow("test", 800, 800);
	auto game_timer = yang::GameTimer::GetInstance();
	
	window_manager.Show();
	game_timer.Reset();
	while (!yang::WindowManager::GetWindowManger().ShouldClose())
	{
		game_timer.Tick();
		window_manager.ProcessMessage();
	}
	window_manager.Terminate();
	return 0;
}