#include <cstddef>
#include <cstdint>
#include <future>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
session::session(system_config config, size_t version, size_t rank,
                 peer_list peers, peer_list runners, mailbox *mailbox,
                 slotbox *slotbox, rchan::client_pool *client_pool, strategy s)
    : config_(config),
      version_(version),
      rank_(rank),
      peers_(std::move((peers))),
      runners_(std::move(runners)),
      all_reduce_topo_(make_graph_pair_list(s, peers_.size())),
      mailbox_(mailbox),
      slotbox_(slotbox),
      client_pool_(client_pool)
{
    if (config_.thread_pool_size > 0) {
        log() << "using thread pool";
        pool_.reset(sync::thread_pool::New(config_.thread_pool_size));
    } else {
        log() << "not using thread pool";
    }
    if (config_.rt == rt_async) {
        async_runtime_.reset(runtime::New(8));
    }
    if (config_.use_affinity) {
        set_affinity(rank_, peers_.size());  // FIXME: use local
    }
    barrier();
}

extern void run_graphs(session *sess, const workspace &w,
                       const std::vector<const graph *> &gs);

void session::broadcast(const void *input, void *output, size_t count, dtype dt,
                        std::string name)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = sum,  // not used
        .name = std::move(name),
    };
    run_graphs_multi_thread(this, w,
                            {&all_reduce_topo_.pairs[0].broadcast_graph});
}

void session::all_reduce(const workspace &w)
{
    STDML_COLLECTIVE_PROFILE_RATE(__func__, 0);
    const auto f = [sess = this] {
        if (sess->config_.rt == rt_async) {
            return run_graph_pair_list_async;
        }
        if (sess->config_.rt == rt_go) {
#if STDML_COLLECTIVE_HAVE_GO_RUNTIME
            return run_graph_pair_list_go_rt;
#else
            throw std::runtime_error("go runtime not built");
#endif
        }
        return run_graph_pair_list_multi_thread;
    }();
    f(this, w, all_reduce_topo_, 1 << 20);
}

void session::group_all_reduce(std::vector<std::future<workspace>> ws)
{
    // size = min { max |Si|}, \cap Si \neq \emptyset
    auto p = sync::thread_pool::New(76);
    // auto p = sync::thread_pool::New(75);
    for (auto &fw : ws) {
        p->add([this, &fw = fw] {
            auto w = fw.get();
            // log() << __func__ << w.name;
            all_reduce(w);
        });
    }
    p->wait();
    delete p;
}

void session::all_reduce(const void *input, void *output, size_t count,
                         dtype dt, reduce_op op, std::string name)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = op,
        .name = std::move(name),
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
    for (size_t i = 0; i < ws.size(); ++i) {
        const size_t j = name_based_hash(i, ws[i].name);
        const auto &[g0, g1] = gps.choose(j);
        pw.emplace_back(
            std::make_pair(ws[i], std::vector<const graph *>({g0, g1})));
    }
    return pw;
}
}  // namespace stdml::collective
