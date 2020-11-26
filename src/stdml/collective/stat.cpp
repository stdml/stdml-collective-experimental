#include <iomanip>
#include <iostream>

#include <stdml/bits/collective/stat.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const std::string &s);
}

namespace stdml::collective::rchan
{
stat _global_stat;

stat::stat() : enabled_(parse_env_bool("STDML_COLLECTIVE_ENABLE_TRACE"))
{
    events_.reserve(1 << 20);
}

stat::~stat()
{
    report();
}

void stat::enable()
{
    enabled_ = true;
}

void stat::disable()
{
    enabled_ = false;
}

void stat::report()
{
    for (const auto &e : events_) {
        const auto t0 = e.t0.time_since_epoch();
        const auto t1 = e.t1.time_since_epoch();
        const std::chrono::duration<double> d = t1 - t0;
        const double rate = e.payload / 1e9 / d.count();
        std::cout << "``" << std::setw(20) << e.name  //
                  << std::setw(24) << t0.count()      //
                  << std::setw(24) << t1.count()      //
                  << std::setw(20) << std::fixed << std::setprecision(3)
                  << 1e3 * d.count()             //
                  << std::setw(20) << e.payload  //
                  << std::setw(20) << rate << std::endl;
    }
}

void stat::record(std::string name, instant t0, instant t1, size_t payload)
{
    if (enabled_) {
        std::lock_guard<std::mutex> _(mu_);
        events_.emplace_back(std::move(name), t0, t1, payload);
    }
}

scope::scope(std::string name, stat &s, size_t payload)
    : name_(std::move(name)), t0_(clock::now()), s_(s), payload_(payload)
{
}

scope::~scope()
{
    const instant t1 = clock::now();
    s_.record(std::move(name_), t0_, t1, payload_);
}

void stat_report()
{
    _global_stat.report();
}

void stat_enable()
{
    _global_stat.enable();
}

void stat_disable()
{
    _global_stat.disable();
}
}  // namespace stdml::collective::rchan
