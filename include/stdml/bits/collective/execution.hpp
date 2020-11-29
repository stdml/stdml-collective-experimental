#pragma once
#include <execution>
#include <thread>
#include <vector>

#include <stdml/bits/collective/thread_pool.hpp>

namespace stdml::collective
{
template <typename F, typename L>
void par(const F &f, const L &xs)
{
    std::vector<std::thread> ths;
    ths.reserve(xs.size());
    for (const auto &x : xs) {
        ths.emplace_back([&, &x = x] { f(x); });
    }
    for (auto &th : ths) {
        th.join();
    }
}

template <typename F, typename L>
void seq(const F &f, const L &xs)
{
    for (const auto &x : xs) {
        f(x);
    }
}

template <typename P, typename F, typename L>
struct fmap_t;

template <typename F, typename L>
struct fmap_t<std::execution::parallel_policy, F, L> {
    void operator()(std::execution::parallel_policy, const F &f, const L &xs)
    {
        par(f, xs);
    }
};

template <typename F, typename L>
struct fmap_t<sync::thread_pool *, F, L> {
    void operator()(sync::thread_pool *pool, const F &f, const L &xs)
    {
        if (pool) {
            pool->par(f, xs);
        } else {
            par(f, xs);
        }
    }
};

template <typename F, typename L>
struct fmap_t<std::execution::sequenced_policy, F, L> {
    void operator()(std::execution::sequenced_policy, const F &f, const L &xs)
    {
        seq(f, xs);
    }
};

template <typename P, typename F, typename L>
void fmap(const P p, const F &f, const L &xs)
{
    fmap_t<P, F, L>()(p, f, xs);
}
}  // namespace stdml::collective
