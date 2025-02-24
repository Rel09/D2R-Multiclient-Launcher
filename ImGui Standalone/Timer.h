#pragma once

#include <chrono>

#define  TIMESTAMP std::chrono::steady_clock::time_point

namespace Timer {
	std::chrono::steady_clock::time_point InitTimer();
	bool Sleep(TIMESTAMP& timestamp, const int msDuration);
}