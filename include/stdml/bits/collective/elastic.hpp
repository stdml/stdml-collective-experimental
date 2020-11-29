#pragma once
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

resize_result propose_cluster_config(const cluster_config &config);
}  // namespace stdml::collective
