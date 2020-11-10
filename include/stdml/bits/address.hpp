#pragma once
#include <cstdint>
#include <vector>

namespace stdml::collective
{
struct peer_id {
    uint32_t ipv4;
    uint16_t port;

    bool operator==(const peer_id &p) const
    {
        return ipv4 == p.ipv4 && port == p.port;
    }
};

using peer_list = std::vector<peer_id>;
}  // namespace stdml::collective
