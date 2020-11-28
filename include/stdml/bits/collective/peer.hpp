#pragma once
#include <cstdint>
#include <memory>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/config.hpp>
#include <stdml/bits/collective/elastic.hpp>
#include <stdml/bits/collective/mailbox.hpp>
#include <stdml/bits/collective/rchan.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
class peer
{
    const system_config config_;

    const peer_id self_;
    const peer_list init_peers_;
    const peer_list init_runners_;
    const strategy init_strategy_;

    std::unique_ptr<mailbox> mailbox_;
    std::unique_ptr<slotbox> slotbox_;
    std::unique_ptr<rchan::conn_handler> handler_;

    // destruct client before server
    std::unique_ptr<rchan::server> server_;
    std::unique_ptr<rchan::client_pool> client_pool_;  //

    peer(system_config config, peer_id self, peer_list init_peers,
         peer_list init_runners, strategy init_strategy = star);

  public:
    static peer single();

    static peer from_kungfu_env();
    static peer from_ompi_env();
    static peer from_env();

    const system_config &config() const
    {
        return config_;
    }

    void start();
    void stop();

    session join();

    resize_result resize(session &);
    resize_result resize(session &, size_t);
};
}  // namespace stdml::collective
