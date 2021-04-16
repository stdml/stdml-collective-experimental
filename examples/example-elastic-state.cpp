#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdml/bits/collective/stat.hpp>
#include <stdml/collective>

#include <std/ranges>

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

class elastic_state
{
    size_t global_step_;
    bool synced_;
    bool detached_;

  public:
    elastic_state() : global_step_(0), synced_(false), detached_(false)
    {
    }

    operator size_t() const
    {
        return global_step_;
    }

    void operator++()
    {
        ++global_step_;
    }

    bool should_stop(size_t max_step) const
    {
        return detached_ || global_step_ >= max_step;
    }

    void sync(stdml::collective::session &sess)
    {
        auto j = sess.all_reduce(global_step_, stdml::collective::max);
        global_step_ = j;
        synced_ = true;
    }

    void resize(stdml::collective::peer &peer,
                std::unique_ptr<stdml::collective::session> &sess,
                size_t new_size)
    {
        auto result = peer.resize(sess, new_size);
        if (result.detached) {
            detached_ = true;
        }
        if (result.changed) {
            synced_ = false;
        }
    }
};

void train_step(model &m, std::unique_ptr<stdml::collective::session> &sess)
{
    for (auto i : std::views::iota((size_t)0, m.xs.size())) {
        log() << "  - tensor" << i;
        sess->all_reduce(m.xs[i].data(), m.ys[i].data(), m.xs[i].size());
    }
}

using adapt_func =
    std::function<void(elastic_state &, stdml::collective::peer &,
                       std::unique_ptr<stdml::collective::session> &)>;
using step_func =
    std::function<void(model &, std::unique_ptr<stdml::collective::session> &)>;

void elastic_train(int max_step, model &m, const step_func &train_step,
                   const adapt_func &adapt)
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    auto sess = peer.join_elastic();
    std::cout << "session joined, rank: " << sess->rank()
              << ", size: " << sess->size() << std::endl;

    for (elastic_state state; !state.should_stop(max_step); ++state) {
        state.sync(*sess);
        log() << "step" << static_cast<int>(state);
        train_step(m, sess);
        adapt(state, peer, sess);
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

int main(int argc, char *argv[])
{
    stdml::collective::enable_log();
    int max_step = 0;
    auto s = parse_schedule(argc, argv, max_step);
    log() << "max-step: " << max_step;
    for (auto &[k, v] : s) {
        log() << "will resize to" << v << "at step" << k;
    }
    model m({1024, 1024});

    auto adapt = [&](elastic_state &state, stdml::collective::peer &peer,
                     std::unique_ptr<stdml::collective::session> &sess) {
        const size_t i = state;
        if (s.count(i) > 0) {
            auto new_size = s.at(i);
            state.resize(peer, sess, new_size);
        }
    };
    elastic_train(max_step, m, train_step, adapt);
    log() << argv[0] << "finished";
    return 0;
}
