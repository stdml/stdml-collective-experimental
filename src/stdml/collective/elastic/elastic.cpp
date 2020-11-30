#include "libkungfu-elastic-cgo.h"

#include <stdml/bits/collective/elastic.hpp>
#include <stdml/bits/collective/log.hpp>

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);

std::optional<cluster_config> get_cluster_config()
{
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
    auto bs = old_cluster.bytes();
    GoWriteConfigServer(bs.data(), bs.size(), (int)new_size);
}

void commit_cluster_config(const cluster_config &new_cluster,
                           size_t new_version)
{
    log() << __func__ << "called with" << new_cluster;
    auto bs = new_cluster.bytes();
    GoCommitClusterConfig(bs.data(), bs.size(), new_version);
}
}  // namespace stdml::collective
