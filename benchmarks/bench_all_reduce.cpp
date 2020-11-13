#include <chrono>
#include <cstdio>
#include <cstdlib>
// #include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/bits/stat.hpp>
#include <stdml/collective>

#include <tracer/simple_log>

#include "common.hpp"

DEFINE_TRACE_CONTEXTS;

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

using C = std::chrono::high_resolution_clock;

void bench_all_reduce_one(stdml::collective::session &session,
                          const std::string &name, std::vector<float> &x,
                          std::vector<float> &y)
{
    // TRACE_SCOPE(__func__);
    auto t0 = C::now();
    session.all_reduce(x.data(), x.data() + x.size(), y.data(), name);
    auto t1 = C::now();
    double d = (t1 - t0).count();
}

double bench_step(stdml::collective::session &session,
                  std::vector<fake_cpu_buffer_t<float>> &buffers)
{
    TRACE_SCOPE(__func__);
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
    size_t size;
    while (fs >> size) { sizes.push_back(size); }
    return sizes;
}

using stdml::collective::log;
using stdml::collective::PRINT;

void bench(const std::vector<size_t> &sizes, int times)
{
    pprint(sizes);
    const auto tot = std::accumulate(sizes.begin(), sizes.end(), 0);
    log(PRINT) << sizes.size() << "tensors";
    log(PRINT) << "total size" << tot;

    stdml::collective::rchan::stat_disable();

    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session session = peer.join();

    stdml::collective::rchan::stat_enable();

    size_t multiplier = 4 * (session.size() - 1);

    std::vector<fake_cpu_buffer_t<float>> buffers;
    for (size_t i = 0; i < sizes.size(); ++i) {
        const std::string name = "variable:" + std::to_string(i);
        // log(PRINT) << name << sizes[i];
        buffers.emplace_back(name, sizes[i]);
    }

    for (int i = 0; i < times; ++i) {
        log(PRINT) << "bench step" << i;
        auto d = bench_step(session, buffers);
        printf("%.3f GiB/s\n", gigabytes(multiplier * tot * 4) / d);
    }

    stdml::collective::rchan::stat_report();
}

int main(int argc, char *argv[])
{
    TRACE_SCOPE(__func__);

    const auto sizes = read_int_list(argv[1]);
    const int times = std::stoi(argv[2]);
    bool fuse = false;
    if (argc > 3) { fuse = true; }
    if (fuse) {
        const size_t tot = std::accumulate(sizes.begin(), sizes.end(), 0);
        bench({tot}, times);
    } else {
        bench(sizes, times);
    }
    return 0;
}
