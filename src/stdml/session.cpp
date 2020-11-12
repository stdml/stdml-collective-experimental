#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/log.hpp>
#include <stdml/bits/session.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
struct recv {
    const session *sess;
    const workspace *w;
    const bool reduce;
    rchan::client *client;

    recv(const session *sess, const workspace *w, bool reduce)
        : sess(sess), w(w), reduce(reduce)
    {
    }

    void operator()(const peer_id &id)
    {
        mailbox::Q *q = sess->mailbox_->require(id, w->name);
        log() << "required queue for recv " << id << "@" << w->name << ":" << q;
        log() << "expect msg from" << id << "with name length" << w->name.size()
              << "in queue" << q;
        q->get();
        log() << "arrived msg from" << id;
    }
};

struct send {
    const session *sess;
    const workspace *w;
    const bool reduce;
    rchan::client *client;

    send(const session *sess, const workspace *w, bool reduce)
        : sess(sess), w(w), reduce(reduce)
    {
    }

    void operator()(const peer_id &id)
    {
        uint32_t flags = 0;
        if (reduce) { flags |= rchan::message_header::wait_recv_buf; };
        auto client = sess->client_pool_->require(rchan::conn_collective);
        client->send(id, w->name.c_str(), w->send, w->data_size(), flags);
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
    // auto client = client_pool_->require(rchan::conn_collective);
    for (const auto g : gs) {
        const auto prevs = peers_[g->prevs(rank_)];
        const auto nexts = peers_[g->nexts(rank_)];
        if (g->self_loop(rank_)) {
            par(recv(this, &w, true), prevs);
            par(send(this, &w, true), nexts);
        } else {
            seq(recv(this, &w, false), prevs);
            par(send(this, &w, false), nexts);
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
    log() << "next:" << next;
    auto client = client_pool_->require(rchan::conn_ping);
    using namespace std::string_literals;
    const auto msg = "hello world"s;
    client->send(next, "ping", msg.data(), msg.size());
}
}  // namespace stdml::collective
