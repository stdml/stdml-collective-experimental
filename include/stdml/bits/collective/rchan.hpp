#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <stdml/bits/collective/address.hpp>
#include <stdml/bits/collective/connection.hpp>

namespace stdml::collective::rchan
{
class conn_handler
{
  protected:
    bool handle_next(connection *conn, msg_handler *h)
    {
        auto reader = conn->next();
        auto mh = reader->read_header();
        if (!mh.has_value()) {
            return false;
        }
        message_reader *r = reader.get();
        return (*h)(mh.value(), r, conn->src());
    }

    size_t handle_to_end(connection *conn, msg_handler *h)
    {
        for (size_t i = 0;; ++i) {
            if (bool ok = handle_next(conn, h); !ok) {
                return i;
            }
        }
    }

  public:
    virtual ~conn_handler() = default;

    virtual size_t operator()(std::unique_ptr<connection> conn) = 0;
};

class client
{
  public:
    virtual ~client() = default;

    virtual void send(const peer_id target, const char *name, const void *data,
                      size_t size, uint32_t flags = 0) = 0;

    virtual std::unique_ptr<connection> dial(peer_id self, conn_type type) = 0;

    static std::unique_ptr<client> New(peer_id target, conn_type type);
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
    virtual void serve() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~server() = default;

    static std::unique_ptr<server> New(const peer_id self,
                                       conn_handler *handler);
};
}  // namespace stdml::collective::rchan
