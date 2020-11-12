#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

int main()
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    stdml::collective::session session = peer.join();
    std::cout << "session joined" << std::endl;

    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    const int n = 1 << 10;
    std::vector<float> x(n);
    std::vector<float> y(n);
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
    return 0;
}
