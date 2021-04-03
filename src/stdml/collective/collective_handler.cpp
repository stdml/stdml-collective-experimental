#include <cstddef>
#include <map>
#include <memory>
#include <utility>

#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/mailbox.hpp>
#include <stdml/bits/collective/rchan.hpp>

namespace stdml::collective
{
template <rchan::conn_type type>
class msg_handler_impl;

template <>
class msg_handler_impl<rchan::conn_collective> : public rchan::msg_handler
{
    mailbox *mailbox_;
    slotbox *slotbox_;

  public:
    msg_handler_impl(mailbox *mb, slotbox *sb) : mailbox_(mb), slotbox_(sb)
    {
    }

    bool operator()(const rchan::received_header &mh,
                    rchan::message_reader *reader, const peer_id &src) override
    {
        if (mh.flags & rchan::message_header::wait_recv_buf) {
            auto q = slotbox_->require(src, mh.name);
            q->put([&](void *ptr) { reader->read_body(ptr); });
        } else {
            buffer b = alloc_buffer(mh.len);
            reader->read_body(b.data.get());
            auto q = mailbox_->require(src, mh.name);
            q->put(std::move(b));
        }
        return true;
    }
};

class conn_handler_impl : public rchan::conn_handler
{
    std::map<rchan::conn_type, std::unique_ptr<rchan::msg_handler>>
        msg_handlers_;

  public:
    void handle(rchan::conn_type type, rchan::msg_handler *h)
    {
        msg_handlers_[type].reset(h);
    }

    size_t operator()(std::unique_ptr<rchan::connection> conn) override
    {
        if (auto it = msg_handlers_.find(conn->type());
            it != msg_handlers_.end()) {
            auto h = it->second.get();
            return this->handle_to_end(conn.get(), h);
        }
        return 0;
    }
};

rchan::conn_handler *new_collective_handler(mailbox *mb, slotbox *sb)
{
    auto h = new conn_handler_impl;
    h->handle(rchan::conn_collective,
              new msg_handler_impl<rchan::conn_collective>(mb, sb));
    return h;
}
}  // namespace stdml::collective
