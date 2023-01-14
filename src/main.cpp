#include "core/window_manager_class.h"
#include "core/game_timer_class.h"
#include "core/d3d_manager_class.h"
#include <iostream>
#include <functional>


void down(WPARAM a, uint x, uint y)
{
	std::cout << x << " " << y << std::endl;
}

int main()
{
	auto& window_manager = yang::WindowManager::GetInstance();
	window_manager.set_mouse_right_down_func(down);
	const auto window = window_manager.CreateYWindow("test", 800, 800);
	auto d3d_manager = yang::D3dManager(window);
	auto game_timer = yang::GameTimer::GetInstance();

	window_manager.Show();
	game_timer.Reset();
	while (!yang::WindowManager::GetInstance().ShouldClose())
	{
		game_timer.Tick();
		window_manager.ProcessMessage();

		d3d_manager.Draw();
	}
	window_manager.Terminate();
	return 0;
}
