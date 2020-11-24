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

    static void par(const std::vector<task *> &tasks);
    static void seq(const std::vector<task *> &tasks);
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

class chained_task : public task
{
    std::vector<std::unique_ptr<task>> tasks_;
    size_t finished_;

  public:
    chained_task(std::vector<std::unique_ptr<task>> tasks);

    void poll() override;

    bool finished() override;
};
}  // namespace stdml::collective
