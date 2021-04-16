#include <cstdint>
#include <cstdio>
#include <cstring>

#include <stdml/bits/collective/net/tcp.hpp>

#include <arpa/inet.h>

namespace stdml::collective
{
inline void unpack(uint32_t n, int &a, int &b, int &c, int &d)
{
    a = n >> 24;
    b = (n >> 16) & 0xff;
    c = (n >> 8) & 0xff;
    d = n & 0xff;
}

InetAddr::InetAddr(uint16_t port, uint32_t host) : addr_(new sockaddr_in)
{
    memset(addr_.get(), 0, sizeof(sockaddr_in));
    addr_->sin_family = AF_INET;
    addr_->sin_port = htons(port);
    if (host) {
        char ip[16];
        int a, b, c, d;
        unpack(host, a, b, c, d);
        sprintf(ip, "%d.%d.%d.%d", a, b, c, d);
        inet_pton(AF_INET, ip, &addr_->sin_addr);
    }
}

int InetAddr::bind(int sock)
{
    return ::bind(sock, get(), sizeof(sockaddr_in));
}

int InetAddr::connect(int sock)
{
    return ::connect(sock, get(), sizeof(sockaddr_in));
}
}  // namespace stdml::collective
