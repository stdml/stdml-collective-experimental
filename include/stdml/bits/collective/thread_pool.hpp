#pragma once
#include <cstddef>
#include <functional>

#include <stdml/bits/collective/waitgroup.hpp>

namespace stdml::sync
{
class thread_pool
{
  protected:
    using task = std::function<void()>;

  public:
    static thread_pool *New(int n);

    virtual ~thread_pool() = default;

    virtual void add(task f) = 0;

    virtual void wait() = 0;

    template <typename F, typename L>
    void par0(const F &f, const L &xs)
    {
        wait_group wg(xs.size());
        for (const auto &x : xs) {
            add([&, &x = x] {
                f(x);
                wg.done();
            });
        }
        wg.wait();
    }

    template <typename F, typename L>
    void par1(const F &f, const L &xs)
    {
        if (xs.size() < 1) {
            return;
        }
        wait_group wg(xs.size() - 1);
        for (size_t i = 1; i < xs.size(); ++i) {
            add([&, &x = xs[i]] {
                f(x);
                wg.done();
            });
        }
        f(xs[0]);
        wg.wait();
    }

    template <typename F, typename L, bool ex_situ = false>
    void par(const F &f, const L &xs)
    {
        ex_situ ? par0(f, xs) : par1(f, xs);
    }
};
}  // namespace stdml::sync
