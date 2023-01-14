#pragma once
#include <chrono>
namespace yang
{

	class GameTimer
	{
		typedef std::chrono::high_resolution_clock clock;
		typedef std::chrono::duration<double> duration;
	private:
		GameTimer();
		clock::time_point current_time_,prev_time_;
		duration delta_time_;
	public:
		static GameTimer& GetInstance();
		void Tick();
		void Reset();
		double GetDeltaTime() const;

	};


}
