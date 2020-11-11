#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/session.hpp>

namespace stdml::collective
{
void session::ring_handshake()
{
    const size_t next_rank = (rank_ + 1) % peers_.size();
    const auto next = peers_[next_rank];

    std::cout << "next: " << (std::string)next << std::endl;
    connection::dial(next, rchan::conn_ping, peers_[rank_]);
}
}  // namespace stdml::collective
