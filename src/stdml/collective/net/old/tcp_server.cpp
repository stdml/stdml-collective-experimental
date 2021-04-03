#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/net/tcp.hpp>
#include <stdml/bits/collective/patient.hpp>

#include <arpa/inet.h>
#include <unistd.h>  // for close

extern "C" {
extern void reuse_addr(int fd);
extern void set_non_blocking(int fd);
}

bool would_block()
{
    return errno == EAGAIN;
}

namespace stdml::collective
{
TcpServer::TcpServer(uint16_t port, uint32_t host)
    : port_(port),
      host_(host),
      sock_(socket(AF_INET, SOCK_STREAM, 0)),
      stopped_(false)
{
    reuse_addr(sock_);
}

TcpServer::~TcpServer()
{
    int code = close(sock_);
    _check(code, "close");
}

void TcpServer::start()
{
    InetAddr addr(port_, host_);

    int code;
    code = addr.bind(sock_);
    _check(code, "bind");

    code = listen(sock_, 128);
    _check(code, "listen");

    set_non_blocking(sock_);
}

std::unique_ptr<TcpSocket> TcpServer::wait_accept()
{
    for (patient p(5);;) {
        if (stopped_.load()) {
            break;
        }
        if (int fd = accept(sock_, nullptr, nullptr); fd < 0) {
            if (!p.ok()) {
                p.reset();
                log() << "accept failed for" << p.show() << "errno" << errno;
            }
            if (would_block()) {
                std::this_thread::yield();
                continue;
            } else {
                perror("accept failed");
                throw std::runtime_error("accept failed");
            }
        } else {
            p.reset();
            log() << "accepted" << fd;
            return std::make_unique<TcpSocket>(fd, &stopped_);
        }
    }
    return nullptr;
}

void TcpServer::serve(const Handler &handle)
{
    set_non_blocking(sock_);
    patient p(5);
    for (int i = 0;; ++i) {
        if (stopped_.load()) {
            break;
        }
        if (int fd = accept(sock_, nullptr, nullptr); fd < 0) {
            if (!p.ok()) {
                log() << "accept failed for" << p.show() << "with errno"
                      << errno;
            }
            if (would_block()) {
                std::this_thread::yield();
                continue;
            }
        } else {
            p.reset();
            auto conn = std::make_unique<TcpSocket>(fd);
            handle(std::move(conn));
        }
    }
}

void TcpServer::stop()
{
    stopped_.store(true);
}
}  // namespace stdml::collective
