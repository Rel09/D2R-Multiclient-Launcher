#include "Timer.h"


namespace Timer {

    std::chrono::steady_clock::time_point InitTimer() {
        return std::chrono::steady_clock::now();
    }

    bool Sleep(TIMESTAMP& timestamp, const int msDuration) {
        if (timestamp.time_since_epoch().count() == 0) {
            timestamp = std::chrono::steady_clock::now();
            return false;
        }

        auto currentTimestamp = std::chrono::steady_clock::now();
        auto elapsedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimestamp - timestamp).count();

        if (elapsedDuration >= msDuration) {
            timestamp = std::chrono::steady_clock::time_point();
            return true;
        }

        return false;
    }

}