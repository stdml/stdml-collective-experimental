#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
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
} __attribute__((packed));

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
        for (auto i : ranks) {
            ps.push_back(parent::operator[](i));
        }
        return ps;
    }

    peer_list operator-(const peer_list &ps)
    {
        std::set<peer_id> s(ps.begin(), ps.end());
        peer_list d;
        std::copy_if(begin(), end(), std::back_inserter(d),
                     [&](auto id) { return s.count(id) == 0; });
        return d;
    }

    static peer_list gen(size_t n);

    size_t rank(const peer_id &id) const
    {
        auto pos = std::find(begin(), end(), id);
        return pos - begin();
    }
};

std::ostream &operator<<(std::ostream &os, const peer_list &ps);

std::optional<peer_id> parse_peer_id(const std::string &s);

std::optional<peer_list> parse_peer_list(const std::string &s);

struct cluster_config {
    peer_list runners;
    peer_list workers;

    cluster_config(peer_list runners, peer_list workers)
        : runners(std::move(runners)), workers(std::move(workers))
    {
    }

    bool operator==(const cluster_config &c) const
    {
        return runners == c.runners && workers == c.workers;
    }

    cluster_config resize(size_t new_size) const;

    static std::optional<cluster_config> from(const std::vector<std::byte> &bs);

    std::vector<std::byte> bytes() const;

    std::string json() const;

    static std::optional<cluster_config> from_json(const std::string &js);
};

std::ostream &operator<<(std::ostream &os, const cluster_config &);
}  // namespace stdml::collective
