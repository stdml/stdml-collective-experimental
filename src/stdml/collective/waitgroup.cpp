#include <stdml/bits/collective/waitgroup.hpp>

namespace stdml::sync
{
wait_group::wait_group(size_t n) : n_(n) {}

void wait_group::add(size_t n)
{
    std::lock_guard<std::mutex> _(mu_);
    n_ += n;
}

void wait_group::done()
{
    {
        std::lock_guard<std::mutex> _(mu_);
        --n_;
    }
    cv_.notify_one();
}

void wait_group::wait()
{
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [&] { return n_ == 0; });
}
}  // namespace stdml::sync
