#pragma once
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
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

    bool operator<(const peer_id &p) const
    {
        return ipv4 < p.ipv4 || (ipv4 == p.ipv4 && port < p.port);
    }

    std::string hostname() const;

    operator std::string() const;

    uint64_t hash() const
    {
        return (static_cast<uint64_t>(ipv4) << 32) |
               static_cast<uint64_t>(port);
    }
};

std::ostream &operator<<(std::ostream &os, const peer_id &id);

class peer_list : public std::vector<peer_id>
{
    using parent = std::vector<peer_id>;
    using parent::parent;

  public:
    using parent::operator[];

    template <typename I>
    peer_list operator[](const std::vector<I> &ranks) const
    {
        peer_list ps;
        ps.reserve(ranks.size());
        for (auto i : ranks) { ps.push_back(parent::operator[](i)); }
        return ps;
    }

    static peer_list gen(size_t n);
};

std::ostream &operator<<(std::ostream &os, const peer_list &ps);

std::optional<peer_id> parse_peer_id(const std::string &s);

std::optional<peer_list> parse_peer_list(const std::string &s);
}  // namespace stdml::collective
