#pragma once
#include <experimental/net>

#include <stdml/bits/collective/buffer.hpp>

namespace stdml::collective
{
inline bool would_block(const std::error_code &ec)
{
    return ec.value() == 11;
}

template <typename Socket>
class basic_ioutil
{
  public:
    static size_t read_b(Socket &socket, void *ptr, size_t n)
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

    static size_t read_nb(Socket &socket, void *ptr, size_t n)
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

    static size_t read(Socket &socket, void *ptr, size_t n)
    {
        // if (socket.native_non_blocking()) {
        //     return read_nb(socket, ptr, n);
        // } else {
        return read_b(socket, ptr, n);
        // }
    }

    template <typename T>
    static size_t read(Socket &socket, T &t)
    {
        return read(socket, &t, sizeof(T));
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

    template <typename T>
    static size_t write(Socket &socket, const T &t)
    {
        return write(socket, &t, sizeof(T));
    }
};
}  // namespace stdml::collective
