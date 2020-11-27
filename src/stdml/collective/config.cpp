#include <stdml/bits/collective/config.hpp>

namespace stdml::collective
{
system_config parse_system_config_from_env()
{
    runtime_type rt = rt_thread;
    return {
        .rt = rt,
        .thread_pool_size = 0,
        .use_affinity = false,
    };
}
}  // namespace stdml::collective
