#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/net/tcp.hpp>

#include <arpa/inet.h>
#include <unistd.h>  // for close

namespace stdml::collective
{
TcpSocket::TcpSocket(std::atomic<bool> *stopped)
    : fd_(socket(AF_INET, SOCK_STREAM, 0)), stopped_(stopped)
{
}

TcpSocket::TcpSocket(int conn, std::atomic<bool> *stopped)
    : fd_(conn), stopped_(stopped)
{
}

TcpSocket::~TcpSocket()
{
    int code = close(fd_);
    log() << fd_ << "closed with" << code;
    _check(code, "close");
}

int TcpSocket::fd()
{
    return fd_;
}

int TcpSocket::recv_all(char *data, size_t size)
{
    int got = 0;
    while (size > 0) {
        if (stopped_ && stopped_->load()) {
            break;
        }
        int n = ::recv(fd_, data, size, 0);
        if (n <= 0) {
            if (errno == 35) {  // FIXME: use system const
                continue;
            }
            break;
        }
        size -= n;
        data += n;
        got += n;
    }
    return got;
}

int TcpSocket::recv(void *data, size_t size)
{
    // int n = ::recv(fd_, data, size, 0);
    int n = recv_all((char *)data, size);
    if (n != (int)size) {
        perror("TcpSocket::recv");
        throw std::runtime_error("TcpSocket::recv partially: " +
                                 std::to_string(size));
    }
    return n;
}

int TcpSocket::recv(void *data, size_t size, int &ec)
{
    int n = recv_all((char *)data, size);
    if (n != (int)size) {
        if (errno) {
            fprintf(stderr, "TcpSocket::recv with ec %d (%s)\n", errno,
                    strerror(errno));
            // shouldn't throw, caller should handle ec
        } else {
            // stopped
        }
        ec = 1;
    }
    return n;
}

int TcpSocket::send(const void *data, size_t size)
{
    int n = ::send(fd_, data, size, 0);
    if (n != (int)size) {
        perror("TcpSocket::send");
        throw std::runtime_error("TcpSocket::send partially: " +
                                 std::to_string(size));
    }
    return n;
}
}  // namespace stdml::collective
