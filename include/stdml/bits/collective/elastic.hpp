#pragma once
#include <cstddef>
#include <optional>

#include <stdml/bits/collective/address.hpp>

namespace stdml::collective
{
struct resize_result {
    bool changed;
    bool detached;
};

std::optional<cluster_config> get_cluster_config();

void propose_new_size(const cluster_config &old_cluster, size_t new_size);

void commit_cluster_config(const cluster_config &config, size_t new_version);
}  // namespace stdml::collective
