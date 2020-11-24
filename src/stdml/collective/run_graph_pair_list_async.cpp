#include <cassert>
#include <execution>
#include <iostream>

#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/session.hpp>
#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
class recv_onto : public task
{
  public:
    void poll() override {}

    bool finished() override { return true; }
};

class recv_into : public task
{
  public:
    void poll() override {}

    bool finished() override { return true; }
};

class send_onto : public task
{
  public:
    void poll() override {}

    bool finished() override { return true; }
};

class send_into : public task
{
  public:
    void poll() override {}

    bool finished() override { return true; }
};

struct recv {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    recv(const session *sess, workspace_state *state, bool reduce)
        : sess(sess), state(state), reduce(reduce)
    {
    }

    task *operator()(const peer_id &id) const { return new noop_task(); }
};

struct send {
    const session *sess;
    //  const
    workspace_state *state;
    const bool reduce;

    send(const session *sess, workspace_state *w, bool reduce)
        : sess(sess), state(w), reduce(reduce)
    {
    }

    task *operator()(const peer_id &id) const { return new noop_task(); }
};

task *run_graphs(session *sess, const workspace &w,
                 const std::vector<const graph *> &gs)
{
    const auto &peers = sess->peers();
    auto rank = sess->rank();

    workspace_state state(&w);
    task_builder steps;
    for (const auto g : gs) {
        const auto prevs = peers[g->prevs(rank)];
        const auto nexts = peers[g->nexts(rank)];
        if (g->self_loop(rank)) {
            steps << task::par(task::fmap(recv(sess, &state, true), prevs));
            steps << task::par(task::fmap(send(sess, &state, true), nexts));
        } else {
            if (prevs.size() == 0 && state.recv_count() == 0) {
                steps << new simple_task([&] { state.forward(); });
            } else {
                steps << task::seq(
                    task::fmap(recv(sess, &state, false), prevs));
            }
            steps << task::par(task::fmap(send(sess, &state, false), nexts));
        }
    }
    return steps.seq();
}

template <typename T>
T ceil_div(T a, T b)
{
    return (a / b) + (a % b ? 1 : 0);
}

size_t name_based_hash(size_t i, const std::string &name)
{
    size_t h = 0;
    for (const auto &c : name) { h += c * c; }
    return h;
}

size_t run_graph_pair_list_async(session *sess, const workspace &w,
                                 const graph_pair_list &gps, size_t chunk_size)
{
    const size_t k = ceil_div(w.data_size(), chunk_size);
    const auto ws = w.split(k);
    const auto f = [&](size_t i) {
        const size_t j = name_based_hash(i, ws[i].name);
        const auto &[g0, g1] = gps.choose(j);
        return run_graphs(sess, ws[i], {g0, g1});
    };
    auto t = task::par(task::fmap(f, std::views::iota((size_t)0, ws.size())));
    t->finish();
    delete t;
    return k;
}
}  // namespace stdml::collective
