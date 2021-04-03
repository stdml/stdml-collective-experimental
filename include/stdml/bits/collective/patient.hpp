#pragma once
#include <chrono>
#include <cstdint>
#include <string>

#include <stdml/bits/collective/stat.hpp>  // for rchan::show_duration

namespace stdml::collective
{
class patient
{
    using Clock = std::chrono::high_resolution_clock;
    using Instant = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<int64_t, std::nano>;

    const Duration threshold_;
    Instant last_;

  public:
    patient(int n) : patient(std::chrono::seconds(n))
    {
    }

    patient(Duration threshold = std::chrono::seconds(1))
        : threshold_(threshold), last_(Clock::now())
    {
    }

    std::string show() const
    {
        return rchan::show_duration(threshold_);
    }

    void reset()
    {
        last_ = Clock::now();
    }

    bool ok() const
    {
        return (Clock::now() - last_) < threshold_;
    }
};
}  // namespace stdml::collective
