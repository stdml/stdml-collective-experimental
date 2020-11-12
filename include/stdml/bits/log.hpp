#pragma once

namespace stdml::collective
{
class logger
{
    static std::mutex mu_;
    std::lock_guard<std::mutex> lk_;
    std::ostream &os;

  public:
    logger(std::ostream &os);

    ~logger();

    template <typename T>
    logger &operator<<(const T &x)
    {
        os << " " << x;
        return *this;
    }
};

enum log_level {
    INFO,
    DEBUG,
    PRINT,
};

logger log(log_level level = INFO);
}  // namespace stdml::collective
