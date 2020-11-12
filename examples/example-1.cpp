#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

void bench_all_reduce(stdml::collective::session &session,
                      const size_t n = 1 << 12)
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

int main()
{
    example_1();
    std::cout << "example finished" << std::endl;
    // auto g = stdml::collective::start_broadcast_graph_builder(4, 0);
    // std::cout << g.build();

    return 0;
}
