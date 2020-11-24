#include <ranges>
#include <thread>

// #include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/task.hpp>
#include <stdml/bits/collective/thread_pool.hpp>

namespace stdml::collective
{
class multi_thread_runtime : public runtime
{
    std::unique_ptr<sync::thread_pool> pool_;

  public:
    multi_thread_runtime(size_t m) : pool_(sync::thread_pool::New(m))
    {
    }

    void par(std::vector<std::unique_ptr<task>> &tasks) override
    {
        sync::wait_group wg(tasks.size());
        for (const auto &t : tasks) {
            pool_->add([&, t = t.get()] {
                t->finish();
                wg.done();
            });
        }
        wg.wait();
    }
};

runtime *runtime::New(size_t m)
{
    return new multi_thread_runtime(m);
}

void task::finish()
{
    while (!finished()) {
        poll();
    }
}

task *task::par(std::vector<std::unique_ptr<task>> tasks, runtime *rt)
{
    return new parallel_tasks(std::move(tasks), rt);
}

task *task::seq(std::vector<std::unique_ptr<task>> tasks)
{
    return new sequence_tasks(std::move(tasks));
}

simple_task::simple_task(std::function<void()> f) : f_(f), finished_(false)
{
}

void simple_task::poll()
{
    // log() << "simple_task" << __func__;
    if (finished_) {
        return;
    }
    f_();
    finished_ = true;
}

bool simple_task::finished()
{
    return finished_;
}

sequence_tasks::sequence_tasks(std::vector<std::unique_ptr<task>> tasks)
    : tasks_(std::move(tasks)), finished_(0)
{
}

void sequence_tasks::poll()
{
    // log() << "sequence_tasks" << __func__;
    if (finished_ >= tasks_.size()) {
        return;
    }
    tasks_[finished_]->poll();
    if (tasks_[finished_]->finished()) {
        ++finished_;
    }
}

bool sequence_tasks::finished()
{
    return finished_ == tasks_.size();
}

parallel_tasks::parallel_tasks(std::vector<std::unique_ptr<task>> tasks,
                               runtime *rt)
    : finished_(tasks.size()),
      running_(tasks.size()),
      tasks_(std::move(tasks)),
      rt_(rt)
{
    std::fill(finished_.begin(), finished_.end(), false);
}

void parallel_tasks::poll()
{
    if (running_ <= 0) {
        return;
    }
    // log() << "parallel_tasks" << __func__;
    if (rt_) {
        rt_->par(tasks_);  // FIXME: yield
        running_ = 0;
        return;
    }
    for (auto i : std::views::iota((size_t)0, tasks_.size())) {
        if (finished_[i]) {
            continue;
        }
        tasks_[i]->poll();
        if (tasks_[i]->finished()) {
            finished_[i] = true;
            --running_;
        }
    }
}

bool parallel_tasks::finished()
{
    return running_ == 0;
}
}  // namespace stdml::collective
