#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <system_error>
#include <vector>
struct sockaddr_in;
struct sockaddr;

namespace stdml::collective
{
constexpr uint32_t pack(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(c) << 8) | (static_cast<uint32_t>(d));
}

constexpr uint32_t any_addr = pack(0, 0, 0, 0);
constexpr uint32_t localhost = pack(127, 0, 0, 1);

inline void _check(int code, const std::string &msg)
{
    if (code) {
        perror(msg.c_str());
        exit(1);
    }
}

class TcpSocket
{
    const int fd_;
    std::atomic<bool> *stopped_;

    int recv_all(char *data, size_t size);

  public:
    TcpSocket(std::atomic<bool> *stopped = nullptr);
    TcpSocket(int fd, std::atomic<bool> *stopped = nullptr);
    ~TcpSocket();
    int fd();

    int recv(void *data, size_t size);
    int recv(void *data, size_t size, int &ec);  // FIXME: std::error_code
    int send(const void *data, size_t size);
};

class TcpServer
{
    const uint16_t port_;
    const uint32_t host_;

    const int sock_;
    std::atomic<bool> stopped_;

  public:
    TcpServer(uint16_t port, uint32_t host = any_addr);
    ~TcpServer();

    void start();

    std::unique_ptr<TcpSocket> wait_accept();

    using Handler = std::function<void(std::unique_ptr<TcpSocket>)>;

    void serve(const Handler &handle);

    void stop();
};

class TcpClient
{
    const uint16_t port_;
    const uint32_t host_;

    const int sock_;

  public:
    TcpClient(uint16_t port, uint32_t host = localhost);
    ~TcpClient();
    void dial(int *code = nullptr);
    void write(const void *data, size_t size);
    std::vector<char> Talk(const void *data, size_t size);
    std::vector<char> Talk(const std::vector<char> &buf);
};

class InetAddr
{
    std::unique_ptr<sockaddr_in> addr_;

  public:
    InetAddr(uint16_t port, uint32_t host);

    const sockaddr *get() const
    {
        return reinterpret_cast<const sockaddr *>(addr_.get());
    }

    int bind(int sock);

    int connect(int sock);
};
}  // namespace stdml::collective
