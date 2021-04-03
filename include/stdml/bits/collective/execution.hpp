#pragma once
#include <cstddef>
#include <execution>
#include <thread>
#include <vector>

#include <stdml/bits/collective/std/execution.hpp>
#include <stdml/bits/collective/thread_pool.hpp>

namespace stdml::collective
{
template <typename F, typename L>
void par0(const F &f, const L &xs)
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
void par1(const F &f, const L &xs)
{
    if (xs.size() < 1) {
        return;
    }
    std::vector<std::thread> ths;
    ths.reserve(xs.size() - 1);
    for (size_t i = 1; i < xs.size(); ++i) {
        ths.emplace_back([&, &x = xs[i]] { f(x); });
    }
    f(xs[0]);
    for (auto &th : ths) {
        th.join();
    }
}

template <typename F, typename L, bool ex_situ = false>
void par(const F &f, const L &xs)
{
    ex_situ ? par0(f, xs) : par1(f, xs);
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
