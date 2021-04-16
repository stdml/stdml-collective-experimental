#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include <stdml/bits/collective/thread_pool.hpp>

namespace stdml::sync
{
class semaphore
{
    int value;
    std::mutex mu;
    std::condition_variable cv;

    void get(int n)
    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock, [&] { return value >= n; });
        value -= n;
    }

    void put(int n)
    {
        {
            std::lock_guard<std::mutex> _(mu);
            value += n;
        }
        cv.notify_one();
    }

  public:
    semaphore() : value(0)
    {
    }

    void get_one()
    {
        get(1);
    }

    void put_one()
    {
        put(1);
    }
};

class thread_pool_impl : public thread_pool
{
    class state
    {
        int waiting;
        int running;
        bool stopped;

      public:
        state() : waiting(0), running(0), stopped(false)
        {
        }

        void add()
        {
            ++waiting;
        }

        void start()
        {
            assert(waiting > 0);
            --waiting;
            ++running;
        }

        void finish()
        {
            assert(running > 0);
            --running;
        }

        void stop()
        {
            stopped = true;
        }

        bool is_zero() const
        {
            return waiting == 0 && running == 0;
        }

        bool is_stopped() const
        {
            return stopped;
        }
    };

    using thread_pool::task;

    std::vector<std::thread> ths_;

    std::mutex mu;
    std::queue<task> q;
    state s;
    std::condition_variable cv;
    semaphore sema;

    void worker()
    {
        for (;;) {
            sema.get_one();
            {
                std::lock_guard<std::mutex> _(mu);
                if (s.is_stopped()) {
                    break;
                }
            }
            task f;
            {
                std::lock_guard<std::mutex> _(mu);
                f = std::move(q.front());
                q.pop();
                s.start();
            }
            f();
            {
                std::lock_guard<std::mutex> _(mu);
                s.finish();
            }
            cv.notify_one();
        }
    }

    void add_worker()
    {
        ths_.emplace_back([this] { worker(); });
    }

  public:
    thread_pool_impl(int n = std::thread::hardware_concurrency())
    {
        ths_.reserve(n);
        for (int i = 0; i < n; ++i) {
            add_worker();
        }
    }

    ~thread_pool_impl()
    {
        {
            std::lock_guard<std::mutex> _(mu);
            s.stop();
        }
        for (const auto &_ [[gnu::unused]] : ths_) {
            sema.put_one();
        }
        wait();
        for (auto &th : ths_) {
            th.join();
        }
    }

    void wait() override
    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock, [&] { return s.is_zero(); });
    }

    void add(task f) override
    {
        {
            std::lock_guard<std::mutex> _(mu);
            q.push(f);
            s.add();
        }
        sema.put_one();
    }
};

thread_pool *thread_pool::New(int n)
{
    return new thread_pool_impl(n);
}
}  // namespace stdml::sync
