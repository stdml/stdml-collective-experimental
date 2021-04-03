#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>
#include <thread>

#include <stdml/bits/collective/log.hpp>

namespace stdml::collective
{
extern bool parse_env_bool(const char *);

static bool log_enabled()
{
    if (parse_env_bool("STDML_COLLECTIVE_ENABLE_LOG")) {
        return true;
    }
    return false;
}

std::mutex logger::mu_;
bool logger::enabled_ = log_enabled();

logger::logger(std::ostream &os) : lk_(mu_), os(os)
{
    if (enabled_) {
        os << std::boolalpha;
        os << "[D] " << std::this_thread::get_id();
    }
}

logger::~logger()
{
    if (enabled_) {
        os << std::endl;
    }
}

logger log(log_level level)
{
    if (level == PRINT) {
        return logger(std::cout);
    }
    return logger(std::cerr);
}

void enable_log()
{
    logger::enabled_ = true;
}
}  // namespace stdml::collective
