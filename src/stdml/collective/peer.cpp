#include <chrono>
#include <csignal>
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

static peer *__peer = nullptr;

void __stop_peer()
{
    log() << __func__ << "called";
    __peer->stop();
}

void __stop_peer_sign_handler(int sig)
{
    log() << __func__ << "called with" << strsignal(sig);
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(10ms);
    // log() << __func__ << "finished sleep" << strsignal(sig);
    __peer->stop();
    log() << __func__ << "finished" << strsignal(sig);  // exit before here
}

void register_cleanup_handlers(peer *peer)
{
    __peer = peer;
    // std::atexit(__stop_peer);  // doesn't work for Ctrl+C
    std::signal(SIGINT, __stop_peer_sign_handler);
    std::signal(SIGKILL, __stop_peer_sign_handler);
    std::signal(SIGTERM, __stop_peer_sign_handler);
}

peer::peer(const peer_id self, const peer_list init_peers,
           const strategy init_strategy)
    : self_(self),
      init_peers_(init_peers),
      init_strategy_(init_strategy),
      mailbox_(new mailbox),
      slotbox_(new slotbox),
      handler_(rchan::conn_handler::New(mailbox_.get(), slotbox_.get())),
      client_pool_(new rchan::client_pool(self_))
{
    register_cleanup_handlers(this);  // doesn't work
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
    if (!self) {
        return single();
    }
    const auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) {
        return single();
    }
    const strategy s = parse_kungfu_startegy();
    log() << "using strategy" << s;
    peer p(self.value(), peers.value(), s);
    p.start();
    return p;
}

peer peer::from_ompi_env()
{
    const auto size = parse_env_int("OMPI_COMM_WORLD_SIZE");
    if (!size) {
        return single();
    }
    const auto rank = parse_env_int("OMPI_COMM_WORLD_RANK");
    if (!rank) {
        return single();
    }
    const auto ps = peer_list::gen(size.value());
    peer p(ps[rank.value()], ps);
    p.start();
    return p;
}

bool using_kungfu()
{
    return std::getenv("KUNGFU_SELF_SPEC") != nullptr;
}

bool using_ompi()
{
    return std::getenv("OMPI_COMM_WORLD_SIZE") != nullptr;
}

peer peer::from_env()
{
    if (using_kungfu()) {
        return from_kungfu_env();
    }
    if (using_ompi()) {
        return from_ompi_env();
    }
    return single();
}

void peer::start()
{
    server_.reset(rchan::server::New(self_, handler_.get()));
    server_->start();
}

void peer::stop()
{
    log() << "start stop client";
    client_pool_.reset(nullptr);
    log() << "start stop server";
    server_.reset(nullptr);
    log() << "finished stop peer";
}

session peer::join()
{
    session sess(self_, init_peers_, mailbox_.get(), slotbox_.get(),
                 client_pool_.get(), init_strategy_);
    return sess;
}
}  // namespace stdml::collective
