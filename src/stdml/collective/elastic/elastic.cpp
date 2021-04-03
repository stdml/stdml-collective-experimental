#include <cstddef>
#include <cstdint>
#include <execution>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/elastic.hpp>
#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/json.hpp>
#include <stdml/bits/collective/log.hpp>

#include "libkungfu-elastic-cgo.h"

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);
extern bool http_parse_url(std::string url, std::string &host, uint16_t &port,
                           std::string &path);
extern std::pair<int, std::string> http_get(const char *host, uint16_t port,
                                            const char *path);
extern void http_put(const char *host, uint16_t port, const char *path,
                     const void *ptr, size_t len);

std::optional<cluster_config> get_cluster_config()
{
    {
        auto url = safe_getenv("KUNGFU_CONFIG_SERVER");
        std::string host;
        uint16_t port;
        std::string path;
        if (!http_parse_url(url, host, port, path)) {
            return {};
        }
        auto [code, config] = http_get(host.c_str(), port, path.c_str());
        if (code != 200) {
            return {};
        }
        std::cout << code << std::endl;
        std::cout << config << std::endl;
    }

    std::vector<std::byte> buf(1 << 16);
    int len = GoReadConfigServer(buf.data(), (int)buf.size());
    if (len <= 0) {
        return {};
    }
    buf.resize(len);
    return cluster_config::from(buf).value();
}

void propose_new_size(const cluster_config &old_cluster, size_t new_size)
{
    log() << __func__ << "called";
    auto url = safe_getenv("KUNGFU_CONFIG_SERVER");
    std::string host;
    uint16_t port;
    std::string path;
    if (http_parse_url(url, host, port, path)) {
        auto new_cluster = old_cluster.resize(new_size);
        auto js = new_cluster.json();
        http_put(host.c_str(), port, path.c_str(), js.data(), js.size());
    }
}

struct stage {
    int version;
    cluster_config cluster;

    std::string json() const
    {
        std::stringstream ss;
        ss << '{';
        ss << json::to_json(std::make_pair("Version", version));
        ss << ',';
        ss << json::to_json("Cluster") << ':' << cluster.json();
        ss << '}';
        return ss.str();
    }
};

void commit_cluster_config(const cluster_config &new_cluster,
                           size_t new_version)
{
    log() << __func__ << "called with" << new_cluster;
    stage s = {
        .version = static_cast<int>(new_version),
        .cluster = new_cluster,
    };
    auto js = s.json();
    const auto notify = [&](const peer_id &id) {
        peer_id self = {0, 0};  // TODO: fill
        auto conn = rchan::connection::dial(id, rchan::conn_control, self);
        conn->send("update", js.data(), js.size());
    };
    fmap(std::execution::par, notify, new_cluster.runners);
}
}  // namespace stdml::collective
