#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/session.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
struct recv {
    const workspace *w;
    const bool reduce;
    rchan::client *client;

    recv(const workspace *w, rchan::client *client, bool reduce)
        : w(w), reduce(reduce), client(client)
    {
    }

    void operator()(const peer_id &id)
    {
        // client->send(id, w->name, w->send, 1);  //
    }
};

struct send {
    const workspace *w;
    const bool reduce;
    rchan::client *client;

    send(const workspace *w, rchan::client *client, bool reduce)
        : w(w), reduce(reduce), client(client)
    {
    }

    void operator()(const peer_id &id)
    {
        uint32_t flags = 0;
        if (reduce) { flags |= rchan::message_header::wait_recv_buf; };
        client->send(id, w->name.c_str(), w->send, w->data_size(), flags);  //
    }
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
    auto client = client_pool_->require(rchan::conn_collective);
    for (const auto g : gs) {
        const auto prevs = peers_[g->prevs(rank_)];
        const auto nexts = peers_[g->nexts(rank_)];
        if (g->self_loop(rank_)) {
            par(recv(&w, client, true), prevs);
            par(send(&w, client, true), nexts);
        } else {
            seq(recv(&w, client, false), prevs);
            par(send(&w, client, false), nexts);
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
    auto client = client_pool_->require(rchan::conn_ping);
    using namespace std::string_literals;
    const auto msg = "hello world"s;
    client->send(next, "ping", msg.data(), msg.size());
}
}  // namespace stdml::collective
