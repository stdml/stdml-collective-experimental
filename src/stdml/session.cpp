#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/session.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
struct recv {
    recv(bool reduce) {}

    void operator()(const peer_id &id) {}
};

struct send {
    send(bool reduce) {}

    void operator()(const peer_id &id) {}
};

template <typename F>
void par(F f, const peer_list &ps)
{
    for (const auto p : ps) { f(p); }
}

template <typename F>
void seq(F f, const peer_list &ps)
{
    for (const auto p : ps) { f(p); }
}

void session::run_graphs(const workspace &w,
                         const std::vector<const graph *> &gs)
{
    for (const auto g : gs) {
        const auto prevs = peers_.select_ranks(g->prevs(rank_));
        const auto nexts = peers_.select_ranks(g->nexts(rank_));
        if (g->self_loop(rank_)) {
            par(recv(true), prevs);
            par(send(true), nexts);
        } else {
            seq(recv(false), prevs);
            par(send(false), nexts);
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

void session::ring_handshake()
{
    const size_t next_rank = (rank_ + 1) % peers_.size();
    const auto next = peers_[next_rank];

    std::cout << "next: " << (std::string)next << std::endl;
    // connection::dial(next, rchan::conn_ping, peers_[rank_]);
    auto client = clients_->require(rchan::conn_ping);
    using namespace std::string_literals;
    const auto msg = "hello world"s;
    client->send(next, "ping", msg.data(), msg.size());
}
}  // namespace stdml::collective
