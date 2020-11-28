#include <stdml/bits/collective/elastic.hpp>
#include <stdml/bits/collective/log.hpp>

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);

cluster_config get_cluster_config()
{
    auto workers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS")).value();
    auto runners = parse_peer_list(safe_getenv("KUNGFU_INIT_RUNNERS")).value();
    auto config = cluster_config(std::move(runners), std::move(workers));
    log() << config;
    return config;
}

void propose_new_size(size_t size)
{
    // TODO: write cluster config json to config server
}

resize_result propose_cluster_config(const cluster_config &config)
{
    return {false, false};
}
}  // namespace stdml::collective
