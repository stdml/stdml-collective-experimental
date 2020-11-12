#pragma once
#include <mutex>
// #include <unordered_map>
#include <map>

#include <stdml/bits/buffer.hpp>
#include <stdml/bits/channel.hpp>

namespace stdml::collective
{
class mailbox
{
    using queue = channel<buffer>;
    using key = std::pair<peer_id, std::string>;

    std::mutex mu_;
    std::map<key, std::unique_ptr<queue>> boxes_;

  public:
    using Q = queue;

    Q *require(const peer_id id, const std::string &name)
    {
        const auto k = std::make_pair(id, name);
        std::lock_guard<std::mutex> _(mu_);
        if (auto it = boxes_.find(k); it != boxes_.end()) {
            return it->second.get();
        }
        queue *q = new queue;
        boxes_[k].reset(q);
        return q;
    }
};

class slotbox
{
    struct slot_pair {
        channel<buffer> waitQ;
        channel<buffer> recvQ;
    };

    using key = std::pair<peer_id, std::string>;

    std::mutex mu_;
    std::map<key, std::unique_ptr<slot_pair>> slots_;

  public:
    using S = slot_pair;

    S *require(const peer_id id, const std::string &name)
    {
        const auto k = std::make_pair(id, name);
        std::lock_guard<std::mutex> _(mu_);
        if (auto it = slots_.find(k); it != slots_.end()) {
            return it->second.get();
        }
        S *s = new S;
        slots_[k].reset(s);
        return s;
    }
};
}  // namespace stdml::collective
