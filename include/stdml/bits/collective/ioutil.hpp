#pragma once
#include <experimental/net>
#include <type_traits>

#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
inline bool would_block(const std::error_code &ec)
{
    return ec.value() == 11;
}

inline std::runtime_error unexpected_eof("unexpected EOF");

enum io_type {
    io_read,
    io_write,
};

template <io_type>
struct io_some;

template <>
struct io_some<io_read> {
    template <typename Socket>
    auto operator()(Socket &sock, char *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        return sock.read_some(net::buffer(ptr, n));
    }

    template <typename Socket>
    auto operator()(Socket &sock, char *ptr, size_t n, std::error_code &ec)
    {
        namespace net = std::experimental::net;
        return sock.read_some(net::buffer(ptr, n), ec);
    }
};

template <>
struct io_some<io_write> {
    template <typename Socket>
    auto operator()(Socket &sock, const char *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        return sock.write_some(net::buffer(ptr, n));
    }

    template <typename Socket>
    auto operator()(Socket &sock, const char *ptr, size_t n,
                    std::error_code &ec)
    {
        namespace net = std::experimental::net;
        return sock.write_some(net::buffer(ptr, n), ec);
    }
};

template <typename Socket, bool non_blocking = false>
class base_ioutil;

template <typename Socket>
class base_ioutil<Socket, false>
{
    template <io_type iot>
    static void io_exact(Socket &socket, char *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        while (n > 0) {
            auto m = io_some<iot>()(socket, ptr, n);
            if (m == 0) {
                throw unexpected_eof;
            }
            n -= m;
            ptr += m;
        }
    }

  public:
    static void read(Socket &socket, void *ptr, size_t n)
    {
        io_exact<io_read>(socket, (char *)ptr, n);
    }

    static void read(Socket &socket, void *ptr, size_t n, std::error_code &ec)
    {
        namespace net = std::experimental::net;
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
            n -= m;
            ptr = (char *)(ptr) + m;
        }
    }

    static void write(Socket &socket, const void *ptr, size_t n)
    {
        io_exact<io_write>(socket, (char *)ptr, n);
    }
};

template <typename Socket, io_type iot>
class io_exact_task : public task
{
    Socket &socket_;
    char *ptr_;
    size_t n_;

  public:
    io_exact_task(Socket &socket, void *ptr, size_t n)
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
        auto m = io_some<iot>()(socket_, ptr_, n_, ec);
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
    template <io_type iot>
    static void io_exact(Socket &socket, char *ptr, size_t n)
    {
        namespace net = std::experimental::net;
        while (n > 0) {
            std::error_code ec;
            auto m = io_some<iot>()(socket, ptr, n, ec);
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
            n -= m;
            ptr += m;
        }
    }

  public:
    static std::unique_ptr<task> new_read_task(Socket &socket, void *ptr,
                                               size_t n)
    {
        return std::make_unique<io_exact_task<Socket, io_read>>(socket, ptr, n);
    }

    static std::unique_ptr<task> new_write_task(Socket &socket, const void *ptr,
                                                size_t n)
    {
        return std::make_unique<io_exact_task<Socket, io_write>>(
            socket, (char *)ptr, n);
    }

    static void read(Socket &socket, void *ptr, size_t n)
    {
        io_exact<io_read>(socket, (char *)ptr, n);
    }

    static void write(Socket &socket, const void *ptr, size_t n)
    {
        io_exact<io_write>(socket, (char *)ptr, n);
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
