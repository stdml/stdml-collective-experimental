#pragma once
#include <iostream>
#include <mutex>

namespace stdml::collective
{
class logger
{
    static std::mutex mu_;
    static bool enabled_;

    std::lock_guard<std::mutex> lk_;
    std::ostream &os;

  public:
    logger(std::ostream &os);

    ~logger();

    template <typename T>
    logger &operator<<(const T &x)
    {
        if (enabled_) { os << " " << x; }
        return *this;
    }
};

class noop_logger
{
    template <typename T>
    noop_logger &operator<<(const T &x)
    {
        return *this;
    }
};

enum log_level {
    INFO,
    DEBUG,
    PRINT,
};

logger log(log_level level = INFO);
}  // namespace stdml::collective
