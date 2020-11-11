#pragma once
#include <stdml/bits/address.hpp>
#include <stdml/bits/rchan.hpp>

namespace stdml::collective
{
class connection
{
  public:
    static connection *dial(const peer_id remote, const rchan::conn_type type,
                            const peer_id local);
};
}  // namespace stdml::collective
