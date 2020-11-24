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
        runtime_.reset(runtime::New(8));
    } else {
        log() << "not using thread pool";
    }
    // set_affinity(rank_, peers.size());  // FIXME: use local
    barrier();
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
    bool async = parse_env_bool("STDML_COLLECTIVE_USE_ASYNC");
    // log() << __func__ << "using async" << async;
    const auto f = async ? run_graph_pair_list_async : run_graph_pair_list;
    f(this, w, all_reduce_topo_, 1 << 20);
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
