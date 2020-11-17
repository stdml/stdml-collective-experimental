#include <cstdlib>
#include <unordered_map>

#include <stdml/bits/collective/topology.hpp>

extern std::string safe_getenv(const char *name);

namespace stdml::collective
{
strategy parse_kungfu_startegy()
{
    const auto val = std::getenv("KUNGFU_ALLREDUCE_STRATEGY");
    static const std::unordered_map<std::string, strategy> kf_strategy({
        {"RING", ring}, {"STAR", star},
        // {"BINARY_TREE_STAR", 0},
    });
    if (auto it = kf_strategy.find(val); it != kf_strategy.end()) {
        return it->second;
    }
    return star;
}
}  // namespace stdml::collective
