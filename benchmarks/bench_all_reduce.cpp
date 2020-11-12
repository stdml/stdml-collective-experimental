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

#include <stdml/collective>
#include <tracer/simple_log>

#include "common.hpp"

DEFINE_TRACE_CONTEXTS;

double gigabytes(size_t n)
{
    constexpr double gi = 1 << 30;
    return n / gi;
}

using C = std::chrono::high_resolution_clock;

std::pair<double, double> bench_all_reduce(stdml::collective::session &session,
                                           std::vector<float> &x,
                                           std::vector<float> &y)
{
    auto t0 = C::now();
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
    auto t1 = C::now();
    double d = (t1 - t0).count();
    return {d, gigabytes(x.size())};
}

double bench_step(stdml::collective::session &session,
                  const std::vector<size_t> &sizes)
{
    std::vector<fake_cpu_buffer_t<float>> buffers;
    for (size_t i = 0; i < sizes.size(); ++i) {
        const std::string name = "variable:" + std::to_string(i++);
        buffers.emplace_back(name, sizes[i]);
    }
    {
        auto t0 = C::now();
        for (auto &b : buffers) {
            auto [d, g] = bench_all_reduce(session, b.send_buf, b.recv_buf);
            // std::cout << std::format("{}", g / d) << "GiB/s" << std::endl;
            // printf("%.3f GiB/s\n", g / d);
        }
        auto t1 = C::now();
        std::chrono::duration<double> d = (t1 - t0);
        return d.count();
    }
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

int main(int argc, char *argv[])
{
    TRACE_SCOPE(__func__);

    const auto sizes = read_int_list(argv[1]);
    const int times = std::stoi(argv[2]);
    const auto tot = std::accumulate(sizes.begin(), sizes.end(), 0);

    log(PRINT) << sizes.size() << "tensors";
    log(PRINT) << "total size" << tot;

    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session session = peer.join();
    size_t multiplier = 4 * (session.size() - 1);

    for (int i = 0; i < times; ++i) {
        log(PRINT) << "bench step" << i;
        auto d = bench_step(session, sizes);
        printf("%.3f GiB/s\n", gigabytes(multiplier * tot * 4) / d);
    }
    return 0;
}
