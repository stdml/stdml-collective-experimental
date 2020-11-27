#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/mailbox.hpp>

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

inline void _test_conn_header()
{
    static_assert(sizeof(conn_header) == 8);
}

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

struct received_message {
    uint32_t name_len;
    std::unique_ptr<char[]> name;
    uint32_t flags;
    uint32_t len;
    std::unique_ptr<char[]> data;
};

class message_reader
{
  protected:
    struct received_header {
        uint32_t name_len;
        std::string name;
        uint32_t flags;
        uint32_t len;
    };

  public:
    virtual std::optional<received_header> read_header() = 0;
    virtual bool read_body(void *data) = 0;
};

class msg_handler
{
  public:
    virtual ~msg_handler() = default;
    virtual bool operator()(const peer_id &src, void *) = 0;
};

class conn_handler
{
  public:
    virtual ~conn_handler() = default;

    static conn_handler *New(mailbox *mb, slotbox *sb);
};

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
    client_pool(const peer_id self) : self_(self)
    {
    }

    client *require(conn_type type);
};

class server
{
  public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~server() = default;

    static server *New(const peer_id self, conn_handler *handler);
};
}  // namespace stdml::collective::rchan
