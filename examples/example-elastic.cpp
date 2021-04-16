#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdml/bits/collective/stat.hpp>
#include <stdml/collective>

using stdml::collective::log;
using stdml::collective::PRINT;

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

int sync_step(stdml::collective::session &sess, int &i)
{
    int j = sess.all_reduce(i, stdml::collective::max);
    log() << "sync step" << i << " -> " << j;
    return j;
}

void elastic_train(int max_step, model m, const std::map<int, int> &schedule)
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    auto sess = peer.join_elastic();
    std::cout << "session joined, rank: " << sess->rank()
              << ", size: " << sess->size() << std::endl;

    bool synced = false;
    for (int i = 0; i < max_step; ++i) {
        if (!synced) {
            i = sync_step(*sess, i);
            synced = true;
        }
        log() << "step" << i;
        {
            // TRACE_SCOPE("train step");
            for (size_t i = 0; i < m.xs.size(); ++i) {
                log() << "  - tensor" << i;
                sess->all_reduce(m.xs[i].data(), m.ys[i].data(),
                                 m.xs[i].size());
            }
        }
        if (schedule.count(i) > 0) {
            auto new_size = schedule.at(i);
            log() << "apply resize schedule at" << i << "with new size"
                  << new_size;
            const auto [changed, detached] = peer.resize(sess, new_size);
            if (detached) {
                log() << "detached since step" << i;
                break;
            }
            if (changed) {
                log() << "changed since step" << i;
                // TODO: update session
                synced = false;
            }
        }
    }
}

using schedule = std::map<int, int>;
schedule parse_schedule(int argc, char *argv[], int &max_step)
{
    if (argc < 2) {
        throw std::invalid_argument(argv[0]);
    }
    sscanf(argv[1], "--max-step=%d", &max_step);
    schedule s;
    for (int i = 2; i < argc; ++i) {
        int k, v;
        sscanf(argv[i], "%d:%d", &k, &v);
        s[k] = v;
    }
    return s;
}

void log_args(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        log() << "argv[" << i << "] =" << argv[i];
    }
}

int main(int argc, char *argv[])
{
    stdml::collective::enable_log();
    log_args(argc, argv);
    int max_step = 0;
    auto s = parse_schedule(argc, argv, max_step);
    log() << "max-step: " << max_step;
    for (auto &[k, v] : s) {
        log() << "at step" << k << "resize to" << v;
    }
    model m({1024, 1024});
    elastic_train(max_step, m, s);
    log() << "train finished";
    return 0;
}
