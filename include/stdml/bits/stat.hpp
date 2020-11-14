#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace stdml::collective::rchan
{
class summary
{
  public:
    summary() {}
    ~summary() {}
};

class stat
{
    using clock = std::chrono::high_resolution_clock;
    using instant = std::chrono::time_point<clock>;

    struct event {
        std::string name;
        instant t0;
        instant t1;
        size_t payload;
    };

    std::atomic<bool> enabled_;
    std::mutex mu_;
    summary summary_;
    std::vector<event> events_;

  public:
    stat();
    ~stat();

    void enable();
    void disable();

    void record(std::string name, instant t0, instant t1, size_t payload);

    void report();
};

class scope
{
    using clock = std::chrono::high_resolution_clock;
    using instant = std::chrono::time_point<clock>;

    std::string name_;
    const instant t0_;

    stat &s_;
    size_t payload_;

  public:
    scope(std::string name, stat &s, size_t payload);
    ~scope();
};

extern stat _global_stat;

void stat_report();
void stat_enable();
void stat_disable();
}  // namespace stdml::collective::rchan

#ifdef STDML_ENABLE_TRACE
    #define STDML_PROFILE_RATE(name, payload)                                  \
        stdml::collective::rchan::scope __scope(                               \
            name, stdml::collective::rchan::_global_stat, payload)
#else
    #define STDML_PROFILE_RATE(name, payload)
#endif
