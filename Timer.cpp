#include "Timer.h"

namespace asset
{
    Timer::Timer()
        : m_start(Clock::now())
        , m_prev(m_start)
    {
    }

    void Timer::Tick()
    {
        const auto now = Clock::now();
        const std::chrono::duration<float> delta = now - m_prev;
        m_prev = now;
        m_deltaSeconds = delta.count();
        if (m_deltaSeconds < 0.0f)
        {
            m_deltaSeconds = 0.0f;
        }
    }

    float Timer::DeltaSeconds() const
    {
        return m_deltaSeconds;
    }

    double Timer::TotalSeconds() const
    {
        const auto now = Clock::now();
        const std::chrono::duration<double> total = now - m_start;
        return total.count();
    }
} // namespace gfx
