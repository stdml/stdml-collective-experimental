#pragma once
#include <cstdio>
#include <mutex>
#include <ostream>

#define TODO(e) fprintf(stderr, "TODO: %s\n", (e));

namespace stdml::collective
{
class logger
{
    static std::mutex mu_;

    std::lock_guard<std::mutex> lk_;
    std::ostream &os;

  public:
    static bool enabled_;

    logger(std::ostream &os);

    ~logger();

    template <typename T>
    logger &operator<<(const T &x)
    {
        if (enabled_) {
            os << " " << x;
        }
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
    DEBUG,
    INFO,
    PRINT,
    WARN,
    ERROR,
};

logger log(log_level level = INFO);
extern void enable_log();
}  // namespace stdml::collective
