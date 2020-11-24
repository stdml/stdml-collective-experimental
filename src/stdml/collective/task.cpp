#include <ranges>

#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
void task::finish()
{
    while (!finished()) { poll(); }
}

task *task::par(std::vector<std::unique_ptr<task>> tasks)
{
    return new parallel_tasks(std::move(tasks));
}

task *task::seq(std::vector<std::unique_ptr<task>> tasks)
{
    return new sequence_tasks(std::move(tasks));
}

simple_task::simple_task(std::function<void()> f) : f_(f), finished_(false) {}

void simple_task::poll()
{
    if (finished_) { return; }
    f_();
    finished_ = true;
}

bool simple_task::finished() { return finished_; }

sequence_tasks::sequence_tasks(std::vector<std::unique_ptr<task>> tasks)
    : tasks_(std::move(tasks)), finished_(0)
{
}

void sequence_tasks::poll()
{
    if (finished_ >= tasks_.size()) { return; }
    tasks_[finished_]->poll();
    if (tasks_[finished_]->finished()) { ++finished_; }
}

bool sequence_tasks::finished() { return finished_ == tasks_.size(); }

parallel_tasks::parallel_tasks(std::vector<std::unique_ptr<task>> tasks)
    : finished_(tasks.size()), running_(tasks.size()), tasks_(std::move(tasks))
{
    std::fill(finished_.begin(), finished_.end(), false);
}

void parallel_tasks::poll()
{
    if (running_ <= 0) { return; }
    for (auto i : std::views::iota((size_t)0, tasks_.size())) {
        if (finished_[i]) { continue; }
        tasks_[i]->poll();
        if (tasks_[i]->finished()) {
            finished_[i] = true;
            --running_;
        }
    }
}

bool parallel_tasks::finished() { return running_ == 0; }
}  // namespace stdml::collective
