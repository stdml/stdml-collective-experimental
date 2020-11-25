#pragma once
#include <experimental/net>

#include <stdml/bits/collective/buffer.hpp>

namespace stdml::collective
{
template <typename Socket, bool non_blocking = false>
class base_ioutil;

template <typename Socket>
class base_ioutil<Socket, false>
{
  public:
    static size_t read(Socket &socket, void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t got = 0;
        while (n > 0) {
            auto m = socket.read_some(net::buffer(ptr, n));
            if (m == 0) {
                // FIXME: check unexpected EOF
                break;
            }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
        return got;
    }

    static size_t write(Socket &socket, const void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t sent = 0;
        while (n > 0) {
            auto m = socket.write_some(net::buffer(ptr, n));
            if (m == 0) {
                break;
            }
            sent += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
        return sent;
    }
};

template <typename Socket>
class base_ioutil<Socket, true>
{
    static bool would_block(const std::error_code &ec)
    {
        return ec.value() == 11;
    }

  public:
    static size_t read(Socket &socket, void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t got = 0;
        while (n > 0) {
            std::error_code ec;
            auto m = socket.read_some(net::buffer(ptr, n), ec);
            if (ec) {
                if (would_block(ec)) {
                    std::this_thread::yield();
                    continue;
                } else {
                    throw ec;
                }
            }
            if (m == 0) {
                // FIXME: check unexpected EOF
                break;
            }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
        return got;
    }
};

template <typename Socket, bool non_blocking = false>
class basic_ioutil : public base_ioutil<Socket, non_blocking>
{
    using P = base_ioutil<Socket, non_blocking>;

  public:
    using P::read;

    template <typename T>
    static size_t read(Socket &socket, T &t)
    {
        return read(socket, &t, sizeof(T));
    }

    using P::write;

    template <typename T>
    static size_t write(Socket &socket, const T &t)
    {
        return write(socket, &t, sizeof(T));
    }
};
}  // namespace stdml::collective
