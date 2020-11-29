#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

void bench_all_reduce(stdml::collective::session &sess,
                      const size_t n = 1 << 12)
{
    std::vector<float> x(n);
    std::vector<float> y(n);
    std::iota(x.begin(), x.end(), 1);
    sess.all_reduce(x.data(), x.data() + x.size(), y.data());
}

void example_1(stdml::collective::session &sess)
{
    {
        const int n = 10;
        std::vector<int> x(n);
        std::vector<int> y(n);
        std::fill(x.begin(), x.end(), sess.rank());
        std::fill(y.begin(), y.end(), -10);
        // std::iota(x.begin(), x.end(), 1);

        sess.all_reduce(x.data(), x.data() + x.size(), y.data());
    }
}

int main(int argc, char *argv[])
{
    {
        auto peer = stdml::collective::peer::from_env();
        auto sess = peer.join();
        example_1(sess);
        bench_all_reduce(sess);
    }
    std::cout << argv[0] << std::endl;
    return 0;
}
