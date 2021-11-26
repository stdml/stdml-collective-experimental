#include <cstddef>
#include <cstdint>
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
#include <stdml/bits/collective/http.hpp>
#include <stdml/bits/collective/json.hpp>
#include <stdml/bits/collective/log.hpp>

#include "libkungfu-elastic-cgo.h"

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);

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
}  // namespace stdml::collective
