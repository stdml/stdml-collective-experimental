#pragma once
#include <stdml/bits/session.hpp>

namespace stdml::collective
{
class peer
{
  public:
    peer() {}
    ~peer() {}

    session join()
    {
        session sess;
        return std::move(sess);
    }
};
}  // namespace stdml::collective
