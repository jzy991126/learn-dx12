#include "core/game_timer_class.h"

namespace yang {
    GameTimer::GameTimer() {
    }

    GameTimer &GameTimer::GetInstance() {
        static GameTimer game_timer;
        return game_timer;
    }

    void GameTimer::Tick() {
        if (is_stop_) {
            delta_time_ = duration(0);
            return;
        }
        const auto current_time = clock::now();
        delta_time_ = current_time - prev_time_;
        prev_time_ = current_time;
        current_time_ = current_time;
        total_time_ += delta_time_;
    }

    void GameTimer::Reset() {
        prev_time_ = clock::now();
        base_time_ = prev_time_;
        is_stop_ = false;
        total_time_ = duration(0);

    }

    double GameTimer::delta_time() const {
        return delta_time_.count();
    }

    void GameTimer::Stop() {
        if (!is_stop_) {
            stop_time_ = clock::now();
            is_stop_ = true;
        }
    }

    void GameTimer::Start() {
        if (is_stop_) {
            prev_time_ = clock::now();
            is_stop_ = false;
        }
    }

    double GameTimer::total_time() const {
       return  total_time_.count();
    }
}
