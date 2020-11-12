#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace stdml::collective::rchan
{
enum conn_type : uint16_t {
    conn_ping,
    conn_control,
    conn_collective,
    conn_peer_to_peer,
};

struct conn_header {
    conn_type type;
    uint16_t src_port;
    uint32_t src_ipv4;
};

inline void _test_conn_header() { static_assert(sizeof(conn_header) == 8); }

struct message_header {
    static constexpr uint32_t wait_recv_buf = 1 << 0;
    static constexpr uint32_t is_response = 1 << 1;
    static constexpr uint32_t request_failed = 1 << 2;

    uint32_t name_len;
    char *name;
    uint32_t flags;  // TODO: meaning of flags should be based on conn Type
};

struct message {
    uint32_t len;
    void *data;
    // uint32_t
    //     flags;  // copied from Header, shouldn't be used during Read or Write
};

// class handler
// {
//   public:
//     virtual void handle() = 0;
// };

class client
{
  public:
    virtual ~client() = default;

    virtual void send(const peer_id target, const char *name, const void *data,
                      size_t size, uint32_t flags = 0) = 0;
};

class client_pool
{
    const peer_id self_;

    std::mutex mu_;
    std::unordered_map<conn_type, std::unique_ptr<client>> client_pool_;

  public:
    client_pool(const peer_id self) : self_(self) {}

    client *require(conn_type type);
};

class server
{
  public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~server() = default;

    static server *New(const peer_id self);
};
}  // namespace stdml::collective::rchan
