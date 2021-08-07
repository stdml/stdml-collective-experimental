#include <stdml/bits/collective/elastic_state.hpp>

namespace stdml::collective
{
elastic_state::elastic_state(peer::config_prodiver get_config,
                             std::optional<int64_t> max_progress)
    : peer_(peer::from_env()),
      sess_(peer_.join_elastic()),
      max_progress_(max_progress),
      progress_(0),
      synced_(false),
      detached_(false),
      get_config_(std::move(get_config))
{
}

// extern std::optional<cluster_config> go_get_cluster_config();

// elastic_state::elastic_state(std::optional<int64_t> max_progress)
//     : elastic_state(go_get_cluster_config, max_progress)
// {
// }

elastic_state::operator size_t()
{
    sync();
    return progress_;
}

void elastic_state::operator++()
{
    sync();
    ++progress_;
    check_resize();
}

bool elastic_state::should_stop()
{
    sync();
    return detached_ ||
           (max_progress_.has_value() && progress_ >= max_progress_.value());
}

void elastic_state::sync()
{
    if (!synced_) {
        progress_ = sess_->all_reduce(progress_, max);
        synced_ = true;
    }
}

session &elastic_state::sess()
{
    sync();
    return *sess_;
}

void elastic_state::check_resize()
{
    auto result = peer_.resize(sess_, get_config_);
    if (result.detached) {
        detached_ = true;
    }
    if (result.changed) {
        synced_ = false;
    }
}
}  // namespace stdml::collective
