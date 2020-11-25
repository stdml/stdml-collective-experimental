#pragma once
#include <experimental/net>
#include <type_traits>

#include <stdml/bits/collective/buffer.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
inline bool would_block(const std::error_code &ec)
{
    return ec.value() == 11;
}

inline std::runtime_error unexpected_eof("unexpected EOF");

template <typename Socket, bool non_blocking = false>
class base_ioutil;

template <typename Socket>
class base_ioutil<Socket, false>
{
  public:
    static void read(Socket &socket, void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t got = 0;
        while (n > 0) {
            auto m = socket.read_some(net::buffer(ptr, n));
            if (m == 0) {
                throw unexpected_eof;
            }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }

    static void read(Socket &socket, void *ptr, size_t n, std::error_code &ec)
    {
        namespace net = std::experimental::net;
        size_t got = 0;
        while (n > 0) {
            auto m = socket.read_some(net::buffer(ptr, n), ec);
            if (ec) {
                return;
            }
            if (m == 0) {
                // FIXME: assign unexpected_eof
                ec.assign(1, std::generic_category());
                return;
            }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }

    static void write(Socket &socket, const void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t sent = 0;
        while (n > 0) {
            auto m = socket.write_some(net::buffer(ptr, n));
            if (m == 0) {
                throw unexpected_eof;
            }
            sent += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }
};

template <typename Socket>
class read_task : public task
{
    Socket &socket_;
    char *ptr_;
    size_t n_;

  public:
    read_task(Socket &socket, void *ptr, size_t n)
        : socket_(socket), ptr_(reinterpret_cast<char *>(ptr)), n_(n)
    {
    }

    void poll() override
    {
        namespace net = std::experimental::net;
        if (n_ == 0) {
            return;
        }
        std::error_code ec;
        auto m = socket_.read_some(net::buffer(ptr_, n_), ec);
        if (ec) {
            if (!would_block(ec)) {
                throw ec;
            }
        } else {
            if (m == 0) {
                throw unexpected_eof;
            } else {
                ptr_ += m;
                n_ -= m;
            }
        }
    }

    bool finished() override
    {
        return n_ == 0;
    }
};

template <typename Socket>
class base_ioutil<Socket, true>
{
  public:
    static std::unique_ptr<task> new_read_task(Socket &socket, void *ptr,
                                               size_t n)
    {
        return std::make_unique<read_task<Socket>>(socket, ptr, n);
    }

    static void read(Socket &socket, void *ptr, size_t n)
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
                throw unexpected_eof;
            }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }

    static void write(Socket &socket, const void *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        size_t sent = 0;
        while (n > 0) {
            std::error_code ec;
            auto m = socket.write_some(net::buffer(ptr, n), ec);
            if (ec) {
                if (would_block(ec)) {
                    std::this_thread::yield();
                    continue;
                } else {
                    throw ec;
                }
            }
            if (m == 0) {
                throw unexpected_eof;
            }
            sent += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }
};

template <typename Socket, bool non_blocking = false>
class basic_ioutil : public base_ioutil<Socket, non_blocking>
{
    using P = base_ioutil<Socket, non_blocking>;

  public:
    using P::read;

    template <typename T>
    static void read(Socket &socket, T &t)
    {
        static_assert(std::is_standard_layout<T>::value);
        read(socket, &t, sizeof(T));
    }

    template <typename T>
    static void read(Socket &socket, T &t, std::error_code &ec)
    {
        static_assert(std::is_standard_layout<T>::value);
        read(socket, &t, sizeof(T), ec);
    }

    using P::write;

    template <typename T>
    static void write(Socket &socket, const T &t)
    {
        static_assert(std::is_standard_layout<T>::value);
        write(socket, &t, sizeof(T));
    }
};
}  // namespace stdml::collective
