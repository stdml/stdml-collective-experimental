#pragma once
#include <mutex>
// #include <unordered_map>
#include <map>

#include <stdml/bits/channel.hpp>

namespace stdml::collective
{
struct buffer {
    std::unique_ptr<char[]> data;
    uint32_t len;
};

class mailbox
{
    using queue = channel<buffer>;
    using key = std::pair<peer_id, std::string>;

    std::mutex mu_;
    std::map<key, std::unique_ptr<queue>> boxes_;

  public:
    using Q = queue;

    queue *require(const peer_id id, const std::string &name)
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
}  // namespace stdml::collective
