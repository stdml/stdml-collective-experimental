#include <array>

#include <stdml/bits/address.hpp>

std::array<uint8_t, 4> unpack(uint32_t x)
{
    return {
        static_cast<uint8_t>(x >> 24),
        static_cast<uint8_t>((x >> 16) & 0xff),
        static_cast<uint8_t>((x >> 8) & 0xff),
        static_cast<uint8_t>(x & 0xff),
    };
}

namespace stdml::collective
{

std::string peer_id::hostname() const
{
    const auto [a, b, c, d] = unpack(ipv4);
    return std::to_string(a) + "." + std::to_string(b) + "." +
           std::to_string(c) + "." + std::to_string(d);
}
}  // namespace stdml::collective
