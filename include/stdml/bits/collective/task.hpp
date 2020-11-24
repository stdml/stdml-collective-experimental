#pragma once
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
}  // namespace stdml::collective
