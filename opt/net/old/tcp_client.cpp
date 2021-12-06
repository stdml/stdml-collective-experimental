#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <stdml/bits/collective/net/tcp.hpp>

#include <arpa/inet.h>
#include <unistd.h>  // for close

namespace stdml::collective
{
TcpClient::TcpClient(uint16_t port, uint32_t host)
    : port_(port), host_(host), sock_(socket(AF_INET, SOCK_STREAM, 0))
{
}

TcpClient::~TcpClient()
{
    int code = close(sock_);
    _check(code, "close");
}

void TcpClient::dial(int *ret)
{
    InetAddr addr(port_, host_);
    int code;
    code = addr.connect(sock_);
    if (ret) {
        *ret = code;
        return;
    }
    _check(code, "connect");
}

void TcpClient::write(const void *data, size_t size)
{
    int n = send(sock_, data, size, 0);
    if (n != (int)size) {
        throw std::runtime_error("TcpClient::write: only partial data sent");
    }
}

std::vector<char> TcpClient::Talk(const void *data, size_t size)
{
    dial();
    int n = send(sock_, data, size, 0);

    std::vector<char> reply(n);
    n = recv(sock_, reply.data(), reply.size(), 0);
    return reply;
}

std::vector<char> TcpClient::Talk(const std::vector<char> &buf)
{
    return Talk(buf.data(), buf.size());
}
}  // namespace stdml::collective
