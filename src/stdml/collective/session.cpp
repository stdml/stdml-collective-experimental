#include <cassert>
#include <cstring>
#include <execution>
#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>
#include <thread>

#include <stdml/bits/collective/affinity.hpp>
#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/session.hpp>
#include <stdml/bits/collective/stat.hpp>
#include <stdml/bits/collective/thread_pool.hpp>
#include <stdml/bits/collective/topology.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const std::string &s);

session::session(const peer_id self, const peer_list peers, mailbox *mailbox,
                 slotbox *slotbox, rchan::client_pool *client_pool,
                 const strategy s)
    : peers_(peers),
      rank_(std::find(peers.begin(), peers.end(), self) - peers.begin()),
      all_reduce_topo_(make_graph_pair_list(s, peers.size())),
      mailbox_(mailbox),
      slotbox_(slotbox),
      client_pool_(client_pool)
{
    if (parse_env_bool("STDML_USE_THREAD_POOL")) {
        log() << "using thread pool";
        pool_.reset(sync::thread_pool::New(3));
    } else {
        log() << "not using thread pool";
    }
    // set_affinity(rank_, peers.size());  // FIXME: use local
    barrier();
}

class workspace_state
{
    std::mutex mu_;

    const workspace *w;
    uint32_t recv_count_;

  public:
    workspace_state(const workspace *w) : w(w), recv_count_(0) {}

    const workspace *operator->() const { return w; }

    const void *effective_data()
    {
        if (recv_count_ > 0) {
            return w->recv;
        } else {
            return w->send;
        }
    }

    void add_to(const void *data)
    {
        std::lock_guard<std::mutex> _(mu_);
        const void *ptr = effective_data();
        STDML_PROFILE_RATE(__func__, w->count * 4);
        reduce(w->recv, data, ptr, w->count, w->dt, w->op);
        ++recv_count_;
    }

    void replace(const void *data)
    {
        std::lock_guard<std::mutex> _(mu_);
        std::memcpy(w->recv, data, w->data_size());
        ++recv_count_;
    }

    void forward() { std::memcpy(w->recv, w->send, w->data_size()); }

    uint32_t recv_count() const { return recv_count_; }
};

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

void session::run_graphs(const workspace &w,
                         const std::vector<const graph *> &gs)
{
    auto par = pool_.get();
    auto seq = std::execution::seq;
    workspace_state state(&w);
    for (const auto g : gs) {
        const auto prevs = peers_[g->prevs(rank_)];
        const auto nexts = peers_[g->nexts(rank_)];
        if (g->self_loop(rank_)) {
            fmap(par, recv(this, &state, true), prevs);
            fmap(par, send(this, &state, true), nexts);
        } else {
            if (prevs.size() == 0 && state.recv_count() == 0) {
                state.forward();
            } else {
                fmap(seq, recv(this, &state, false), prevs);
            }
            fmap(par, send(this, &state, false), nexts);
        }
    }
}

size_t name_based_hash(size_t i, const std::string &name)
{
    size_t h = 0;
    for (const auto &c : name) { h += c * c; }
    return h;
}

template <typename T>
T ceil_div(T a, T b)
{
    return (a / b) + (a % b ? 1 : 0);
}

size_t session::run_graph_pair_list(const workspace &w,
                                    const graph_pair_list &gps,
                                    size_t chunk_size)
{
    const size_t k = ceil_div(w.data_size(), chunk_size);
    const auto ws = w.split(k);
    const auto f = [&](size_t i) {
        const size_t j = name_based_hash(i, ws[i].name);
        const auto &[g0, g1] = gps.choose(j);
        run_graphs(ws[i], {g0, g1});
    };
    // don't use thread pool here
    fmap(std::execution::par, f, std::views::iota((size_t)0, ws.size()));
    return k;
}

void session::all_reduce(const void *input, void *output, size_t count,
                         dtype dt, reduce_op op, const std::string &name)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = op,
        .name = name,
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
