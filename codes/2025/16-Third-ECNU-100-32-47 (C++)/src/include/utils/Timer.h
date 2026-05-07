#pragma once

#include <chrono>
namespace aaac {
namespace utils {
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    // 重置计时器
    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    // 返回经过的时间（默认毫秒）
    template<typename Duration = std::chrono::milliseconds>
    int64_t elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<Duration>(end - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};
}
}