#include <cassert>
#include <execution>
#include <iostream>

#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
struct recv {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    recv(const session *sess, workspace_state *state, bool reduce)
        : sess(sess), state(state), reduce(reduce)
    {
    }

    void operator()(const peer_id &id) const
    {
        if (reduce) {
            mailbox::Q *q = sess->mailbox_->require(id, (*state)->name);
            auto b = q->get();
            state->add_to(b.data.get());
        } else {
            slotbox::S *s = sess->slotbox_->require(id, (*state)->name);
            s->waitQ.put((*state)->recv);
            void *ptr [[gnu::unused]] = s->recvQ.get();
            assert(ptr == (*state)->recv);
        }
    }
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

    void operator()(const peer_id &id) const
    {
        uint32_t flags = 0;
        if (!reduce) { flags |= rchan::message_header::wait_recv_buf; };
        auto client = sess->client_pool_->require(rchan::conn_collective);
        client->send(id, (*state)->name.c_str(), state->effective_data(),
                     (*state)->data_size(), flags);
    }
};

void run_graphs(session *sess, const workspace &w,
                const std::vector<const graph *> &gs)
{
    const auto &peers = sess->peers();
    auto rank = sess->rank();
    auto par = sess->pool();
    auto seq = std::execution::seq;

    workspace_state state(&w);
    for (const auto g : gs) {
        const auto prevs = peers[g->prevs(rank)];
        const auto nexts = peers[g->nexts(rank)];
        if (g->self_loop(rank)) {
            fmap(par, recv(sess, &state, true), prevs);
            fmap(par, send(sess, &state, true), nexts);
        } else {
            if (prevs.size() == 0 && state.recv_count() == 0) {
                state.forward();
            } else {
                fmap(seq, recv(sess, &state, false), prevs);
            }
            fmap(par, send(sess, &state, false), nexts);
        }
    }
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
        run_graphs(sess, ws[i], {g0, g1});
    };
    // don't use thread pool here
    fmap(std::execution::par, f, std::views::iota((size_t)0, ws.size()));
    return k;
}
}  // namespace stdml::collective
