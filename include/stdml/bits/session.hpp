#pragma once
#include <cstddef>
#include <iterator>

#include <stdml/bits/address.hpp>
#include <stdml/bits/buffer.hpp>
#include <stdml/bits/dtype.hpp>
#include <stdml/bits/mailbox.hpp>
#include <stdml/bits/rchan.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
class session
{
    const peer_list peers_;
    const size_t rank_;

    graph_pair_list all_reduce_topo_;

    void run_graphs(const workspace &w, const std::vector<const graph *> &gs);
    void run_graph_pair_list(const workspace &w, const graph_pair_list &gps);

    void all_reduce(const void *input, void *output, size_t count, dtype dt,
                    reduce_op op, const std::string &name = "");
    void broadcast(const void *input, void *output, size_t count, dtype dt);

    void barrier();
    void _ring_handshake();

  public:
    mailbox *mailbox_;                 // owned by peer
    slotbox *slotbox_;                 // owned by peer
    rchan::client_pool *client_pool_;  // owned by peer

    session(const peer_id self, const peer_list peers, mailbox *mailbox,
            slotbox *slotbox, rchan::client_pool *client_pool,
            const strategy s = star)
        : peers_(peers),
          rank_(std::find(peers.begin(), peers.end(), self) - peers.begin()),
          all_reduce_topo_(make_graph_pair_list(s, peers.size())),
          mailbox_(mailbox),
          slotbox_(slotbox),
          client_pool_(client_pool)
    {
        barrier();
    }

    ~session() {}

    size_t rank() { return rank_; }

    size_t size() { return peers_.size(); }

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

    template <typename R>
    R all_reduce(const R &x, reduce_op op = sum)
    {
        R y;
        all_reduce(&x, &y, 1, op);
        return y;
    }

    template <typename R>
    void broadcast(const R *begin1, const R *end1, R *begin2)
    {
        const size_t count = std::distance(begin1, end1);
        broadcast(begin1, begin2, count, type<R>());
    }
};
}  // namespace stdml::collective
