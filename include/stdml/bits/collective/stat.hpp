#pragma once
#include <atomic>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace stdml::collective::rchan
{
class summary
{
  public:
    summary()
    {
    }
    ~summary()
    {
    }
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

class single_site_context
{
    size_t count_;

  public:
    single_site_context() : count_(0)
    {
    }

    size_t begin()
    {
        return ++count_;
    }

    void end()
    {
        //
    }
};

template <typename Duration>
static std::string show_duration(const Duration &d)
{
    std::stringstream ss;
    ss << std::setprecision(4) << std::setw(6);
    if (d < std::chrono::microseconds(1)) {
        using D = std::chrono::duration<int64_t, std::nano>;
        auto d1 = std::chrono::duration_cast<D>(d);
        ss << d1.count() << "ns";
    } else if (d < std::chrono::milliseconds(1)) {
        using D = std::chrono::duration<float, std::micro>;
        auto d1 = std::chrono::duration_cast<D>(d);
        ss << d1.count() << "us";
    } else if (d < std::chrono::seconds(1)) {
        using D = std::chrono::duration<float, std::milli>;
        auto d1 = std::chrono::duration_cast<D>(d);
        ss << d1.count() << "ms";
    } else {
        using D = std::chrono::duration<float>;
        auto d1 = std::chrono::duration_cast<D>(d);
        ss << d1.count() << "s";
    }
    return ss.str();
}

class scope_latency_logger
{
    using clock = std::chrono::high_resolution_clock;
    using instant = std::chrono::time_point<clock>;

    std::string name_;
    size_t seq_;
    const instant t0_;

  public:
    scope_latency_logger(single_site_context &ctx, std::string name)
        : name_(std::move(name)), seq_(ctx.begin()), t0_(clock::now())
    {
    }

    ~scope_latency_logger()
    {
        auto t1 = clock::now();
        std::chrono::duration<double> d = t1 - t0_;
        std::cout << "#" << std::setw(8) << seq_                        //
                  << "  |  took: " << std::setw(8) << show_duration(d)  //
                  << "    @" << name_ << std::endl;
    }
};

class scope_rate_logger
{
    using clock = std::chrono::high_resolution_clock;
    using instant = std::chrono::time_point<clock>;

    std::string name_;
    size_t seq_;
    const instant t0_;
    const size_t payload_;

    static std::string show_rate(size_t size, std::chrono::duration<double> d)
    {
        char line[32];
        double rate = size / d.count();
        if (rate > (1 << 30)) {
            sprintf(line, "%.3f GiB/s", rate / (1 << 30));
        } else {
            sprintf(line, "%.3f MiB/s", rate / (1 << 20));
        }
        return line;
    }

  public:
    scope_rate_logger(single_site_context &ctx, std::string name,
                      size_t payload)
        : name_(std::move(name)),
          seq_(ctx.begin()),
          t0_(clock::now()),
          payload_(payload)
    {
    }

    ~scope_rate_logger()
    {
        auto t1 = clock::now();
        std::chrono::duration<double> d = t1 - t0_;
        std::cout << "#" << std::setw(8) << seq_  //
                  << std::setw(16) << show_rate(payload_, d)
                  << "  |  took: " << std::setw(8) << show_duration(d)  //
                  << "    @" << name_ << std::endl;
    }
};

extern stat _global_stat;

void stat_report();
void stat_enable();
void stat_disable();
}  // namespace stdml::collective::rchan

#ifdef STDML_COLLECTIVE_ENABLE_TRACE
    #define STDML_COLLECTIVE_PROFILE_RATE(name, payload)                       \
        stdml::collective::rchan::scope __scope(                               \
            name, stdml::collective::rchan::_global_stat, payload)
#else
    #define STDML_COLLECTIVE_PROFILE_RATE(name, payload)
#endif

#define LOG_SCOPE_RATE(name, payload)                                          \
    static stdml::collective::rchan::single_site_context __scope_rate_ctx;     \
    stdml::collective::rchan::scope_rate_logger __scope_rate_logger(           \
        __scope_rate_ctx, name, payload)

#define LOG_SCOPE_LATENCY(name)                                                \
    static stdml::collective::rchan::single_site_context __scope_rate_ctx;     \
    stdml::collective::rchan::scope_latency_logger __scope_latency_logger(     \
        __scope_rate_ctx, name)

#define LOG_EXPR_LATENCY(e)                                                    \
    [&] {                                                                      \
        LOG_SCOPE_LATENCY(#e);                                                 \
        return (e);                                                            \
    }()
