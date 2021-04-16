#pragma once
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <stdml/bits/collective/buffer.hpp>
#include <stdml/bits/collective/channel.hpp>

namespace stdml::collective
{
template <typename T>
struct queue_pair {
    channel<T> waitQ;
    channel<T> recvQ;

    void get(T p)
    {
        waitQ.put(p);
        T q = recvQ.get();
        if (p != q) {
            assert(p == q);  // TODO: throw
        }
    }

    void put(const std::function<void(T)> &write)
    {
        T p = waitQ.get();
        write(p);
        recvQ.put(p);
    }
};

template <typename Box>
class basic_mailbox
{
    using key = std::pair<peer_id, std::string>;

    std::mutex mu_;
    std::map<key, std::unique_ptr<Box>> boxes_;

  public:
    using Q = Box;

    Box *require(const peer_id id, const std::string &name)
    {
        const auto k = std::make_pair(id, name);
        std::lock_guard<std::mutex> _(mu_);
        if (auto it = boxes_.find(k); it != boxes_.end()) {
            return it->second.get();
        }
        Box *q = new Box;
        boxes_[k].reset(q);
        return q;
    }
};

using mailbox = basic_mailbox<channel<buffer>>;
using slotbox = basic_mailbox<queue_pair<void *>>;
}  // namespace stdml::collective
