#pragma once
#include <stdml/bits/address.hpp>
#include <stdml/bits/rchan.hpp>

namespace stdml::collective::rchan
{
class connection
{
  public:
    static connection *dial(const peer_id remote, const rchan::conn_type type,
                            const peer_id local);
    virtual ~connection() = default;

    virtual void send(const char *name, const void *data, size_t size,
                      uint32_t flags) = 0;
};
}  // namespace stdml::collective::rchan
