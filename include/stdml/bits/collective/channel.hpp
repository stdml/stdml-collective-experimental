#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace stdml::collective
{
template <typename T>
class channel
{
    const size_t cap;

    std::mutex mu;
    std::queue<T> buffer;

    std::condition_variable cv;

  public:
    channel(size_t cap = 1) : cap(cap)
    {
    }

    T get()
    {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait(lk, [&]() { return buffer.size() > 0; });
        T x = std::move(buffer.front());
        buffer.pop();
        cv.notify_one();
        return x;
    }

    std::optional<T> try_get()
    {
        std::lock_guard<std::mutex> lk(mu);
        if (buffer.empty()) {
            return {};
        }
        T x = std::move(buffer.front());
        buffer.pop();
        cv.notify_one();
        return x;
    }

    void put(T x)
    {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait(lk, [&]() { return buffer.size() < cap; });
        buffer.push(std::move(x));
        cv.notify_all();
    }
};
}  // namespace stdml::collective
