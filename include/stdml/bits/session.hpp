#pragma once
#include <cstddef>
#include <iterator>

#include <stdml/bits/address.hpp>

namespace stdml::collective
{
class session
{
    const peer_list peers_;
    const size_t rank_;

    void all_reduce(const void *input, void *output, size_t count, int dtype,
                    int op);

  public:
    session(const peer_id self, const peer_list peers)
        : peers_(peers),
          rank_(std::find(peers.begin(), peers.end(), self) - peers.begin())
    {
        printf("rank=%d\n", (int)rank_);
    }

    ~session() {}

    template <typename R>
    void all_reduce_sum_f32(const R *begin1, const R *end1, R *begin2)
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, 0, 0);
    }
};
}  // namespace stdml::collective
