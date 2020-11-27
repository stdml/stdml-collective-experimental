#include <string>

#include <stdml/bits/collective/config.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const char *name);

system_config parse_system_config_from_env()
{
    runtime_type rt = rt_thread;
    size_t thread_pool_size = 0;
    if (parse_env_bool("STDML_COLLECTIVE_USE_THREAD_POOL")) {
        thread_pool_size = 3;
    }
    if (parse_env_bool("STDML_COLLECTIVE_USE_ASYNC")) {
        rt = rt_async;
    }
    if (parse_env_bool("STDML_COLLECTIVE_USE_GO_RUNTIME")) {
        rt = rt_go;
    }
    return {
        .rt = rt,
        .thread_pool_size = thread_pool_size,
        .use_affinity = false,
    };
}
}  // namespace stdml::collective
