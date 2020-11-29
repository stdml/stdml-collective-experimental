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
extern std::optional<int> parse_env_int(const char *name);
extern rchan::conn_handler *new_collective_handler(mailbox *mb, slotbox *sb);

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

peer::peer(system_config config, peer_id self, peer_list init_peers,
           peer_list init_runners, strategy init_strategy)
    : config_(std::move(config)),
      self_(self),
      init_peers_(std::move(init_peers)),
      init_runners_(std::move(init_runners)),
      init_strategy_(init_strategy),
      mailbox_(new mailbox),
      slotbox_(new slotbox),
      handler_(new_collective_handler(mailbox_.get(), slotbox_.get())),
      client_pool_(new rchan::client_pool(self_))
{
    // register_cleanup_handlers(this);  // doesn't work
}

peer peer::single()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    auto config = parse_system_config_from_env();
    return peer(std::move(config), id, {id}, {});
}

extern strategy parse_kungfu_startegy();

peer peer::from_kungfu_env()
{
    auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    if (!self) {
        return single();
    }
    auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) {
        // FIXME: throw?
        return single();
    }
    auto runners = parse_peer_list(safe_getenv("KUNGFU_INIT_RUNNERS"));
    if (!runners) {
        // FIXME: throw?
        return single();
    }
    auto config = parse_system_config_from_env();
    strategy s = parse_kungfu_startegy();
    log() << "using strategy" << s;
    peer p(std::move(config), self.value(), std::move(peers.value()),
           std::move(runners.value()), s);
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
    auto config = parse_system_config_from_env();
    auto self = ps[rank.value()];
    peer p(std::move(config), self, std::move(ps), {});
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
    server_ = rchan::server::New(self_, handler_.get());
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
    auto rank = std::find(init_peers_.begin(), init_peers_.end(), self_) -
                init_peers_.begin();
    session sess(config_, rank, init_peers_, init_runners_, mailbox_.get(),
                 slotbox_.get(), client_pool_.get(), init_strategy_);
    return sess;
}

#ifdef STDML_COLLECTIVE_ENABLE_ELASTIC
resize_result peer::resize(session &sess, size_t new_size)
{
    auto old_cluster = sess.cluster();
    propose_new_size(old_cluster, new_size);
    return resize(sess);
}

resize_result peer::resize(session &sess)
{
    auto old_cluster = sess.cluster();
    auto new_cluster = [&] {
        for (;;) {
            auto config = get_cluster_config().value_or(old_cluster);
            auto digest = config.bytes();
            if (sess.consistent(digest.data(), digest.size())) {
                return config;
            }
            log() << "cluster config still not consistent";
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }();
    if (old_cluster == new_cluster) {
        log() << "ignore unchanged resize";
        return {false, false};
    }
    return propose_cluster_config(new_cluster);
}
#else
resize_result peer::resize(session &sess, size_t new_size)
{
    log() << __func__ << "NOT enabled";
    return {false, false};
}

resize_result peer::resize(session &sess)
{
    log() << __func__ << "NOT enabled";
    return {false, false};
}
#endif
}  // namespace stdml::collective
