#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/mailbox.hpp>
#include <stdml/bits/collective/rchan.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
class peer
{
    const peer_id self_;
    const peer_list init_peers_;
    const strategy init_strategy_;

    std::unique_ptr<mailbox> mailbox_;
    std::unique_ptr<slotbox> slotbox_;
    std::unique_ptr<rchan::conn_handler> handler_;

    // destruct client before server
    std::unique_ptr<rchan::server> server_;
    std::unique_ptr<rchan::client_pool> client_pool_;  //

  public:
    static peer single();

    static peer from_kungfu_env();
    static peer from_ompi_env();
    static peer from_env();

    peer(const peer_id self, const peer_list init_peers,
         const strategy init_strategy = star);

    void start();
    void stop();

    session join();
};
}  // namespace stdml::collective
