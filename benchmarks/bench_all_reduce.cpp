#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

void bench_all_reduce(stdml::collective::session &session, const size_t n)
{
    std::vector<float> x(n);
    std::vector<float> y(n);
    std::iota(x.begin(), x.end(), 1);
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
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

void example_1()
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    stdml::collective::session session = peer.join();
    std::cout << "session joined, rank: " << session.rank()
              << ", size: " << session.size() << std::endl;

    {
        const int n = 10;
        std::vector<int> x(n);
        std::vector<int> y(n);
        std::fill(x.begin(), x.end(), session.rank());
        std::fill(y.begin(), y.end(), -10);
        // std::iota(x.begin(), x.end(), 1);

        // std::cout << std::string(80, '-') << std::endl;
        session.all_reduce(x.data(), x.data() + x.size(), y.data());
        // std::cout << std::string(80, '-') << std::endl;
        pprint(x);
        pprint(y);
    }
    // bench_all_reduce(session);
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
    const auto sizes = read_int_list(argv[1]);
    const int times = std::stoi(argv[2]);
    log(PRINT) << sizes.size() << "tensors";
    log(PRINT) << "total size"
               << std::accumulate(sizes.begin(), sizes.end(), 0);
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session session = peer.join();
    for (int i = 0; i < times; ++i) {
        for (auto size : sizes) { bench_all_reduce(session, size); }
    }
    return 0;
}
