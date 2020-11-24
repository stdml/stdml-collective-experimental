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
}  // namespace stdml::collective
