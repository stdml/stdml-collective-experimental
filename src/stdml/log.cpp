#include <iostream>
#include <mutex>
#include <thread>

#include <stdml/bits/log.hpp>

namespace stdml::collective
{
std::mutex logger::mu_;

logger::logger(std::ostream &os) : lk_(mu_), os(os)
{
    os << "[D] " << std::this_thread::get_id();
}

logger::~logger() { os << std::endl; }

logger log(log_level level)
{
    if (level == PRINT) { return logger(std::cout); }
    return logger(std::cerr);
}
}  // namespace stdml::collective
