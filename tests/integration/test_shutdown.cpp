#include <csignal>
#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

#include <stdml/bits/collective/log.hpp>
#include <stdml/collective>

using stdml::collective::log;

void signal_handler(int sig)
{
    log() << "signal" << sig << ":" << strsignal(sig);  //
}

int main()
{
    stdml::collective::enable_log();
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();

    // auto old = std::signal(SIGINT, signal_handler);
    // std::cout << old << std::endl;

    int n = 1024;
    std::vector<float> x(n);
    std::vector<float> y(n);
    int m = 10;
    int times = 10;
    for (int i = 0; i < times; i++) {
        for (int i = 0; i < m; ++i) {
            sess.all_reduce(x.data(), y.data(), n);
            log() << "done"
                  << "all_reduce";
        }
    }
    {
        peer.stop();
    }
    return 0;
}
