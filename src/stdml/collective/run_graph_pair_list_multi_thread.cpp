#include <cstddef>
#include <cstdint>
#include <execution>
#include <vector>

#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
// https://dev.azure.com/haibara/stdml/_workitems/edit/1/
// struct name can't be recv, to avoid dynamic link conflict
struct recv_task {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    recv_task(const session *sess, workspace_state *state, bool reduce)
        : sess(sess), state(state), reduce(reduce)
    {
    }

    void operator()(const peer_id &id) const
    {
        if (reduce) {
            auto q = sess->mailbox_->require(id, (*state)->name);
            auto b = q->get();
            state->add_to(b.data.get());
        } else {
            auto q = sess->slotbox_->require(id, (*state)->name);
            q->get((*state)->recv);
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
        if (!reduce) {
            flags |= rchan::message_header::wait_recv_buf;
        };
        auto client = sess->client_pool_->require(rchan::conn_collective);
        client->send(id, (*state)->name.c_str(), state->effective_data(),
                     (*state)->data_size(), flags);
    }
};

void run_graphs_multi_thread(session *sess, const workspace &w,
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
            fmap(par, recv_task(sess, &state, true), prevs);
            fmap(par, send(sess, &state, true), nexts);
        } else {
            if (prevs.size() == 0 && state.recv_count() == 0) {
                state.forward();
            } else {
                fmap(seq, recv_task(sess, &state, false), prevs);
            }
            fmap(par, send(sess, &state, false), nexts);
        }
    }
}

size_t run_graph_pair_list_multi_thread(session *sess, const workspace &w,
                                        const graph_pair_list &gps,
                                        size_t chunk_size)
{
    const auto pw = split_work(w, gps, chunk_size);
    const auto f = [&](auto &wgp) {
        const auto &[w, gp] = wgp;
        run_graphs_multi_thread(sess, w, gp);
    };
    // don't use thread pool here
    // fmap(std::execution::par, f, pw);
    fmap(std::execution::seq, f, pw);  // no need to use par!
    return pw.size();
}
}  // namespace stdml::collective
