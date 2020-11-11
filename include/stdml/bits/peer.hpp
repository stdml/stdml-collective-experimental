#pragma once
#include <coroutine>
#include <cstdint>
#include <memory>

#include <stdml/bits/address.hpp>
#include <stdml/bits/rchan.hpp>
#include <stdml/bits/session.hpp>

namespace stdml::collective
{
class server
{
  public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~server() = default;
};

class peer
{
    const peer_id self_;
    const peer_list init_peers_;

    std::unique_ptr<server> server_;

    peer(const peer_id self, const peer_list init_peers)
        : self_(self), init_peers_(init_peers)
    {
        start();
    }

  public:
    static peer single();
    static peer from_env();

    ~peer() { stop(); }

    void start();
    void stop();

    session join()
    {
        session sess(self_, init_peers_);
        return std::move(sess);
    }
};
}  // namespace stdml::collective
