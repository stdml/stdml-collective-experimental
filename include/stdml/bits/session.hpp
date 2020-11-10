#pragma once
#include <cstddef>
#include <iterator>

namespace stdml::collective
{
class session
{
    void all_reduce(const void *input, void *output, size_t count, int dtype,
                    int op);

  public:
    session() {}
    ~session() {}

    template <typename R>
    void all_reduce_sum_f32(const R *begin1, const R *end1, R *begin2)
    {
        const size_t count = std::distance(begin1, end1);
        all_reduce(begin1, begin2, count, 0, 0);
    }
};
}  // namespace stdml::collective
