#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/stat.hpp>

#include <cxxabi.h>

template <typename T>
std::string task_name()
{
    int status = 0;
    return abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
}

namespace stdml::collective
{
class task;

class runtime
{
  public:
    virtual ~runtime() = default;

    static runtime *New(size_t m);

    virtual void par(std::vector<std::unique_ptr<task>> &tasks) = 0;
};

static int task_cnt = 0;
class task
{
  public:
    task()
    {
        task_cnt++;
    }

    virtual ~task() = default;
    virtual void poll() = 0;
    virtual bool finished() = 0;

    void finish();

    static task *par(std::vector<std::unique_ptr<task>> tasks,
                     runtime *rt = nullptr);
    static task *seq(std::vector<std::unique_ptr<task>> tasks);

    template <typename F, typename L>
    static std::vector<std::unique_ptr<task>> fmap(const F &f, const L &xs)
    {
        std::vector<std::unique_ptr<task>> tasks;
        for (const auto &x : xs) {
            tasks.emplace_back(f(x));
        }
        return tasks;
    }
};

template <typename Task>
class monitored_task : public task
{
    using clock = std::chrono::high_resolution_clock;
    using instant = std::chrono::time_point<clock>;

    Task task_;
    size_t polled_;
    instant t0_;

  public:
    monitored_task(Task task)
        : task_(std::move(task)), polled_(0), t0_(clock::now())
    {
    }

    void poll() override
    {
        ++polled_;
        task_.poll();
        return;
    }

    bool finished() override
    {
        return task_.finished();
    }

    ~monitored_task()
    {
        summary();
    }

    void summary()
    {
        auto d = clock::now() - t0_;
        auto s = std::to_string(polled_);
        s.resize(8, ' ');
        log() << "polled" << s << "times  | "  //
              << "took" << rchan::show_duration(d) << " | "
              << "@" << task_name<Task>();
    }
};

class noop_task : public task
{
  public:
    void poll() override
    {
        return;
    }

    bool finished() override
    {
        return true;
    }
};

class simple_task : public task
{
    std::function<void()> f_;
    bool finished_;

  public:
    simple_task(std::function<void()> f);

    void poll() override;

    bool finished() override;
};

class sequence_tasks : public task
{
    std::vector<std::unique_ptr<task>> tasks_;
    size_t finished_;

  public:
    sequence_tasks(std::vector<std::unique_ptr<task>> tasks);

    void poll() override;

    bool finished() override;
};

class parallel_tasks : public task
{
    std::vector<bool> finished_;
    size_t running_;
    std::vector<std::unique_ptr<task>> tasks_;
    runtime *rt_;

  public:
    parallel_tasks(std::vector<std::unique_ptr<task>> tasks,
                   runtime *rt = nullptr);

    void poll() override;

    bool finished() override;
};

class task_builder
{
    std::vector<std::unique_ptr<task>> tasks_;

  public:
    void operator<<(task *t)
    {
        tasks_.emplace_back(t);
    }

    void operator<<(std::unique_ptr<task> t)
    {
        tasks_.emplace_back(std::move(t));
    }

    task *par()
    {
        return task::par(std::move(tasks_));
    }

    task *seq()
    {
        return task::seq(std::move(tasks_));
    }
};
}  // namespace stdml::collective
