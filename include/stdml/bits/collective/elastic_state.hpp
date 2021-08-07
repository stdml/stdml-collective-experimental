#pragma once
#include <optional>

#include <stdml/bits/collective/peer.hpp>

namespace stdml::collective
{
class elastic_state
{
    peer peer_;
    std::unique_ptr<session> sess_;
    std::optional<int64_t> max_progress_;

    size_t progress_;
    bool synced_;
    bool detached_;

    peer::config_prodiver get_config_;

    void sync();

    void check_resize();

  public:
    elastic_state(peer::config_prodiver get_config,
                  std::optional<int64_t> max_progress = {});

    // TODO: implement builtin config_prodiver in C++
    // elastic_state(std::optional<int64_t> max_progress = {});

    operator size_t();

    void operator++();

    bool should_stop();

    session &sess();
};
}  // namespace stdml::collective
