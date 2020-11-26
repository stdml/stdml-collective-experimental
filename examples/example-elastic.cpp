#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include <stdml/bits/collective/stat.hpp>
#include <stdml/collective>
// #include <tracer/simple_log>

using stdml::collective::log;
using stdml::collective::PRINT;

// DEFINE_TRACE_CONTEXTS;

struct model {
    using tensor = std::vector<float>;

    std::vector<tensor> xs;
    std::vector<tensor> ys;

    model(const std::vector<size_t> &sizes)
    {
        for (auto size : sizes) {
            xs.emplace_back(size);
            ys.emplace_back(size);
        }
    }
};

void elastic_train(int max_step, model m, const std::map<int, int> &schedule)
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    stdml::collective::session sess = peer.join();
    std::cout << "session joined, rank: " << sess.rank()
              << ", size: " << sess.size() << std::endl;

    bool synced = false;
    for (int i = 0; i < max_step; ++i) {
        if (!synced) {
            int j = sess.all_reduce(i, stdml::collective::max);
            i = j;
            synced = true;
        }
        log(PRINT) << "step" << i;
        {
            // TRACE_SCOPE("train step");
            for (auto i : std::views::iota((size_t)0, m.xs.size())) {
                log(PRINT) << "  - tensor" << i;
                sess.all_reduce(m.xs[i].data(), m.ys[i].data(), m.xs[i].size());
            }
        }

        if (schedule.count(i) > 0) {
            const auto [changed, detached] = peer.resize();
            if (detached) {
                break;
            }
            if (changed) {
                synced = false;
            }
        }
    }
}

int main()
{
    stdml::collective::enabled_log();
    std::map<int, int> schedule{
        {10, 1}, {20, 2}, {30, 3}, {40, 1}, {50, 4},
    };
    model m({1024, 1024});
    elastic_train(100, m, schedule);
    return 0;
}
