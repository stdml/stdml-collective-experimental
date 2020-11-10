#pragma once
#include <cstdint>

#include <stdml/bits/address.hpp>
#include <stdml/bits/session.hpp>

namespace stdml::collective
{
class peer
{
    const peer_id self_;
    const peer_list init_peers_;

    peer(const peer_id self, const peer_list init_peers)
        : self_(self), init_peers_(init_peers)
    {
    }

  public:
    static peer single();
    static peer from_env();

    ~peer() {}

    session join()
    {
        session sess(self_, init_peers_);
        return std::move(sess);
    }
};
}  // namespace stdml::collective
