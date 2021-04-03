#pragma once
#include <cstddef>
#include <ostream>

namespace stdml::collective
{
enum runtime_type {
    rt_multi_thread,
    rt_thread_pool,
    rt_async,
    rt_coro,
    rt_go,
};

struct system_config {
    runtime_type rt;
    size_t thread_pool_size;
    bool use_affinity;
};

system_config parse_system_config_from_env();

const char *runtime_name(runtime_type rt);
std::ostream &operator<<(std::ostream &os, const system_config &config);
}  // namespace stdml::collective
