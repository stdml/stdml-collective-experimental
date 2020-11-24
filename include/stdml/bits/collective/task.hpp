#pragma once
#include <functional>
#include <memory>
#include <vector>

namespace stdml::collective
{
class task
{
  public:
    virtual ~task() = default;
    virtual void poll() = 0;
    virtual bool finished() = 0;

    void finish();

    static task *par(std::vector<std::unique_ptr<task>> tasks);
    static task *seq(std::vector<std::unique_ptr<task>> tasks);

    template <typename F, typename L>
    static std::vector<std::unique_ptr<task>> fmap(const F &f, const L &xs)
    {
        std::vector<std::unique_ptr<task>> tasks;
        for (const auto &x : xs) { tasks.emplace_back(f(x)); }
        return tasks;
    }
};

class noop_task : public task
{
  public:
    void poll() override { return; }

    bool finished() override { return true; }
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

  public:
    parallel_tasks(std::vector<std::unique_ptr<task>> tasks);

    void poll() override;

    bool finished() override;
};

class task_builder
{
    std::vector<std::unique_ptr<task>> tasks_;

  public:
    void operator<<(task *t) { tasks_.emplace_back(t); }

    task *par() { return task::par(std::move(tasks_)); }
    task *seq() { return task::seq(std::move(tasks_)); }
};
}  // namespace stdml::collective
