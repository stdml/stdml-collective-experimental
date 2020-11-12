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
#include <stdml/bits/log.hpp>
#include <stdml/bits/mailbox.hpp>
#include <stdml/bits/peer.hpp>
#include <stdml/bits/rchan.hpp>

std::string safe_getenv(const char *name)
{
    const char *ptr = std::getenv(name);
    if (ptr) { return std::string(ptr); }
    return "";
}

std::optional<int> parse_env_int(const std::string &s)
{
    const auto v = safe_getenv(s.c_str());
    if (v.empty()) return {};
    return std::stoi(v);
}

namespace stdml::collective
{
peer::peer(const peer_id self, const peer_list init_peers)
    : self_(self),
      init_peers_(init_peers),
      mailbox_(new mailbox),
      handler_(rchan::handler::New(mailbox_.get())),
      client_pool_(new rchan::client_pool(self_))
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
}

peer peer::from_ompi_env()
{
    const auto size = parse_env_int("OMPI_COMM_WORLD_SIZE");
    if (!size) { return single(); }
    const auto rank = parse_env_int("OMPI_COMM_WORLD_RANK");
    if (!rank) { return single(); }
    const auto ps = peer_list::gen(size.value());
    return peer(ps[rank.value()], ps);
}

bool using_kungfu() { return std::getenv("KUNGFU_SELF_SPEC") != nullptr; }

bool using_ompi() { return std::getenv("OMPI_COMM_WORLD_SIZE") != nullptr; }

peer peer::from_env()
{
    if (using_kungfu()) { return from_kungfu_env(); }
    if (using_ompi()) { return from_ompi_env(); }
    return single();
}

void peer::start()
{
    server_.reset(rchan::server::New(self_, handler_.get()));
    server_->start();
}

void peer::stop()
{
    log() << "stop peer";
    client_pool_.reset(nullptr);
    server_.reset(nullptr);
}

session peer::join()
{
    session sess(self_, init_peers_, mailbox_.get(), client_pool_.get());
    return std::move(sess);
}
}  // namespace stdml::collective
