#include <optional>
#include <string>

namespace stdml::collective
{
std::string safe_getenv(const char *name)
{
    const char *ptr = std::getenv(name);
    if (ptr) {
        return std::string(ptr);
    }
    return "";
}

std::optional<int> parse_env_int(const char *name)
{
    const auto v = safe_getenv(name);
    if (v.empty())
        return {};
    return std::stoi(v);
}

bool parse_env_bool(const char *name)
{
    auto v = safe_getenv(name);
    std::transform(v.begin(), v.end(), v.begin(),
                   [](char c) { return std::tolower(c); });
    if (v == "true" || v == "on") {
        return true;
    }
    if (v == "1") {
        return true;
    }
    return false;
}
}  // namespace stdml::collective
