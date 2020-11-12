#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

void example_1()
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    stdml::collective::session session = peer.join();
    std::cout << "session joined" << std::endl;

    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    const int n = 10;
    std::vector<int8_t> x(n);
    std::vector<int8_t> y(n);
    std::iota(x.begin(), x.end(), 'a');
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
}

int main()
{
    example_1();
    std::cout << "example finished" << std::endl;
    return 0;
}
