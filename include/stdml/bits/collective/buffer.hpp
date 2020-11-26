#pragma once
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

#include <stdml/bits/collective/dtype.hpp>
#include <stdml/bits/collective/stat.hpp>

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

template <typename T>
std::pair<T, T> divide(T a, T b)
{
    T q = a / b;
    T r = a - b * q;
    return {q, r};
}

template <typename T>
struct interval {
    T begin;
    T end;

    T len() const
    {
        return end - begin;
    }
};

template <typename T>
std::vector<interval<T>> even_partition(const interval<T> &i, T k)
{
    const auto [q, r] = divide(i.len(), k);
    std::vector<interval<T>> ps;
    ps.reserve(k);
    T off = i.begin;
    for (T j = 0; j < k; ++j) {
        const T size = j < r ? q + 1 : q;
        // ps.emplace_back(off, off + size);// requires c++20
        ps.push_back({off, off + size});
        off += size;
    }
    return ps;
}

struct workspace {
    const void *send;
    void *recv;
    size_t count;
    dtype dt;
    reduce_op op;
    std::string name;

    size_t data_size() const
    {
        return count * dtype_size(dt);
    }

    workspace slice(size_t i, size_t j) const
    {
        const size_t s = dtype_size(dt);
        return {
            .send = (char *)send + i * s,
            .recv = (char *)recv + i * s,
            .count = j - i,
            .dt = dt,
            .op = op,
            .name = "part::" + name + "[" + std::to_string(i) + ":" +
                    std::to_string(j) + "]",
        };
    }

    std::vector<workspace> split(size_t k) const
    {
        std::vector<workspace> ws;
        ws.reserve(k);
        const interval<size_t> all = {0, count};
        for (const auto &i : even_partition(all, k)) {
            ws.push_back(slice(i.begin, i.end));
        }
        return ws;
    }
};

class workspace_state
{
    std::mutex mu_;

    const workspace *w;
    uint32_t recv_count_;

  public:
    workspace_state(const workspace *w) : w(w), recv_count_(0)
    {
    }

    const workspace *operator->() const
    {
        return w;
    }

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
        STDML_COLLECTIVE_PROFILE_RATE(__func__, w->count * 4);
        reduce(w->recv, data, ptr, w->count, w->dt, w->op);
        ++recv_count_;
    }

    void replace(const void *data)
    {
        std::lock_guard<std::mutex> _(mu_);
        std::memcpy(w->recv, data, w->data_size());
        ++recv_count_;
    }

    void forward()
    {
        std::memcpy(w->recv, w->send, w->data_size());
    }

    uint32_t recv_count() const
    {
        return recv_count_;
    }
};
}  // namespace stdml::collective
