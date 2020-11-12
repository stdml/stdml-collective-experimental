#include <cassert>
#include <cstring>
#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/log.hpp>
#include <stdml/bits/session.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
class workspace_state
{
    std::mutex mu_;

    const workspace *w;
    uint32_t recv_count;

  public:
    workspace_state(const workspace *w) : w(w), recv_count(0) {}

    const workspace *operator->() const { return w; }

    const void *effective_data()
    {
        std::lock_guard<std::mutex> _(mu_);  //
        if (recv_count > 0) {
            return w->recv;
        } else {
            return w->send;
        }
    }

    void add_to(const void *data)
    {
        const void *ptr = effective_data();
        std::lock_guard<std::mutex> _(mu_);
        reduce(w->recv, data, ptr, w->count, w->dt, w->op);
        ++recv_count;
    }

    void replace(const void *data)
    {
        std::lock_guard<std::mutex> _(mu_);
        std::memcpy(w->recv, data, w->data_size());
        ++recv_count;
    }
};

struct recv {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    recv(const session *sess, workspace_state *state, bool reduce)
        : sess(sess), state(state), reduce(reduce)
    {
    }

    void operator()(const peer_id &id)
    {
        if (reduce) {
            mailbox::Q *q = sess->mailbox_->require(id, (*state)->name);
            auto b = q->get();
            state->add_to(b.data.get());
        } else {
            slotbox::S *s = sess->slotbox_->require(id, (*state)->name);
            s->waitQ.put((*state)->recv);
            void *ptr = s->recvQ.get();
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
        : sess(sess), state(state), reduce(reduce)
    {
    }

    void operator()(const peer_id &id)
    {
        uint32_t flags = 0;
        if (!reduce) { flags |= rchan::message_header::wait_recv_buf; };
        auto client = sess->client_pool_->require(rchan::conn_collective);
        client->send(id, (*state)->name.c_str(), state->effective_data(),
                     (*state)->data_size(), flags);
    }
};

template <typename F, typename L>
void par(F f, const L &xs)
{
    for (const auto x : xs) { f(x); }
}

template <typename F, typename L>
void seq(F f, const L &xs)
{
    for (const auto x : xs) { f(x); }
}

void session::run_graphs(const workspace &w,
                         const std::vector<const graph *> &gs)
{
    workspace_state state(&w);
    for (const auto g : gs) {
        // log(PRINT) << *g;
        const auto prevs = peers_[g->prevs(rank_)];
        const auto nexts = peers_[g->nexts(rank_)];
        // log(PRINT) << "prevs" << prevs;
        // log(PRINT) << "nexts" << nexts;
        if (g->self_loop(rank_)) {
            par(recv(this, &state, true), prevs);
            par(send(this, &state, true), nexts);
        } else {
            seq(recv(this, &state, false), prevs);
            par(send(this, &state, false), nexts);
        }
    }
}

void session::run_graph_pair_list(const workspace &w,
                                  const graph_pair_list &gps)
{
    // TODO: partition workspace evenly and run them on the graph pair list
    const auto &p0 = gps.pairs[0];
    run_graphs(w, {&p0.reduce_graph, &p0.broadcast_graph});
}

void session::all_reduce(const void *input, void *output, size_t count,
                         dtype dt, reduce_op op)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = op,
        .name = "",
    };
    run_graph_pair_list(w, all_reduce_topo_);
}

void session::barrier()
{
    uint32_t x = 1;
    all_reduce(x);
}

void session::_ring_handshake()
{
    const size_t next_rank = (rank_ + 1) % peers_.size();
    const auto next = peers_[next_rank];
    log() << "next:" << next;
    auto client = client_pool_->require(rchan::conn_ping);
    using namespace std::string_literals;
    const auto msg = "hello world"s;
    client->send(next, "ping", msg.data(), msg.size());
}
}  // namespace stdml::collective
