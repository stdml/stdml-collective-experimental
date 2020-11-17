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

#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/mailbox.hpp>
#include <stdml/bits/collective/peer.hpp>
#include <stdml/bits/collective/rchan.hpp>

namespace stdml::collective
{
extern std::string safe_getenv(const char *name);
extern std::optional<int> parse_env_int(const std::string &s);

peer::peer(const peer_id self, const peer_list init_peers,
           const strategy init_strategy)
    : self_(self),
      init_peers_(init_peers),
      init_strategy_(init_strategy),
      mailbox_(new mailbox),
      slotbox_(new slotbox),
      handler_(rchan::handler::New(mailbox_.get(), slotbox_.get())),
      client_pool_(new rchan::client_pool(self_))
{
}

peer peer::single()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    return peer(id, {id});
}

extern strategy parse_kungfu_startegy();

peer peer::from_kungfu_env()
{
    const auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    if (!self) { return single(); }
    const auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) { return single(); }
    const strategy s = parse_kungfu_startegy();
    log() << "using strategy" << s;
    peer p(self.value(), peers.value(), s);
    p.start();
    return p;
}

peer peer::from_ompi_env()
{
    const auto size = parse_env_int("OMPI_COMM_WORLD_SIZE");
    if (!size) { return single(); }
    const auto rank = parse_env_int("OMPI_COMM_WORLD_RANK");
    if (!rank) { return single(); }
    const auto ps = peer_list::gen(size.value());
    peer p(ps[rank.value()], ps);
    p.start();
    return p;
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
    session sess(self_, init_peers_, mailbox_.get(), slotbox_.get(),
                 client_pool_.get(), init_strategy_);
    return std::move(sess);
}
}  // namespace stdml::collective
