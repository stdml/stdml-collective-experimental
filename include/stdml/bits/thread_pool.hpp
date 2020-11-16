#pragma once
#include <functional>
#include <stdml/bits/waitgroup.hpp>

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

    template <typename F, typename L>
    void par(const F &f, const L &xs)
    {
        wait_group wg(xs.size());
        for (const auto &x : xs) {
            add([&, x = x] {
                f(x);
                wg.done();
            });
        }
        wg.wait();
    }
};
}  // namespace stdml::sync
