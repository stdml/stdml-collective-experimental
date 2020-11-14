#include <iostream>
#include <mutex>
#include <thread>

#include <stdml/bits/log.hpp>

extern std::string safe_getenv(const char *name);

namespace stdml::collective
{
bool log_enabled()
{
    if (safe_getenv("STDML_ENABLE_LOG") == "1") { return true; }
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
