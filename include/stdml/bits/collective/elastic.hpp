#pragma once
#include <stdml/bits/collective/address.hpp>

namespace stdml::collective
{
struct resize_result {
    bool changed;
    bool detached;
};

cluster_config get_cluster_config();

void propose_new_size(size_t size);

resize_result propose_cluster_config(const cluster_config &config);
}  // namespace stdml::collective
