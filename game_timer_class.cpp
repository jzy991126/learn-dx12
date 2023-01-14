#include "game_timer_class.h"

namespace yang
{
	GameTimer::GameTimer()
	{
	}

	GameTimer& GameTimer::GetInstance()
	{
		static GameTimer game_timer;
		return game_timer;
	}

	void GameTimer::Tick()
	{
		const auto current_time = clock::now();
		delta_time_ = current_time - prev_time_;
		prev_time_ = current_time;
		current_time_ = current_time;
	}

	void GameTimer::Reset()
	{
		prev_time_ = clock::now();

	}

	double GameTimer::delta_time() const
	{
		return delta_time_.count();
	}
}
