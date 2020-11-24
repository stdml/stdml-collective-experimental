#include <ranges>

#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
void task::finish()
{
    while (!finished()) { poll(); }
}

void task::par(const std::vector<task *> &tasks)
{
    std::vector<bool> finished(tasks.size());
    std::fill(finished.begin(), finished.end(), false);
    for (auto running = tasks.size(); running > 0;) {
        for (auto i : std::views::iota((size_t)0, tasks.size())) {
            if (finished[i]) { continue; }
            tasks[i]->poll();
            if (tasks[i]->finished()) {
                finished[i] = true;
                --running;
            }
        }
    }
}

void task::seq(const std::vector<task *> &tasks)
{
    for (auto &t : tasks) { t->finish(); }
}

simple_task::simple_task(std::function<void()> f) : f_(f), finished_(false) {}

void simple_task::poll()
{
    if (finished_) { return; }
    f_();
    finished_ = true;
}

bool simple_task::finished() { return finished_; }

chained_task::chained_task(std::vector<std::unique_ptr<task>> tasks)
    : tasks_(std::move(tasks)), finished_(0)
{
}

void chained_task::poll()
{
    if (finished_ >= tasks_.size()) { return; }
    tasks_[finished_]->poll();
    if (tasks_[finished_]->finished()) { ++finished_; }
}

bool chained_task::finished() { return finished_ == tasks_.size(); }
}  // namespace stdml::collective
