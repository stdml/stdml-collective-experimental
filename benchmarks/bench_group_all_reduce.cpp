#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/bits/collective/stat.hpp>
#include <stdml/collective>

// #include <tracer/simple_log>

#include "common.hpp"

// DEFINE_TRACE_CONTEXTS;

template <typename T>
std::future<T> make_ready_future(T x)
{
    std::promise<T> p;
    std::future<T> f = p.get_future();
    p.set_value(std::move(x));
    return f;
}

template <typename T, typename Duration>
std::future<T> make_delayed_future(T x, Duration d)
{
    std::promise<T> p;
    std::future<T> f = p.get_future();
    std::this_thread::sleep_for(d);
    p.set_value(std::move(x));
    return f;
}

double gigabytes(size_t n)
{
    constexpr double gi = 1 << 30;
    return n / gi;
}

std::string show_rate(double gibps)
{
    char line[64];
    if (gibps >= 1) {
        sprintf(line, "%.3f GiB/s", gibps);
    } else {
        sprintf(line, "%.3f GiB/s (%.3f MiB/s)", gibps, gibps * 1024);
    }
    return line;
}

template <typename T>
T mean(const std::vector<T> &xs)
{
    return std::accumulate(xs.begin(), xs.end(), static_cast<T>(0)) / xs.size();
}

using C = std::chrono::high_resolution_clock;

std::vector<std::future<stdml::collective::workspace>>
train_model(fake_cpu_model<float> &model)
{
    std::vector<std::future<stdml::collective::workspace>> fs;
    for (auto &b : model.buffers) {
        auto dt = stdml::collective::type<float>();
        stdml::collective::workspace w = {
            .send = b.send_buf.data(),
            .recv = b.recv_buf.data(),
            .count = b.send_buf.size(),
            .dt = dt,
            .op = stdml::collective::sum,
            .name = b.name,
        };
        using namespace std::chrono_literals;
        // fs.push_back(make_delayed_future(w, 1ms));
        fs.push_back(make_ready_future(w));
    }
    return fs;
}

double bench_step(stdml::collective::session &sess,
                  fake_cpu_model<float> &model)
{
    size_t multiplier = 4 * (sess.size() - 1);
    LOG_SCOPE_RATE(__func__, multiplier * model.data_size);
    auto t0 = C::now();
    auto fs = train_model(model);
    sess.group_all_reduce(std::move(fs));
    auto t1 = C::now();
    std::chrono::duration<double> d = (t1 - t0);
    return d.count();
}

std::vector<size_t> read_int_list(const char *filename)
{
    std::vector<size_t> sizes;
    std::ifstream fs(filename);
    if (!fs.is_open()) {
        throw std::runtime_error("file not found");
    }
    size_t size;
    while (fs >> size) {
        sizes.push_back(size);
    }
    return sizes;
}

using stdml::collective::log;
using stdml::collective::PRINT;

void bench(const std::string &name, const std::vector<size_t> &sizes, int steps,
           int warmup_steps)
{
    fake_cpu_model<float> model(sizes);
    log() << sizes.size() << "tensors";
    log() << "total size" << model.data_size <<  //
        "(" << gigabytes(model.data_size) << " GiB)";

    // stdml::collective::rchan::stat_disable();

    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();

    // stdml::collective::rchan::stat_enable();

    size_t multiplier = 4 * (sess.size() - 1);

    auto run_stage = [&](const char *name, int steps) {
        std::vector<double> metrics;
        for (auto i : std::views::iota(0, steps)) {
            log() << "bench step" << i;
            auto d = bench_step(sess, model);
            double metric = gigabytes(multiplier * model.data_size) / d;
            metrics.push_back(metric);
            if (sess.rank() == 0) {
                log() << name << i + 1 << show_rate(metric);
            }
        }
        return metrics;
    };
    run_stage("warmup", warmup_steps);
    auto metrics = run_stage("step", steps);
    if (sess.rank() == 0) {
        std::cout << "FINAL RESULT: " << name << " " << show_rate(mean(metrics))
                  << " | " << peer.config() << std::endl;
    }
}

struct options {
    std::string name;
    std::vector<size_t> sizes;
    int steps;
    int warmup_steps;
};

options parse_args(int argc, char *argv[])
{
    std::vector<size_t> sizes;
    std::string workload(argv[1]);
    int x, n;
    if (sscanf(workload.c_str(), "%dx%d", &x, &n) == 2) {
        sizes.resize(n);
        std::fill(sizes.begin(), sizes.end(), x);
    } else {
        sizes = read_int_list(workload.c_str());
    }

    const int steps = std::stoi(argv[2]);

    int warmup_step = 0;
    if (argc > 3) {
        warmup_step = std::stoi(argv[3]);
    }

    return {workload, sizes, steps, warmup_step};
}

int main(int argc, char *argv[])
{
    // TRACE_SCOPE(__func__);
    auto options = parse_args(argc, argv);
    bench(options.name, options.sizes, options.steps, options.warmup_steps);
    log(PRINT) << argv[0] << "finished";
    return 0;
}
