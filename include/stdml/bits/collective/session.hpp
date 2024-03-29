#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/buffer.hpp>
#include <stdml/bits/collective/config.hpp>
#include <stdml/bits/collective/dtype.hpp>
#include <stdml/bits/collective/mailbox.hpp>
#include <stdml/bits/collective/rchan.hpp>
#include <stdml/bits/collective/task.hpp>
#include <stdml/bits/collective/thread_pool.hpp>
#include <stdml/bits/collective/topology.hpp>

namespace stdml::collective
{
class session
{
    std::unique_ptr<sync::thread_pool> pool_;
    std::unique_ptr<runtime> async_runtime_;

    const system_config config_;
    const size_t version_;
    const size_t rank_;
    const peer_list peers_;
    const peer_list runners_;

    graph_pair_list all_reduce_topo_;

    void barrier();

  public:
    mailbox *mailbox_;                 // owned by peer
    slotbox *slotbox_;                 // owned by peer
    rchan::client_pool *client_pool_;  // owned by peer

    session(system_config config, size_t version, size_t rank, peer_list peers,
            peer_list runners, mailbox *mailbox, slotbox *slotbox,
            rchan::client_pool *client_pool, strategy s = star);

    size_t rank() const
    {
        return rank_;
    }

    size_t size() const
    {
        return peers_.size();
    }

    const peer_list &peers() const
    {
        return peers_;
    }

    sync::thread_pool *pool() const
    {
        return pool_.get();
    }

    runtime *async_rt() const
    {
        return async_runtime_.get();
    }

    cluster_config cluster() const
    {
        return cluster_config(runners_, peers_);
    }

    size_t version() const
    {
        return version_;
    }

    void all_reduce(const workspace &w);
    void group_all_reduce(std::vector<std::future<workspace>> ws);

    void all_reduce(const void *input, void *output, size_t count, dtype dt,
                    reduce_op op, std::string name = "");

    void all_gather(const void *input, void *output, size_t count, dtype dt,
                    std::string name = "");

    void broadcast(const void *input, void *output, size_t count, dtype dt,
                   std::string name = "");

    void recv(void *output, size_t count, dtype dt, size_t target,
              std::string name = "");

    void send(const void *input, size_t count, dtype dt, size_t target,
              std::string name = "");

    template <typename R>
    void all_reduce(const R *begin1, R *begin2, const size_t count,
                    const std::string &name = "")
    {
        all_reduce(begin1, begin2, count, type<R>(), sum, name);
    }

    template <typename R>
    void all_reduce(const R *begin1, R *begin2, const size_t count,
                    reduce_op op, const std::string &name = "")
    {
        all_reduce(begin1, begin2, count, type<R>(), op, name);
    }

    template <typename R>
    void all_reduce(const R *begin1, const R *end1, R *begin2)
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, type<R>(), sum, "");
    }

    template <typename R>
    void all_reduce(const R *begin1, const R *end1, R *begin2,
                    const std::string &name)
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, type<R>(), sum, name);
    }

    template <typename R>
    void all_reduce(const R *begin1, const R *end1, R *begin2, reduce_op op,
                    const std::string &name = "")
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, type<R>(), op, name);
    }

    template <typename T>
    T all_reduce(const T &x, reduce_op op = sum)
    {
        T y;
        all_reduce(&x, &y, 1, op);
        return y;
    }

    template <typename R>
    void all_gather(const R *input, R *output, const size_t count,
                    const std::string &name = "")
    {
        all_gather(input, output, count, type<R>(), name);
    }

    bool consistent(const void *ptr, size_t size);

    template <typename T>
    bool consistent(const T &x)
    {
        return consistent(&x, sizeof(T));
    }

    template <typename R>
    void broadcast(const R *begin1, const R *end1, R *begin2)
    {
        const size_t count = std::distance(begin1, end1);
        broadcast(begin1, begin2, count, type<R>());
    }

    template <typename T>
    T broadcast(const T &x)
    {
        // static_assert(std::is_standard_layout<T>::value);
        T y;
        using R = int8_t;
        broadcast((const R *)&x, (R *)&y, sizeof(T), type<R>());
        return y;
    }
};

std::vector<std::pair<workspace, std::vector<const graph *>>>
split_work(const workspace &w, const graph_pair_list &gps, size_t chunk_size);

void run_graphs_multi_thread(session *sess, const workspace &w,
                             const std::vector<const graph *> &gs);

size_t run_graph_pair_list_multi_thread(session *sess, const workspace &w,
                                        const graph_pair_list &gps,
                                        size_t chunk_size);

size_t run_graph_pair_list_async(session *sess, const workspace &w,
                                 const graph_pair_list &gps, size_t chunk_size);
}  // namespace stdml::collective
