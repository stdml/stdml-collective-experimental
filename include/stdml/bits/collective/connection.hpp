#pragma once
#include <memory>

#include <stdml/bits/collective/address.hpp>

namespace stdml::collective::rchan
{
enum conn_type : uint16_t {
    conn_ping,
    conn_control,
    conn_collective,
    conn_peer_to_peer,

    // for profile
    conn_send,
    conn_recv,
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

struct received_header {
    uint32_t name_len;
    std::string name;
    uint32_t flags;
    uint32_t len;
};

class message_reader
{
  public:
    virtual ~message_reader() = default;

    virtual std::optional<received_header> read_header() = 0;

    virtual bool read_body(void *data) = 0;
};

class connection;

class msg_handler
{
  public:
    virtual ~msg_handler() = default;

    virtual bool operator()(const received_header &mh, message_reader *reader,
                            const peer_id &src) = 0;
};

class connection
{
  public:
    static std::unique_ptr<connection> dial(const peer_id remote,
                                            const rchan::conn_type type,
                                            const peer_id local);
    virtual ~connection() = default;

    virtual conn_type type() const = 0;

    virtual peer_id src() const = 0;

    virtual std::unique_ptr<message_reader> next() = 0;

    virtual void send(const char *name, const void *data, size_t size,
                      uint32_t flags = 0) = 0;
};
}  // namespace stdml::collective::rchan
