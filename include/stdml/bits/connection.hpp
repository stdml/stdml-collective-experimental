#pragma once
#include <stdml/bits/address.hpp>

namespace stdml::collective
{
class connection
{
  public:
    static connection *dial(const peer_id id);
};
}  // namespace stdml::collective
