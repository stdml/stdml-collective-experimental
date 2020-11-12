#pragma once
#include <memory>

#include <stdml/bits/dtype.hpp>

namespace stdml::collective
{
struct buffer {
    std::unique_ptr<char[]> data;
    uint32_t len;
};

inline buffer alloc_buffer(uint32_t n)
{
    return buffer{
        .data = std::unique_ptr<char[]>(new char[n]),
        .len = n,
    };
}

struct workspace {
    const void *send;
    void *recv;
    size_t count;
    dtype dt;
    reduce_op op;
    std::string name;

    size_t data_size() const { return count * dtype_size(dt); }
};
}  // namespace stdml::collective
