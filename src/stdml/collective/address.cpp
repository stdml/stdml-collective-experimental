#include <array>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>

#include <stdml/bits/collective/address.hpp>

uint32_t pack(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(c) << 8) | (static_cast<uint32_t>(d));
}

std::array<uint8_t, 4> unpack(uint32_t x)
{
    return {
        static_cast<uint8_t>(x >> 24),
        static_cast<uint8_t>((x >> 16) & 0xff),
        static_cast<uint8_t>((x >> 8) & 0xff),
        static_cast<uint8_t>(x & 0xff),
    };
}

static std::vector<std::string> split(const std::string &text, const char sep)
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream ss(text);
    while (std::getline(ss, line, sep)) {
        if (!line.empty()) { lines.push_back(line); }
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

    for (const auto &p : ps) { os << "[" << p << "]"; }
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
}  // namespace stdml::collective
