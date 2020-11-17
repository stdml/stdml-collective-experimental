#include <iostream>
#include <mutex>
#include <thread>

#include <stdml/bits/collective/log.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const std::string &s);

bool log_enabled()
{
    if (parse_env_bool("STDML_ENABLE_LOG")) { return true; }
    return false;
}

std::mutex logger::mu_;
bool logger::enabled_ = log_enabled();

logger::logger(std::ostream &os) : lk_(mu_), os(os)
{
    if (enabled_) { os << "[D] " << std::this_thread::get_id(); }
}

logger::~logger()
{
    if (enabled_) { os << std::endl; }
}

logger log(log_level level)
{
    if (level == PRINT) { return logger(std::cout); }
    return logger(std::cerr);
}
}  // namespace stdml::collective
