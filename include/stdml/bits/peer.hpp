#pragma once
#include <coroutine>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <stdml/bits/address.hpp>
#include <stdml/bits/mailbox.hpp>
#include <stdml/bits/rchan.hpp>
#include <stdml/bits/session.hpp>

namespace stdml::collective
{
class peer
{
    const peer_id self_;
    const peer_list init_peers_;

    std::unique_ptr<mailbox> mailbox_;
    std::unique_ptr<rchan::handler> handler_;

    std::unique_ptr<rchan::client_pool> client_pool_;
    std::unique_ptr<rchan::server> server_;

  public:
    static peer single();

    static peer from_kungfu_env();
    static peer from_ompi_env();
    static peer from_env();

    peer(const peer_id self, const peer_list init_peers);

    ~peer();

    void start();
    void stop();

    session join();
};
}  // namespace stdml::collective
