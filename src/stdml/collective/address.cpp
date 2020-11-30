#include <array>
#include <cstring>
#include <experimental/iterator>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>
#include <string>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/json.hpp>

uint32_t pack(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    uint32_t x;
    uint8_t *p = reinterpret_cast<uint8_t *>(&x);
    p[3] = a;
    p[2] = b;
    p[1] = c;
    p[0] = d;
    return x;
}

constexpr std::array<uint8_t, 4> unpack(uint32_t x)
{
    uint8_t *p = reinterpret_cast<uint8_t *>(&x);
    return {p[3], p[2], p[1], p[0]};
}

static std::vector<std::string> split(const std::string &text, const char sep)
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream ss(text);
    while (std::getline(ss, line, sep)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

namespace stdml::collective
{
std::string peer_id::hostname() const
{
    const auto [a, b, c, d] = unpack(ipv4);
    return std::to_string(a) + "." + std::to_string(b) + "." +
           std::to_string(c) + "." + std::to_string(d);
}

peer_id::operator std::string() const
{
    return hostname() + ":" + std::to_string(port);
}

std::optional<peer_id> parse_peer_id(const std::string &s)
{
    uint8_t a, b, c, d;
    uint16_t p;
    if (sscanf(s.c_str(), "%hhu.%hhu.%hhu.%hhu:%hu", &a, &b, &c, &d, &p) == 5) {
        return peer_id{
            .ipv4 = pack(a, b, c, d),
            .port = p,
        };
    }
    return {};
}

std::optional<peer_list> parse_peer_list(const std::string &s)
{
    peer_list ps;
    for (const auto &p : split(s, ',')) {
        const auto id = parse_peer_id(p);
        if (id) {
            ps.push_back(id.value());
        } else {
            return {};
        }
    }
    return ps;
}

std::ostream &operator<<(std::ostream &os, const peer_id &id)
{
    using namespace std::string_literals;
    os << "#<peer:"s << id.hostname() << ":"s << std::to_string(id.port)
       << ">"s;
    return os;
}

std::ostream &operator<<(std::ostream &os, const peer_list &ps)
{
    os << "[";
    std::copy(ps.begin(), ps.end(),
              std::experimental::make_ostream_joiner(os, ", "));
    os << "]";
    return os;
}

peer_list peer_list::gen(size_t n)
{
    peer_list ps;
    for (auto i : std::views::iota((size_t)0, n)) {
        const auto id = parse_peer_id("127.0.0.1:" + std::to_string(i + 10000));
        ps.push_back(id.value());
    }
    return ps;
}

std::vector<std::byte> cluster_config::bytes() const
{
    static_assert(sizeof(peer_id) == 6);

    using N = uint32_t;
    const size_t size_1 = sizeof(peer_id) * runners.size();
    const size_t size_2 = sizeof(peer_id) * workers.size();
    const size_t size = sizeof(N) * 2 + size_1 + size_2;

    std::vector<std::byte> bs(size);
    std::byte *ptr = bs.data();
    *(N *)(ptr + sizeof(N) * 0) = static_cast<N>(runners.size());
    *(N *)(ptr + sizeof(N) * 1) = static_cast<N>(workers.size());
    std::memcpy(ptr + sizeof(N) * 2, runners.data(), size_1);
    std::memcpy(ptr + sizeof(N) * 2 + size_1, workers.data(), size_2);
    return bs;
}

std::optional<cluster_config>
cluster_config::from(const std::vector<std::byte> &bs)
{
    const std::byte *ptr = bs.data();
    using N = uint32_t;
    uint32_t nw = *(N *)(ptr + sizeof(N) * 0);
    uint32_t nr = *(N *)(ptr + sizeof(N) * 1);

    peer_list runners(nw);
    peer_list workers(nr);
    const size_t size_1 = sizeof(peer_id) * runners.size();
    const size_t size_2 = sizeof(peer_id) * workers.size();

    if (size_1 + size_2 + sizeof(N) * 2 != bs.size()) {
        return {};
    }
    std::memcpy(runners.data(), ptr + sizeof(N) * 2, size_1);
    std::memcpy(workers.data(), ptr + sizeof(N) * 2 + size_1, size_2);
    return cluster_config(std::move(runners), std::move(workers));
}

std::string peer_to_json(const peer_id &id)
{
    std::stringstream ss;
    ss << '{';
    std::vector<std::pair<std::string, int>> pairs({
        {"IPv4", id.ipv4},
        {"Port", id.port},
    });
    std::transform(pairs.begin(), pairs.end(),
                   std::experimental::make_ostream_joiner(ss, ","),
                   json::to_json<std::string, int>);
    ss << '}';
    return ss.str();
}

std::string cluster_config::json() const
{
    std::stringstream ss;
    ss << '{';
    ss << '"' << "Runners" << '"' << ':';
    ss << '[';
    std::transform(runners.begin(), runners.end(),
                   std::experimental::make_ostream_joiner(ss, ","),
                   peer_to_json);
    ss << ']';
    ss << ',';
    ss << '"' << "Workers" << '"' << ':';
    ss << '[';
    std::transform(workers.begin(), workers.end(),
                   std::experimental::make_ostream_joiner(ss, ","),
                   peer_to_json);
    ss << ']';
    ss << '}';
    return ss.str();
}

std::ostream &operator<<(std::ostream &os, const cluster_config &config)
{
    os << "runners: " << config.runners << ", workers: " << config.workers;
    return os;
}
}  // namespace stdml::collective
