#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/bits/collective/stat.hpp>
#include <stdml/collective>

#include <std/filesystem>
#include <std/ranges>

// #include <tracer/simple_log>

#include "common.hpp"

// DEFINE_TRACE_CONTEXTS;

double gigabytes(size_t n)
{
    constexpr double gi = 1 << 30;
    return n / gi;
}

std::string show_rate(double gibps)
{
    char line[64];
    if (gibps >= 1) {
        std::snprintf(line, sizeof(line), "%.3f GiB/s", gibps);
    } else {
        std::snprintf(line, sizeof(line), "%.3f GiB/s (%.3f MiB/s)", gibps,
                      gibps * 1024);
    }
    return line;
}

std::string pad(std::string s, size_t len)
{
    if (s.size() < len) {
        s.resize(len, ' ');
    }
    return s;
}

template <typename T>
void pprint(const std::vector<T> &xs)
{
    int i = 0;
    for (const T &x : xs) {
        if (i++) {
            std::cout << ", ";
        }
        std::cout << x;
    }
    std::cout << std::endl;
}

template <typename T>
T mean(const std::vector<T> &xs)
{
    return std::accumulate(xs.begin(), xs.end(), static_cast<T>(0)) / xs.size();
}

using C = std::chrono::high_resolution_clock;

void bench_all_reduce_one(stdml::collective::session &sess,
                          const std::string &name, std::vector<float> &x,
                          std::vector<float> &y)
{
    // size_t multiplier = 4 * (sess.size() - 1);
    // LOG_SCOPE_RATE(__func__, multiplier * x.size() * sizeof(float));

    auto t0 = C::now();
    sess.all_reduce(x.data(), x.data() + x.size(), y.data(), name);
    auto t1 = C::now();
    double d [[gnu::unused]] = (t1 - t0).count();
}

double bench_step(stdml::collective::session &session,
                  std::vector<fake_cpu_buffer_t<float>> &buffers)
{
    // TRACE_SCOPE(__func__);
    auto t0 = C::now();
    for (auto &b : buffers) {
        // auto [d, g] =
        bench_all_reduce_one(session, b.name, b.send_buf, b.recv_buf);
    }
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
    // pprint(sizes);
    const auto tot = std::accumulate(sizes.begin(), sizes.end(), 0);
    log(PRINT) << sizes.size() << "tensors";
    log(PRINT) << "total size" << tot * 4 <<  //
        "(" << gigabytes(tot * 4) << " GiB)";

    // stdml::collective::rchan::stat_disable();

    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();

    // stdml::collective::rchan::stat_enable();

    size_t multiplier = 4 * (sess.size() - 1);

    std::vector<fake_cpu_buffer_t<float>> buffers;
    for (auto i : std::views::iota((size_t)0, sizes.size())) {
        const std::string name = "variable:" + std::to_string(i);
        // log(PRINT) << name << sizes[i];
        buffers.emplace_back(name, sizes[i]);
    }

    auto run_stage = [&](const char *name, int steps) {
        std::vector<double> metrics;
        for (auto i : std::views::iota(0, steps)) {
            log(PRINT) << "bench step" << i;
            auto d = bench_step(sess, buffers);
            double metric = gigabytes(multiplier * tot * sizeof(float)) / d;
            metrics.push_back(metric);
            if (sess.rank() == 0) {
                std::cout << name << " " << i + 1 << " " << show_rate(metric)
                          << std::endl;
            }
        }
        return metrics;
    };
    {
        LOG_SCOPE_LATENCY("warmup");
        run_stage("warmup", warmup_steps);
    }
    auto metrics = LOG_EXPR_LATENCY(run_stage("step", steps));
    if (sess.rank() == 0) {
        std::cout << "FINAL RESULT: " << pad(name, 20) << " "
                  << show_rate(mean(metrics)) << " | " << peer.config()
                  << std::endl;
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
    return 0;
}
