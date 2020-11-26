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
#include <stdml/bits/collective/runtimes/go.hpp>
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
    if (parse_env_bool("STDML_COLLECTIVE_USE_THREAD_POOL")) {
        log() << "using thread pool";
        pool_.reset(sync::thread_pool::New(3));
        runtime_.reset(runtime::New(8));
    } else {
        log() << "not using thread pool";
    }
    // set_affinity(rank_, peers.size());  // FIXME: use local
    barrier();
}

extern void run_graphs(session *sess, const workspace &w,
                       const std::vector<const graph *> &gs);

void session::broadcast(const void *input, void *output, size_t count, dtype dt)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = sum,  // not used
        .name = "",
    };
    run_graphs(this, w, {&all_reduce_topo_.pairs[0].broadcast_graph});
}

void session::all_reduce(const workspace &w)
{
#if STDML_COLLECTIVE_HAVE_GO_RUNTIME
    bool use_go_rt = parse_env_bool("STDML_COLLECTIVE_USE_GO_RUNTIME");
    if (use_go_rt) {
        constexpr auto f = run_graph_pair_list_go_rt;
        f(this, w, all_reduce_topo_, 1 << 20);
        return;
    }
#endif
    bool async = parse_env_bool("STDML_COLLECTIVE_USE_ASYNC");
    const auto f = async ? run_graph_pair_list_async : run_graph_pair_list;
    f(this, w, all_reduce_topo_, 1 << 20);
}

void session::group_all_reduce(std::vector<std::future<workspace>> ws)
{
    // TODO: async
    for (auto &w : ws) {
        auto w1 = w.get();
        all_reduce(w1);
    }
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
    all_reduce(w);
}

bool session::consistent(const void *ptr, size_t size)
{
    size_t size_1 = all_reduce(size, min);
    size_t size_2 = all_reduce(size, max);
    if (size_1 != size_2) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    std::vector<uint8_t> y(size);
    std::vector<uint8_t> z(size);
    all_reduce(ptr, y.data(), size, type<uint8_t>(), min);
    all_reduce(ptr, z.data(), size, type<uint8_t>(), max);
    return y == z;
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

template <typename T>
T ceil_div(T a, T b)
{
    return (a / b) + (a % b ? 1 : 0);
}

size_t name_based_hash(size_t i, const std::string &name)
{
    size_t h = 0;
    for (const auto &c : name) {
        h += c * c;
    }
    return h;
}

std::vector<std::pair<workspace, std::vector<const graph *>>>
split_work(const workspace &w, const graph_pair_list &gps, size_t chunk_size)
{
    const size_t k = ceil_div(w.data_size(), chunk_size);
    const auto ws = w.split(k);
    std::vector<std::pair<workspace, std::vector<const graph *>>> pw;
    pw.reserve(ws.size());
    for (auto i : std::views::iota((size_t)0, ws.size())) {
        const size_t j = name_based_hash(i, ws[i].name);
        const auto &[g0, g1] = gps.choose(j);
        pw.emplace_back(
            std::make_pair(ws[i], std::vector<const graph *>({g0, g1})));
    }
    return pw;
}
}  // namespace stdml::collective
