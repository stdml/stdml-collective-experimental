#pragma once
#include <cstddef>

namespace stdml::collective
{
enum runtime_type {
    rt_thread,
    rt_async,
    rt_coro,
};

struct system_config {
    runtime_type rt;
    size_t thread_pool_size;
    bool use_affinity;
};

system_config parse_system_config_from_env();
}  // namespace stdml::collective
