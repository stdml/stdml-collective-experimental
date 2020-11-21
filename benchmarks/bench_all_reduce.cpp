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

double gigabytes(size_t n)
{
    constexpr double gi = 1 << 30;
    return n / gi;
}

template <typename T>
void pprint(const std::vector<T> &xs)
{
    int i = 0;
    for (const T &x : xs) {
        if (i++) { std::cout << ", "; }
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

void bench_all_reduce_one(stdml::collective::session &session,
                          const std::string &name, std::vector<float> &x,
                          std::vector<float> &y)
{
    // TRACE_SCOPE(__func__);
    auto t0 = C::now();
    session.all_reduce(x.data(), x.data() + x.size(), y.data(), name);
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
    if (!fs.is_open()) { throw std::runtime_error("file not found"); }
    size_t size;
    while (fs >> size) { sizes.push_back(size); }
    return sizes;
}

using stdml::collective::log;
using stdml::collective::PRINT;

void bench(const std::string &name, const std::vector<size_t> &sizes, int times)
{
    pprint(sizes);
    const auto tot = std::accumulate(sizes.begin(), sizes.end(), 0);
    log(PRINT) << sizes.size() << "tensors";
    log(PRINT) << "total size" << tot * 4 <<  //
        "(" << gigabytes(tot * 4) << " GiB)";

    // stdml::collective::rchan::stat_disable();

    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session session = peer.join();

    // stdml::collective::rchan::stat_enable();

    size_t multiplier = 4 * (session.size() - 1);

    std::vector<fake_cpu_buffer_t<float>> buffers;
    for (auto i : std::views::iota((size_t)0, sizes.size())) {
        const std::string name = "variable:" + std::to_string(i);
        // log(PRINT) << name << sizes[i];
        buffers.emplace_back(name, sizes[i]);
    }

    std::vector<double> metrics;
    for (auto i : std::views::iota(0, times)) {
        log(PRINT) << "bench step" << i;
        auto d = bench_step(session, buffers);
        double metric = gigabytes(multiplier * tot * sizeof(float)) / d;
        metrics.push_back(metric);
        printf("%.3f GiB/s\n", metric);
    }
    printf("FINAL RESULT: %s %.3f GiB/s\n", name.c_str(), mean(metrics));
}

struct options {
    std::string name;
    std::vector<size_t> sizes;
    int times;
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

    const int times = std::stoi(argv[2]);

    if (argc > 3) {
        const size_t tot = std::accumulate(sizes.begin(), sizes.end(), 0);
        sizes = {tot};
    }

    return {workload, sizes, times};
}

void signal_handler(int sig)
{
    log() << "signal" << sig << ":" << strsignal(sig);  //
}

int main(int argc, char *argv[])
{
    // TRACE_SCOPE(__func__);
    auto options = parse_args(argc, argv);

    std::signal(SIGINT, signal_handler);

    bench(options.name, options.sizes, options.times);
    return 0;
}
