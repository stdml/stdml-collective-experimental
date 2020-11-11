#pragma once
#include <cstddef>
#include <iterator>

#include <stdml/bits/address.hpp>
#include <stdml/bits/dtype.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{

struct workspace {
    const void *send;
    void *recv;
    size_t count;
    dtype dt;
    reduce_op op;
    std::string name;
};

class session
{
    const peer_list peers_;
    const size_t rank_;

    graph_pair_list all_reduce_topo_;

    void run_graphs(const workspace &w, const std::vector<const graph *> &gs);
    void run_graph_pair_list(const workspace &w, const graph_pair_list &gps);

    void all_reduce(const void *input, void *output, size_t count, dtype dt,
                    reduce_op op);

    void barrier();
    void ring_handshake();

  public:
    session(const peer_id self, const peer_list peers, const strategy s = star)
        : peers_(peers),
          rank_(std::find(peers.begin(), peers.end(), self) - peers.begin()),
          all_reduce_topo_(make_graph_pair_list(s, peers.size()))
    {
        printf("rank=%d\n", (int)rank_);
        ring_handshake();
    }

    ~session() {}

    template <typename R>
    void all_reduce(const R *begin1, const R *end1, R *begin2,
                    reduce_op op = sum)
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, type<R>(), op);
    }
};
}  // namespace stdml::collective
