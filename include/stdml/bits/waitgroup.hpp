#pragma once
#include <condition_variable>
#include <mutex>

namespace stdml::sync
{
class wait_group
{
    std::condition_variable cv_;
    std::mutex mu_;
    size_t n_;

  public:
    wait_group(size_t n = 0);
    void add(size_t n);
    void done();
    void wait();
};
}  // namespace stdml::sync
