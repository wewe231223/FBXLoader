#pragma once

#include <chrono>

namespace asset
{
    class Timer
    {
    public:
        Timer();

        void Tick();

        float DeltaSeconds() const;
        double TotalSeconds() const;

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_start{};
        Clock::time_point m_prev{};
        float m_deltaSeconds{0.0f};
    };
} // namespace gfx
