#include <stdml/bits/collective/address.hpp>

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);
extern std::optional<int> parse_env_int(const char *name);

cluster_config singleton_cluster()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    return cluster_config({}, {id});
}

cluster_config get_init_cluster_config_from_env()
{
    // auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    // if (!self) {
    //     return singleton_cluster();
    // }
    auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) {
        // FIXME: throw?
        return singleton_cluster();
    }
    auto runners = parse_peer_list(safe_getenv("KUNGFU_INIT_RUNNERS"));
    if (!runners) {
        // FIXME: throw?
        return singleton_cluster();
    }
    return cluster_config(runners.value(), peers.value());
}
}  // namespace stdml::collective
