#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/peer.hpp>
#include <stdml/bits/rchan.hpp>

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

uint32_t pack(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(c) << 8) | (static_cast<uint32_t>(d));
}

std::string safe_getenv(const char *name)
{
    const char *ptr = std::getenv(name);
    if (ptr) { return std::string(ptr); }
    return "";
}

namespace stdml::collective
{
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

peer::peer(const peer_id self, const peer_list init_peers)
    : self_(self),
      init_peers_(init_peers),
      clients_(new rchan::client_pool(self_))
{
    start();
}

peer::~peer() { stop(); }

peer peer::single()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    return peer(id, {id});
}

peer peer::from_kungfu_env()
{
    const auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    if (!self) { return single(); }
    const auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) { return single(); }
    return peer(self.value(), peers.value());
    // p.start();
    // return std::move(p);
}

peer peer::from_env() { return from_kungfu_env(); }

void peer::start()
{
    server_.reset(rchan::server::New(self_));
    server_->start();
}

void peer::stop()
{
    std::cout << "stop peer" << std::endl;
    server_.reset(nullptr);
}
}  // namespace stdml::collective
