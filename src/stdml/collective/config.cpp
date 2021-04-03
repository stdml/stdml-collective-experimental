#include <cstddef>
#include <ostream>
#include <thread>

#include <stdml/bits/collective/config.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const char *name);

system_config parse_system_config_from_env()
{
    runtime_type rt = rt_multi_thread;
    size_t thread_pool_size = 0;
    if (parse_env_bool("STDML_COLLECTIVE_USE_THREAD_POOL")) {
        thread_pool_size = std::thread::hardware_concurrency();
        rt = rt_thread_pool;
    }
    if (parse_env_bool("STDML_COLLECTIVE_USE_ASYNC")) {
        rt = rt_async;
    }
    if (parse_env_bool("STDML_COLLECTIVE_USE_GO_RUNTIME")) {
        rt = rt_go;
    }
    bool use_affinity = false;
    if (parse_env_bool("STDML_COLLECTIVE_USE_AFFINITY")) {
        use_affinity = true;
    }
    return {
        .rt = rt,
        .thread_pool_size = thread_pool_size,
        .use_affinity = use_affinity,
    };
}

const char *runtime_name(runtime_type rt)
{
    switch (rt) {
    case rt_multi_thread:
        return "rt_multi_thread";
    case rt_thread_pool:
        return "rt_thread_pool";
    case rt_async:
        return "rt_async";
    case rt_coro:
        return "rt_coro";
    case rt_go:
        return "rt_go";
    default:
        return "";
    }
}

std::ostream &operator<<(std::ostream &os, const system_config &config)
{
    os << "runtime: " << runtime_name(config.rt);
    return os;
}
}  // namespace stdml::collective
